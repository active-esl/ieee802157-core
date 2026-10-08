#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Finite malformed-input regression for the offline symbol fixture only."""
import subprocess
import sys


def check(data, expected):
    result = subprocess.run([sys.argv[1]], input=data, capture_output=True,
                            timeout=5)
    assert result.returncode == expected, (data[:80], result.returncode)
    assert not result.stdout, "Malformed/partial inputs must not produce packets"


for valid in (b"", b"0 5 5 5\n", b"0 -1 -1 -1\n",
              b"18446744073709551615 105 10 8\n",
              b"0 5 5 5\n0 5 5 5\n", b"0 5 5 5"):
    check(valid, 0)
for invalid in (b"-1 5 5 5\n", b"+1 5 5 5\n",
                b"18446744073709551616 5 5 5\n", b"0 256 5 5\n",
                b"0 -2 5 5\n", b"0 -1 5 5\n", b"0 5 5\n",
                b"0 5 5 5 trailing\n", b"0 5.0 5 5\n",
                b"0 999999999999999999999999999999 5 5\n",
                b"9" * 200 + b" 5 5 5\n", b"0 5 5 5\x00 hidden\n",
                b"0 5 5 5\n" * 10001):
    check(invalid, 2)
print("colour fixture: bounded numeric parsing and malformed input PASS")
