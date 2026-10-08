#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Custom slow-colour SYNTHETIC fixture; no camera/device APIs.
Independent packet/colour/exposure model, lossless FFV1 roundtrip, C classifier
and framed decoder. Offline phase acquisition searches a configured period;
no transmitter epoch is passed to it. Arbitrary clock/ROI acquisition is NOT.
"""
import argparse
import binascii
import json
import pathlib
import struct
import subprocess
import time
import sys
import numpy as np
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from examples.linux.colour_phase_search import recover, merge_validated_candidates

T = 125_000_000
EPOCH = 1_000_000_000
FPS = 30
WIDTH = HEIGHT = 16
SYNC = [0, 1, 0, 2, 3, 1, 2, 0, 3, 2, 1, 3, 0, 2, 1, 0]
TRAILER = [3, 1, 3, 2]
LIGHT = np.array([[100, 5, 3], [4, 100, 7], [5, 10, 100], [80, 85, 75], [0, 0, 0]], float)


def run(command, **kwargs):
    return subprocess.run(command, check=True, timeout=60, **kwargs)


def packet(seq, board=42, corrupt=False):
    prefix = struct.pack("<2sBBIIHHI", b"OC", 1, 2, board, 7, seq, 0, 1234)
    data = bytearray(prefix + struct.pack("<H", binascii.crc_hqx(prefix, 0xffff)))
    if corrupt:
        data[15] ^= 1  # after CRC, never recomputed
    return data


def symbols(board=42, corrupt=False):
    result = []
    for seq in range(8):
        payload = [((byte >> shift) & 3) for byte in packet(seq, board, corrupt)
                   for shift in (0, 2, 4, 6)]
        result += SYNC + payload + TRAILER + [4, 4]
    assert len(result) == 880
    return np.array(result)


def exposed_light(start, duration, pattern, period, latency, epoch):
    # Piecewise-constant analytic integral, separate from C waveform generation.
    end = start + duration
    value = np.zeros(3)
    while start < end:
        shifted = start - epoch - latency
        index = int(np.floor(shifted / period))
        boundary = epoch + latency + (index + 1) * period
        stop = min(end, boundary)
        if stop <= start:
            raise AssertionError("non-progressing exposure integration")
        colour = int(pattern[index]) if 0 <= index < len(pattern) else 4
        value += LIGHT[colour] * (stop - start)
        start = stop
    return value / duration


def scenario(build, output, name, params, expected):
    pattern = symbols(params.get("board", 42), params.get("corrupt", False))
    starts = np.arange(3360, dtype=np.int64) * (1_000_000_000 // FPS) + params.get("phase", 17_000_000)
    exposure = params.get("exposure", 10_000_000)
    period = T * (1 + params.get("drift_ppm", 0) / 1e6)
    latency = params.get("latency", 6_300_000)
    epoch = EPOCH + params.get("transmitter_offset", 0)
    colours = []
    for stamp in starts:
        light = exposed_light(float(stamp), exposure, pattern, period, latency, epoch)
        light *= params.get("gain", 1)
        rgb = light + 5 + params.get("ambient", 0)
        if params.get("saturated"):
            rgb[:] = 255
        if params.get("obstructed"):
            rgb[:] = 5
        colours.append(np.rint(np.clip(rgb, 0, 255)).astype(np.uint8))
    colours = np.array(colours)
    raw = np.broadcast_to(colours[:, None, None, :], (len(starts), HEIGHT, WIDTH, 3)).copy().tobytes()
    video = output / (name + ".mkv")
    run(["ffmpeg", "-v", "error", "-f", "rawvideo", "-pix_fmt", "rgb24",
         "-s", f"{WIDTH}x{HEIGHT}", "-r", str(FPS), "-i", "pipe:0", "-an",
         "-c:v", "ffv1", "-pix_fmt", "gbrp", str(video)], input=raw, capture_output=True)
    recovered = run(["ffmpeg", "-v", "error", "-i", str(video), "-f", "rawvideo",
                     "-pix_fmt", "rgb24", "pipe:1"], capture_output=True).stdout
    assert recovered == raw, "lossless colour video roundtrip"
    images = np.frombuffer(recovered, np.uint8).reshape((-1, HEIGHT, WIDTH, 3))
    rgb = images.mean(axis=(1, 2))
    if params.get("gap"):
        # Impairment generation knows the transmitter timeline; the decoder
        # receives only the surviving exposures, never epoch or expected data.
        keep = (starts < epoch + 45 * T) | (starts >= epoch + 48 * T)
        starts, rgb = starts[keep], rgb[keep]
    starts = starts + params.get("timestamp_offset", 0)
    decoded, diagnostics = recover(rgb, starts, exposure, build / "occ_decode_colour_fixture")
    sequences = [p["sequence"] for p in decoded]
    assert sequences == expected, (name, sequences, expected, diagnostics)
    assert diagnostics["conflicting_groups"] == 0, (name, diagnostics)
    for p in decoded:
        assert (p["board"], p["session"], p["kind"], p["status"], p["build"]) == (42, 7, 2, 0, 1234)
    if len(expected) == 8:
        assert decoded[-1]["time_ns"] - int(starts[0]) < 120_000_000_000
    return {"case": name, "sequences": sequences, "accounted_rejections":
            [i for i in range(8) if i not in sequences], "camera_frames": len(starts),
            "decoder": diagnostics, "assumption": diagnostics["assumption"]}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=pathlib.Path, default=pathlib.Path("build"))
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    started = time.monotonic()
    for rgb, starts, exposure in (
            (np.empty((0, 3)), [], 10_000_000),
            (np.zeros((2, 3)), [1, 0], 10_000_000),
            (np.zeros((1, 3)), [-1], 10_000_000),
            (np.zeros((1, 3)), [0.5], 10_000_000),
            (np.zeros((1, 3)), [[0]], 10_000_000),
            (np.zeros((1, 3)), np.array([2**63], dtype=np.uint64), 10_000_000),
            (np.zeros((1, 3)), [0], 10_000_000.5),
            (np.zeros((1, 3)), [0], True),
            (np.zeros((1, 3)), [0], 0),
            (np.zeros((1, 3)), [0], T + 1),
            (np.full((1, 3), np.nan), [0], 10_000_000),
            (np.full((1, 3), 256), [0], 10_000_000),
            (np.zeros((2, 3)), [0, 120_000_000_001], 10_000_000)):
        try:
            recover(rgb, starts, exposure, pathlib.Path("invalid-input-must-not-launch"))
        except ValueError:
            pass
        else:
            raise AssertionError("invalid phase-search input accepted")
    print("phase-search input bounds: PASS", flush=True)
    checked = {"board": 42, "session": 7, "sequence": 1, "kind": 2,
               "status": 0, "build": 1234, "time_ns": 1}
    later = dict(checked, time_ns=2)
    conflicting = dict(checked, status=1)
    assert merge_validated_candidates([checked, later]) == ([later], 0)
    assert merge_validated_candidates([checked] * 20 + [conflicting]) == ([], 1)
    # Conflicting candidates do not contaminate another complete sequence.
    next_packet = dict(checked, sequence=2, time_ns=3)
    assert merge_validated_candidates([checked, conflicting, next_packet]) == ([next_packet], 1)
    assert checked["time_ns"] == 1 and checked["status"] == 0
    print("candidate deduplication/conflict rejection: PASS", flush=True)
    clean = list(range(8))
    cases = [("clean_phase17", {}, clean), ("clean_phase0", {"phase": 0}, clean),
             ("clean_phase32", {"phase": 32_000_000}, clean),
             ("unknown_epoch37", {"transmitter_offset": 37_000_000}, clean),
             ("unknown_epoch103", {"transmitter_offset": 103_000_000}, clean),
             ("large_host_uptime", {"timestamp_offset": 2**54}, clean),
             ("gain", {"gain": 1.3}, clean), ("ambient", {"ambient": 1}, clean),
             ("small_drift", {"drift_ppm": 100}, clean),
             ("gap_recovery", {"gap": True}, list(range(1, 8))),
             ("saturation", {"saturated": True}, []),
             ("obstruction", {"obstructed": True}, []),
             ("wrong_board", {"board": 43}, []),
             ("bad_crc", {"corrupt": True}, []),
             ("long_exposure", {"exposure": T}, [])]
    evidence = []
    for name, params, expected in cases:
        evidence.append(scenario(args.build.resolve(), args.output, name, params, expected))
        print(f"{name}: PASS", flush=True)
        if time.monotonic() - started > 170:
            raise RuntimeError("aggregate synthetic workload cutoff")
    (args.output / "result.json").write_text(json.dumps(evidence, indent=2) + "\n")
    print(f"slow colour: {len(evidence)} synthetic video cases PASS; configured-period offline phase acquisition")


if __name__ == "__main__":
    main()
