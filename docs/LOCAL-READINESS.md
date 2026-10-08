# Local candidate readiness — 8 October 2026

## Current phase-search hardening checkpoint

Fifteen synthetic video cases and explicit input/conflict tests pass. Long host
uptime is tested with a2^54ns timestamp offset; elapsed capture time, not absolute
uptime, owns the120s acceptance cutoff. Fractional or oversized timestamps and
noninteger durations are rejected rather than truncated. A conflicting payload
is rejected even against twenty agreeing candidates; consensus is not authority.
This is still finite offline configured-period acquisition with synthetic
calibration, not streaming/live capture or physical camera/LED proof.

Latest report: `.test-output/colour-video-phase-04/result.json`, SHA256
`f0178faa091366f96a0467141bbbd94e86cfdd66cee03eb77ee6fadc32c894b8`.
Current phase-search source SHA256
`644df50e45dd68e8e37d516bcf35eb38abb6e733a8aff83c130e05190a0feecf`;
current synthetic script SHA256
`5d4098e4b8ba25fcca18d222d5d770bc0f37dfd907fc8bdd32052cb6640b9261`.
Earlier tables, reports and hashes below are retained historical checkpoints,
not evidence for modified inputs.

## Earlier checkpoints

Historical baseline: the table and hashes below identify the earlier calibrated
epoch fixture. The subsequent configured-period phase-search experiment passes
fourteen cases without receiving an optical epoch, including37ms and103ms
transmitter offsets; see COLOUR-PROFILE.md. The synthetic script hash below is
therefore historical and must not be used to validate the newer phase-search
implementation. That newer proof still does not establish streaming/live
capture, arbitrary-rate clock recovery or physical calibration.

New phase-search evidence: `.test-output/colour-video-phase-02/result.json`,
SHA256 `570e6dbab724eac9f446eea09e1859e74ddc2cbba104545d293b756d33c7bfe8`.
Implementation inputs:
`examples/linux/colour_phase_search.py` SHA256
`a956376221cf1378318db8b7670c36ff96443d7f0e0f437c2fcc9b1d1af7b54e`;
`tests/synthetic_colour_video.py` SHA256
`cd09fa213191329f4e863f57a028181b2fb56c51a331973f39cd442644b39481`.
Its24 phase trials are bounded, candidates deduplicated and conflicting
CRC-valid candidate payloads rejected. Runtime input bounds pass before any
subprocess launch. The fourteen video cases all pass; this is synthetic and
not independent interoperability or a field false-positive bound.

This is a bounded evidence checkpoint, not hardware acceptance, publication,
authority or completion of the F1-to-DPX demonstration. The initial board uses
its existing LED package and custom slow-colour signalling; the historical
single-colour UFSOOK/auxiliary-emitter proposal is not the selected next path.

| Requirement | Current evidence | Remaining gate |
|---|---|---|
| Portable Linux/Zephyr core | Allocation-free C99 colour encoder, calibrated classifier and symbol receiver; virtual Zephyr execution passes | Physical adapters and target resource measurements |
| Eight expected envelopes within120s | Synthetic clean cases accept sequences0..7 in110s | Real capture without a supplied optical epoch |
| Corruption/loss handling | All110 symbol-corruption and missing-symbol positions rejected; recovery within two repeated envelopes | Measured camera drops/obstruction and timed observation states |
| Fixture/parser safety | Ten suites pass in GCC Debug/Release/ASan+UBSan;100000 deterministic colour-input iterations; overflow/NUL/length/record limits tested | Independent review and Clang hosted results for this candidate |
| Video impairments | Twelve analytic-exposure/FFV1 cases; every expected sequence accepted or explicitly accounted as rejected | Blind clock/ROI acquisition, real exposure/colour calibration and multiple boards |
| Fitted channel mapping | Earlier source/image corroboration reports R/G/B/W on the tested LED package | Exact current board handoff and independently verified operation coverage |
| Electrical/timing safety | Analytic full-driver update lower bound6.3ms at100kHz; held-symbol package duty98.18% | Measured peak/average current, thermal/off behaviour, optical transition gap and jitter |
| Pressure/CAN isolation | No target measurements from this core | Agree bounded baseline/load and require zero added deadline misses/errors; record CPU/RAM/bus |
| Independent DPX camera path | No camera operation by this lane | Exact camera/device/user/formats, exposure/row timing, restoration, board-only ROI and no audio |
| Human status fallback | Required by recommendation | Shared-firmware arbitration and safe failure indication |
| CI/publication | Workflows include colour video and Zephyr colour marker; syntax/control checks pass locally | Reviewed candidate and exact destination/grant; local changes remain uncommitted |

The latest video report is local ignored evidence:
`.test-output/colour-video-03/result.json`, SHA256
`7bc811700d1fa5c9637e5cb84248304b0f26d9adb3db5d2d3debea3e1f343e5a`.
Clean camera phases, gain, ambient and small drift accept0..7. Gap/recovery
rejects0 and accepts1..7. Saturation, obstruction, wrong board, bad CRC and long
exposure accept no packets. Known optical epoch/period remains an explicit
assumption in every case; none proves an asynchronous board/camera link.

## Frozen colour implementation inputs

These identify the reviewed/tested local inputs, not a release or the complete
repository tree. Source changes invalidate the corresponding evidence.

| File | SHA256 |
|---|---|
| src/colour.c | 3aadd04efde77cdad559b92decdddb572fe5accf45338251f20f9a8eff16c18f |
| include/occ/colour.h | 4cd098855d689014dd4fb4ba1f70b8e7782b4c75a48a3e2e884910c2acd7c85a |
| examples/linux/decode_colour_fixture.c | bb928078ed4b57efe2f330186f444b9dcf8c755c2693b4b3d0073805dee34297 |
| examples/zephyr/src/main.c | f19d89da530c0ed49790c63243a764fee5ac0b49179f3d88114bbfb4c78b082e |
| tests/test_colour.c | 50aaa49db2997ca32d194b61be2084d0fec1b53a008a5c21703e0d49a0c73623 |
| tests/test_colour_fixture.py | 7cf0b352bc565007b1534211829820fbca11512787344ea1d778d1665bb7c203 |
| tests/synthetic_colour_video.py | f107ef5eca93d1f4e2b2f7932001c511662b55936b2d39ba10e2f37b6dfda176 |

Before board use, obtain a factual Bluetooth/LP5811-owner handoff and verify
exact subject/resource/candidate/current/duty/camera/shared-file coverage.
The named chat was not found in the bounded available inventory during this
checkpoint; no new coordination or reservation is claimed. Incoming reports
cannot grant authority. CRC and source labels do not prevent spoofing or replay;
no credentials, confidential payloads or decoded instructions are permitted.

Alex confirmed broader light-communications naming. The same public repository
was renamed to `active-esl/light-comms-core` under independently verified
action-bound coverage, preserving its numeric identity and history. This
checkpoint remains local candidate evidence, not a hosted-CI or hardware pass.
