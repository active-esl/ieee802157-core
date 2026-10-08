#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finite, offline, no-camera waveform -> exposure -> FFV1 -> C decoder test.
The scene model uses independent mathematical square-wave integration, not
receiver symbols; FFmpeg output is checked byte-for-byte before decoding.
"""
import argparse
import hashlib
import json
import pathlib
import subprocess
import time

import numpy as np

WIDTH = HEIGHT = 64
PERIOD = 33_333_333
PHASE = 600_000
EXPOSURE = 1_666_667


def run(command, **kwargs):
    return subprocess.run(command, check=True, timeout=60, **kwargs)


def high_time(t, period):
    cycles, remainder = divmod(t, period)
    return cycles * period / 2 + min(remainder, period / 2)


def exposed(segments, start, exposure, channel=1):
    """Analytic ON-time across all segments; no bit/symbol oracle."""
    remaining = exposure
    total = 0.0
    offset = 0
    for segment in segments:
        end = offset + segment["duration_ns"]
        if start >= end:
            offset = end
            continue
        local = max(0, start - offset)
        duration = min(remaining, segment["duration_ns"] - local)
        if duration <= 0:
            break
        if segment["active"] & channel:
            if segment["rate_num"]:
                period = 1e9 * segment["rate_den"] / segment["rate_num"]
                on = high_time(local + duration, period) - high_time(local, period)
                if segment["inverted"] & channel:
                    on = duration - on
            else:
                on = duration
            total += on
        remaining -= duration
        start += duration
        offset = end
        if not remaining:
            break
    return total / exposure


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=pathlib.Path, default=pathlib.Path("build"))
    parser.add_argument("--output", type=pathlib.Path, required=True)
    options = parser.parse_args()
    build = options.build.resolve()
    output = options.output.resolve()
    # New attempt directory only; preserve previous evidence.
    output.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    fixture = json.loads(run([str(build / "occ_fixture_json")], capture_output=True).stdout)
    (output / "fixture.json").write_text(json.dumps(fixture, indent=2) + "\n")
    frame_count = 360
    scenarios = [
        ("clean", 1, {}),
        ("small_jitter", 1, {"jitter": 100_000}),
        ("dropped_frame", 0, {"drop": 80}),
        ("long_exposure", 0, {"exposure": PERIOD}),
        ("saturated", 0, {"constant": 255}),
        ("ambient_bright", 0, {"constant": 220}),
        ("no_signal", 0, {"constant": 20}),
        ("dim_signal", 0, {"span": 70}),
        ("second_board_outside_roi", 1, {"foreign": True}),
        ("early_capture_phase", 1, {"phase": 100_000}),
        ("late_capture_phase", 1, {"phase": 5_000_000}),
        ("transition_capture_phase", 0, {"phase": 3_800_000}),
        ("frame_rate_29_97", 0, {"period": 33_366_700}),
    ]
    results = []
    for name, expected, parameters in scenarios:
        exposure = parameters.get("exposure", EXPOSURE)
        timestamps = []
        images = []
        for frame in range(frame_count):
            if frame == parameters.get("drop"):
                continue
            jitter = parameters.get("jitter", 0) * (-1 if frame % 2 else 1)
            timestamp = parameters.get("phase", PHASE) + frame * parameters.get("period", PERIOD) + jitter
            fraction = exposed(fixture["segments"], timestamp, exposure)
            level = round(20 + parameters.get("span", 200) * fraction)
            level = parameters.get("constant", level)
            image = np.full((HEIGHT, WIDTH), 20, dtype=np.uint8)
            image[28:36, 28:36] = level
            if parameters.get("foreign"):
                image[4:12, 4:12] = 220 if frame % 2 else 20
            timestamps.append(timestamp)
            images.append(image.tobytes())
        raw = b"".join(images)
        video = output / (name + ".mkv")
        times = output / (name + ".timestamps")
        times.write_text("".join(str(t) + "\n" for t in timestamps))
        run(["ffmpeg", "-v", "error", "-f", "rawvideo", "-pixel_format", "gray",
             "-video_size", "64x64", "-framerate", "30", "-i", "pipe:0",
             "-c:v", "ffv1", "-pix_fmt", "gray", "-y", str(video)], input=raw,
            stdout=subprocess.DEVNULL)
        decoded = run(["ffmpeg", "-v", "error", "-i", str(video),
                       "-f", "rawvideo", "-pix_fmt", "gray", "pipe:1"],
                      capture_output=True).stdout
        if raw != decoded:
            raise AssertionError("Lossless frame roundtrip mismatch: " + name)
        result = json.loads(run(
            [str(build / "occ_decode_gray"), "64", "64", "28", "28", "8", "8",
             str(times), str(PERIOD), "500000", str(fixture["board"])],
            input=decoded, capture_output=True).stdout)
        if result["packets"] != expected:
            raise AssertionError(name + ": " + json.dumps(result))
        result.update(name=name, expected_packets=expected,
                      video_sha256=hashlib.sha256(video.read_bytes()).hexdigest())
        results.append(result)
        print(name + ": PASS packets=" + str(result["packets"]), flush=True)
    report = {"scope": "Synthetic global-shutter-like grayscale UFSOOK scene only; not camera/board proof",
              "results": results, "elapsed_seconds": time.monotonic() - started}
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: " + str(len(results)) + " synthetic video cases", flush=True)


if __name__ == "__main__":
    main()
