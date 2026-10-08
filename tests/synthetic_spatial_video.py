#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline S2-PSK two-source exposure scenes, not co-located RGB."""
import argparse
import hashlib
import json
import pathlib
import subprocess

import numpy as np
from synthetic_video import exposed, run

PERIOD = 33_333_333
FRAME_COUNT = 1080
PHASE = 50_000
EXPOSURE = 100_000


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--build", type=pathlib.Path, default=pathlib.Path("build"))
    p.add_argument("--output", type=pathlib.Path, required=True)
    args = p.parse_args()
    build, output = args.build.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = json.loads(run([str(build / "occ_fixture_json"), "s2psk"], capture_output=True).stdout)
    (output / "fixture.json").write_text(json.dumps(fixture, indent=2) + "\n")
    cases = [
        ("clean_two_sources", 1, {}),
        ("small_jitter", 1, {"jitter": 10_000}),
        ("one_dropped_camera_frame", 1, {"drop": {100}}),
        ("two_dropped_frames_in_symbol", 0, {"drop": {100, 101}}),
        ("full_carrier_period_exposure", 0, {"exposure": 1_000_000}),
        ("missing_second_source", 0, {"missing": True}),
        ("saturated_second_source", 0, {"saturated": True}),
        ("second_source_row_skew", 0, {"row_skew": 500_000}),
    ]
    results = []
    for name, expected, parameters in cases:
        frames, timestamps = [], []
        for i in range(FRAME_COUNT):
            if i in parameters.get("drop", set()):
                continue
            t = PHASE + i * PERIOD + parameters.get("jitter", 0) * (-1 if i % 2 else 1)
            exposure = parameters.get("exposure", EXPOSURE)
            value_a = round(20 + 200 * exposed(fixture["segments"], t, exposure, 1))
            value_b = round(20 + 200 * exposed(fixture["segments"], t + parameters.get("row_skew", 0), exposure, 2))
            if parameters.get("missing"):
                value_b = 20
            if parameters.get("saturated"):
                value_b = 255
            image = np.full((64, 64), 20, dtype=np.uint8)
            image[28:36, 12:20] = value_a
            image[28:36, 44:52] = value_b
            frames.append(image.tobytes())
            timestamps.append(t)
        raw = b"".join(frames)
        video, times = output / (name + ".mkv"), output / (name + ".timestamps")
        times.write_text("".join(str(t) + "\n" for t in timestamps))
        run(["ffmpeg", "-v", "error", "-f", "rawvideo", "-pixel_format", "gray",
             "-video_size", "64x64", "-framerate", "30", "-i", "pipe:0",
             "-c:v", "ffv1", "-pix_fmt", "gray", "-y", str(video)],
            input=raw, stdout=subprocess.DEVNULL)
        decoded = run(["ffmpeg", "-v", "error", "-i", str(video), "-f", "rawvideo",
                       "-pix_fmt", "gray", "pipe:1"], capture_output=True).stdout
        assert raw == decoded, name + ": video roundtrip"
        result = json.loads(run([str(build / "occ_decode_spatial"), str(times)],
                                input=decoded, capture_output=True).stdout)
        assert result["packets"] == expected, name + ": " + json.dumps(result)
        result.update(name=name, expected_packets=expected,
                      video_sha256=hashlib.sha256(video.read_bytes()).hexdigest())
        results.append(result)
        print(name + ": PASS packets=" + str(result["packets"]), flush=True)
    (output / "result.json").write_text(json.dumps(
        {"scope": "Synthetic two-source S2-PSK with calibrated clock epoch; not F1 RGB/DPX proof",
         "results": results}, indent=2) + "\n")
    print("PASS: 8 synthetic spatial video cases", flush=True)


if __name__ == "__main__":
    main()
