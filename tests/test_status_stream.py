#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finite local status integration fixtures; no camera/board access."""
import binascii
import json
import struct
import subprocess
import sys


def packet(board=42, session=99, sequence=7, status=0):
    data = struct.pack("<2sBBIIHHI", b"OC", 1, 2, board, session, sequence, status, 1234)
    return (data + struct.pack("<H", binascii.crc_hqx(data, 0xffff))).hex()


def run(records):
    return subprocess.run([sys.argv[1], "42", "10", "20"], input=records,
                          text=True, capture_output=True, timeout=10)


def main():
    damaged = packet()[:-2] + "00"
    assert damaged != packet()
    events = [(0, "tick"), (1, damaged), (2, packet()), (21, packet()),
              (22, "tick"), (31, "tick"), (32, packet(board=43)),
              (33, packet(sequence=8)), (34, packet(sequence=8, status=1)),
              (35, packet(sequence=8)), (36, packet(session=100, sequence=9)),
              (37, packet(sequence=7)), (38, "uncertain"), (48, "tick")]
    result = run("".join(f"{stamp} {value}\n" for stamp, value in events))
    assert result.returncode == 0, result.stderr
    records = [json.loads(line) for line in result.stdout.splitlines()]
    assert [r["state"] for r in records] == [
        "no_signal", "uncertain", "valid", "valid", "stale", "no_signal",
        "uncertain", "valid", "uncertain", "valid", "uncertain", "uncertain",
        "uncertain", "no_signal"]
    assert [i for i, r in enumerate(records) if r["progressed"]] == [2, 7]
    assert [i for i, r in enumerate(records) if r["accepted"]] == [2, 3, 7, 9]
    assert all(not r["authenticated"] and not r["authoritative"] for r in records)
    assert records[2]["packet"]["sequence"] == 7
    assert records[7]["packet"]["sequence"] == 8
    assert records[8]["packet"] is None
    for malformed in ("1 tick\n0 tick\n", "-1 tick\n", "1 reset\n", "1 tick extra\n",
                      "1 " + "a" * 200 + "\n", "1 tick", "1 abc\n", "1 tick\0hidden\n"):
        assert run(malformed).returncode == 2
    assert run("".join(f"{i} tick\n" for i in range(10001))).returncode == 2
    print("PASS: status handoff, repeats/staleness/outage, rejected identities/session/conflicts, parser bounds")


if __name__ == "__main__":
    main()
