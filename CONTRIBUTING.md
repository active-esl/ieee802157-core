# Contributing

This is an experimental GPL-3.0-only project, not a certified IEEE stack.
Contributions must preserve OS/RTOS/LED/camera abstraction, bounded caller-owned
state, explicit uncertainty and the distinction between integrity and authenticity.

Run CMake/CTest in Debug, Release and sanitizer builds before proposing a change.
For modulation/image changes, also run the relevant synthetic-video suite and
record profile parameters, expected acceptance counts and deviations. Keep
fixtures independent of decoder logic where possible. Zephyr changes need the
bounded native simulation; do not infer board/timing proof from simulation.
See docs/CI.md and docs/TESTING.md for coverage and limitations.

Use SPDX GPL-3.0-only headers in new code/test/build files and document upstream
provenance/licences for any reuse. Do not submit copyrighted standards text,
credentials, signed grants, device backups, private recordings or confidential
diagnostics. No OKATEM source is currently copied into this repository.

Keep changes small and describe measured evidence, not intended behavior.
Changing a hardware driver, current, fault safeguard or timing path requires
separate target-owner review and authorized bench evidence. CI is hardware-free
and does not grant permission to operate a board or camera.
