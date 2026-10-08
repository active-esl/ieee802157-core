# SPDX-License-Identifier: GPL-3.0-only
"""Verify source selection in generated Ninja rules, not just smoke markers."""
import pathlib
import sys


def selected(root):
    rules = (pathlib.Path(root) / "build.ninja").read_text()
    return {name for name in ("occ", "packet", "player", "receiver", "image",
                             "spatial", "cook", "rolling", "colour")
            if f"src/{name}.c.obj:" in rules}


if len(sys.argv) != 3:
    raise SystemExit("usage: check_zephyr_selection.py DISABLED_BUILD SUBSET_BUILD")
if selected(sys.argv[1]):
    raise SystemExit("disabled module still compiles protocol sources")
expected = {"occ", "packet", "player", "colour"}
actual = selected(sys.argv[2])
if actual != expected:
    raise SystemExit(f"subset source mismatch: expected {expected}, got {actual}")
print("LIGHT_COMMS_SELECTION_PASS")
