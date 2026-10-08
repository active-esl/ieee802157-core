#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finite offline phase acquisition with a configured symbol period.

Inputs are ROI-averaged RGB samples and actual exposure-start timestamps.
No transmitter epoch, expected packet, camera API or device access is accepted.
The C fixture supplies SYNTHETIC calibration; this is not a live camera adapter.
"""
import json
import subprocess
import time

import numpy as np


def merge_validated_candidates(records):
    """Deduplicate already framed/CRC-checked C-decoder outputs, not raw data.

    Agreement is not authenticity. A conflict suppresses that entire identity /
    session / sequence group, even if one payload has more candidate votes.
    """
    groups = {}
    for packet in records:
        key = (packet["board"], packet["session"], packet["sequence"])
        payload = tuple(packet[field] for field in ("kind", "status", "build"))
        groups.setdefault(key, {}).setdefault(payload, []).append(packet)
    decoded = []
    conflicts = 0
    for alternatives in groups.values():
        if len(alternatives) != 1:
            conflicts += 1
            continue
        packets = next(iter(alternatives.values()))
        decoded.append(max(packets, key=lambda packet: packet["time_ns"]))
    decoded.sort(key=lambda packet: packet["time_ns"])
    return decoded, conflicts


def recover(rgb, starts, exposure_ns, decoder, period_ns=125_000_000):
    rgb = np.asarray(rgb)
    starts = np.asarray(starts)
    if (starts.ndim != 1 or not np.issubdtype(starts.dtype, np.integer)
            or np.any(starts > np.iinfo(np.int64).max)
            or not np.issubdtype(rgb.dtype, np.number)
            or np.issubdtype(rgb.dtype, np.complexfloating)
            or not isinstance(period_ns, (int, np.integer))
            or not isinstance(exposure_ns, (int, np.integer))
            or isinstance(period_ns, (bool, np.bool_))
            or isinstance(exposure_ns, (bool, np.bool_))):
        raise ValueError("integer timestamps/durations and numeric RGB required")
    starts = starts.astype(np.int64, copy=False)
    if (not 1 <= len(starts) <= 10000 or rgb.shape != (len(starts), 3)
            or np.any(starts < 0) or np.any(np.diff(starts) <= 0)
            or int(starts[-1]) - int(starts[0]) > 120_000_000_000
            or not 0 < exposure_ns <= period_ns or period_ns != 125_000_000
            or not np.all(np.isfinite(rgb)) or np.any(rgb < 0) or np.any(rgb > 255)
            or int(starts[-1]) > np.iinfo(np.int64).max - exposure_ns):
        raise ValueError("invalid/bounded offline phase-search input")
    origin = int(starts[0]) // period_ns * period_ns
    # Keep window arithmetic relative: long host uptime must not destroy
    # nanosecond precision when computing exposure centres with a half duration.
    relative = starts - origin
    centres = relative + exposure_ns / 2
    records = []
    began = time.monotonic()
    # Fixed, bounded phase grid. Packet integrity/framing selects plausible
    # phases, NOT sender authenticity. No arbitrary clock-rate search is made.
    for trial in range(24):
        if time.monotonic() - began > 30:
            raise TimeoutError("offline phase-search budget")
        phase = trial * period_ns // 24
        lines = []
        bins = (int(relative[-1]) + exposure_ns - phase) // period_ns + 1
        for index in range(max(0, bins)):
            left = phase + index * period_ns
            chosen = np.flatnonzero((centres >= left + period_ns * .3)
                                   & (centres < left + period_ns * .7)
                                   & (relative >= left + period_ns * .2)
                                   & (relative + exposure_ns <= left + period_ns * .8))
            if len(chosen) and np.max(np.ptp(rgb[chosen], axis=0)) <= 8:
                value = np.rint(rgb[chosen].mean(axis=0)).astype(int)
            else:
                value = [-1, -1, -1]  # absent/disagreeing frames: never interpolate
            lines.append(f"{origin + left + period_ns // 2} {value[0]} {value[1]} {value[2]}\n")
        result = subprocess.run([str(decoder)], input="".join(lines).encode(),
                                capture_output=True, check=True, timeout=5)
        for line in result.stdout.splitlines():
            packet = json.loads(line)
            records.append(packet)
    decoded, conflicts = merge_validated_candidates(records)
    return decoded, {"phase_trials": 24, "candidate_packets": len(records),
                     "conflicting_groups": conflicts,
                     "assumption": "configured period and synthetic colour calibration; no optical epoch"}
