#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finite calibrated C-OOK rolling-shutter source model, no camera access."""
import argparse
import hashlib
import json
import pathlib
import subprocess

import numpy as np
from synthetic_video import exposed, run

FRAME_PERIOD = 33_333_333
PHASE = 80_000
ROW_PERIOD = 40_000
EXPOSURE = 10_000


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=pathlib.Path, default=pathlib.Path("build"))
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    build, output = args.build.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = json.loads(run([str(build / "occ_fixture_cook")], capture_output=True).stdout)
    (output / "fixture.json").write_text(json.dumps(fixture, indent=2) + "\n")
    cases = [
        ("calibrated_rolling_source", 1, {}),
        ("lost_fragment_frame", 0, {"drop": {10}}),
        ("lost_frame_then_whole_envelope_repeat", 1, {"drop": {10}, "cycles": 2}),
        ("long_row_exposure", 0, {"exposure": 500_000}),
        ("tiny_source_image", 0, {"height": 16}),
        ("saturated_source", 0, {"constant": 255}),
        ("no_signal", 0, {"constant": 20}),
        ("wrong_row_readout", 0, {"row_period": 45_000}),
    ]
    results = []
    for name, expected, parameters in cases:
        timestamps, frames = [], []
        for i in range(22 * parameters.get("cycles", 1)):
            if i in parameters.get("drop", set()):
                continue
            image = np.full((720, 64), 20, dtype=np.uint8)
            segments = fixture["fragments"][i % 22]["segments"]
            for row in range(parameters.get("height", 720)):
                fraction = exposed(segments, PHASE + row * parameters.get("row_period", ROW_PERIOD),
                                   parameters.get("exposure", EXPOSURE))
                value = parameters.get("constant", round(20 + 200 * fraction))
                image[row, 28:36] = value
            frames.append(image.tobytes())
            timestamps.append(i * FRAME_PERIOD + PHASE)
        raw = b"".join(frames)
        video, times = output / (name + ".mkv"), output / (name + ".timestamps")
        times.write_text("".join(str(t) + "\n" for t in timestamps))
        run(["ffmpeg", "-v", "error", "-f", "rawvideo", "-pixel_format", "gray",
             "-video_size", "64x720", "-framerate", "30", "-i", "pipe:0",
             "-c:v", "ffv1", "-pix_fmt", "gray", "-y", str(video)],
            input=raw, stdout=subprocess.DEVNULL)
        decoded = run(["ffmpeg", "-v", "error", "-i", str(video), "-f", "rawvideo",
                       "-pix_fmt", "gray", "pipe:1"], capture_output=True).stdout
        assert raw == decoded, name + ": lossless video roundtrip"
        result = json.loads(run([str(build / "occ_decode_rows"), str(times)],
                                input=decoded, capture_output=True).stdout)
        assert result["packets"] == expected, name + ": " + json.dumps(result)
        result.update(name=name, expected_packets=expected,
                      video_sha256=hashlib.sha256(video.read_bytes()).hexdigest())
        results.append(result)
        print(name + ": PASS packets=" + str(result["packets"]), flush=True)
    (output / "result.json").write_text(json.dumps(
        {"scope": "Synthetic calibrated enlarged-source C-OOK fragments; not DPX or small F1 LED proof",
         "results": results}, indent=2) + "\n")
    print("PASS: 8 synthetic rolling-shutter video cases", flush=True)


if __name__ == "__main__":
    main()
