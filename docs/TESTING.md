# Offline test evidence and remaining gates

All tests run locally on the Framework laptop with already-installed tools;
no camera, microphone, LED driver, board, serial/RTT/SSH device or firmware
operation is involved. The licensed IEEE PDF is not included in the repository.

## Evidence as of 8 October 2026

GNU C13.3.0 native Debug, Release and ASan/UBSan builds pass eight CTest suites:
core, receiver, image, player, spatial, cook, hardening and status_stream (the
last uses the installed Python interpreter to test the C Linux adapter). Assertions are
explicitly enabled in Release fixtures. Core compiles with strict warnings
including conversion warnings treated as errors. Packet fixtures
reject176 individual single-bit corruptions. Three receiver profiles reject
2164 missing/erased-symbol placements, refuse constant light, and reacquire
after malformed frames. These consume recovered symbols, not camera pixels.

`tests/synthetic_video.py` builds a separate scene model using analytic
square-wave exposure integration from exported C encoder segments. It creates
lossless FFV1 grayscale videos, checks the decoded raw frames byte-for-byte,
and feeds them to the C Linux adapter with explicit capture timestamps.
Every required acceptance count is asserted; an unexpected valid packet fails
the test. No logical packet bits are passed directly to the image decoder.

First video run: `.test-output/ufsook-01/result.json` (private ignored evidence).
Expanded Debug run `.test-output/ufsook-02/result.json` and optimized-adapter
run `.test-output/ufsook-release-01/result.json` each pass thirteen cases.

| Scene | Expected/observed accepted packets |
|---|---|
| Clean calibrated UFSOOK | 1/1 |
| Alternating +/-100us capture jitter | 1/1 |
| One dropped camera frame inside payload | 0/0 |
| Exposure extended to an entire frame | 0/0 |
| Saturated LED region | 0/0 |
| Constant bright ambient/source region | 0/0 |
| No optical signal | 0/0 |
| Insufficient source contrast | 0/0 |
| Second blinking source outside chosen ROI | 1/1 |
| Capture phase100us | 1/1 |
| Capture phase5ms | 1/1 |
| Transition-region capture phase3.8ms | 0/0 |
| Camera29.97fps with30fps optical/receiver calibration | 0/0 |

Selected parameters:64x64 image,8x8 ROI,30fps,120/105Hz payload carriers,
1200Hz delimiter,approximately1.667ms exposure and0.6ms initial capture phase.
Receiver timestamp tolerance500us. Source grayscale20..220; explicit calibrated
thresholds and clipping rejection. These are synthetic parameters, not measured
DPX exposure, timing, channel response, geometry or ambient-light tolerance.

## Timing player and Zephyr integration

Fourth native test suite verifies all generated UFSOOK deadlines, returned
channel masks, abort/all-off on excessive lateness, and refusal of inadequate
rate, jitter and channel capabilities. The player makes no hardware calls.
The adapter's capability declarations still require independent measurement.

Zephyr4.4.0 native_sim/native/64 built with host GCC and ran to a bounded0.1s
simulated-time stop, reporting `OCC_ZEPHYR_PASS`. The sample exercises packet
encode/decode, waveform/player initialization and recovered-symbol reception;
it has no physical LED/camera bindings. Zephyr cache is redirected into the
new repository and compiler caching disabled (`USE_CCACHE=0`, not `OFF`).
An initial configuration announced Zephyr's default shared cache before this
was corrected; the compiled Ninja rules have no ccache command prefix.

The final changed core rebuilt and the sample again reported `OCC_ZEPHYR_PASS`.
This is a host simulation, not an MCXW236 footprint or scheduler proof.

## S2-PSK image path

Fifth CTest suite verifies disjoint ROI enforcement, normative Figure179 phase
polarity, calibrated symbol grouping, inconsistent observations and partial
symbol rejection. Figure179 and informative AnnexI use apparently opposite bit
polarity; the implemented profile follows Figure179, not a full conformance
interpretation. Direct binary-state vectors guard this choice.

`.test-output/s2psk-01/result.json` records eight lossless synthetic video cases:
clean two-source, +/-10us jitter and one lost camera frame each yield1 packet;
two lost frames in a symbol, full-carrier-period exposure, missing second source,
saturation and500us second-source row skew each yield0 packets. Each case
asserts its acceptance count and checks the FFV1 roundtrip byte-for-byte.

Parameters:64x64 image, two disjoint8x8 ROIs on the same image rows,30fps,
1000Hz carrier,10Hz line-symbol clock,100us exposure,50us capture phase and
minimum2 consistent camera samples/symbol. Calibrated epoch is0ns; this is NOT
blind optical-clock recovery. The finite Linux adapter's geometry/clock profile
is intentionally explicit. Skew and exposure are independently integrated in
the scene model; no transmitted logical bits are fed to the image classifier.

The final optimized adapters repeat all eight cases successfully in
`.test-output/s2psk-release-01/result.json`.

No evidence here says that RGB dies in one package are separable. Co-located or
overlapping source regions are refused. Real source tracking, camera timing,
exposure/gain and LED-channel proof remain separately authorized hardware work.

## C-OOK rolling-shutter path

Sixth native test suite covers the CRC8/SMBUS check vector,24 single-bit
fragment corruptions, ordering, duplicate handling, assembly timeout and raw
Manchester subpacket reception. Seventh suite adds100000 deterministic malformed
packet/symbol iterations, invalid ROI geometry, overflow and null-argument cases.
This finite stress test is not exhaustive fuzzing or independent security review.
The subsequent eighth suite verifies the finite NDJSON status handoff, all four
observation states, repeats without progress refresh, source/session/conflicting
duplicate rejection, malformed-record bounds and explicit unauthenticated,
non-authoritative output. See LINUX-STATUS-HANDOFF.md; no camera/Briar binding
was exercised.
Tracker tests reject changed kind/status/build at an unchanged sequence without
refreshing either signal or progress time.

`.test-output/cook-03/result.json` records eight FFV1 video cases after moving
row reconstruction into the OS-independent core. Clean enlarged source gives1
packet; loss of one fragment frame gives0; a lost frame followed by a complete
envelope repeat gives1. Long exposure, tiny16-row source, saturation, no signal
and wrong row readout each give0. Analytic exposure integration determines each
row's brightness, and lossless raw-image equality is checked before decoding.

The profile is64x720,8-pixel source width,4400Hz optical clock,40us row readout,
10us exposure and80us capture phase relative to a known per-frame optical epoch.
Mixed transition rows may be ignored, but every recovered cell needs at least3
agreeing confident rows. Opposite binary samples invalidate a cell. The final
partial cell in each image is dropped. Complete repeated subpackets are acquired
within images; envelope fragments assemble across frames with bounded timeout.

`cook-01` preserves a failed pre-build startup (compiler prototype mismatch
prevented the new fixture executable from being produced); that mismatch was
corrected. `cook-02` passed the initial Linux-local row algorithm, then `cook-03`
passed its portable-core extraction. No failed attempt is treated as proof.

The final optimized adapters repeat all eight cases successfully in
`.test-output/cook-release-01/result.json`.

This synthetic large source is not a tiny F1 LED image. Neither row timing nor
exposure/capture epoch is measured DPX behavior. Blind clock acquisition,
partial-subpacket fusion, outer/inner FEC and complete IEEE MAC are not claimed.

## Measured host resources

`size build-release/libocc.a` reports15120bytes summed object text, zero static
data and zero BSS for the library. This is host object text (including compiler
metadata counted by size), not linked MCU flash or complete application RAM.
Caller-owned memory on this x86_64 ABI:

| Object/buffer | Bytes |
|---|---:|
| Wave segment | 24 |
| Recovered-symbol receiver | 72 |
| Source/session tracker | 48 |
| Deadline player | 56 |
| C-OOK assembler / subpacket receiver | 48 / 32 |
| S2 symbol group | 48 |
| UFSOOK22-byte-envelope waveform | 4320 |
| S2-PSK22-byte-envelope waveform | 8640 |
| C-OOK3-byte fragment, repeated twice | 2784 |

`build-release/occ_measure` measured440ns mean packet encode+decode over50000
iterations and284ns mean UFSOOK encode over10000 iterations in one run. These
are host averages, not worst-case bounds, camera processing costs or MCXW236
cycle counts. Buffer capacities are bounded; raw frames and adapter storage
are additional. No heap/OS/driver APIs were found in core source/header scan.

## Further deployment acceptance work (not offline claims)

- Phase/fps/exposure sweeps: report successful reception region, reject uncertain
  observations without false packets, test recovery with a later repetition.
- Wider S2-PSK image stress and blind clock acquisition; calibrated source
  separation/row-skew/exposure/symbol-grouping cases now have synthetic proof.
- Wider calibrated C-OOK phase/row-timing/source-size sweeps; initial portable
  rolling-shutter row/repetition/frame-gap cases now have synthetic proof.
- Target timing-player/adapter stress and measured worst-case MCU CPU/flash/RAM;
  Release/sanitizer and Linux/Zephyr host integration now have bounded proof.
- RGB fitted mapping, electrical limits and optical separation proof; the
  source-backed assessment is in RGB-AND-INTEGRATION.md, not hardware proof.

Passing this offline plan does not prove real LED output, board electrical safety,
pressure/CAN isolation, webcam behavior, IEEE interoperability or production
readiness. Those need separately authorized deployment/evidence.

## Development deliverable audit

The development deliverable is a GPLv3 core usable from Linux/Zephyr, abstracted
from OS/RTOS/LED hardware, with three experimental camera modulation candidates
and synthetic verification, without a conformance or production claim. Evidence:

| Requirement | Inspected evidence |
|---|---|
| Separate GPLv3 development repo | Own .git on main; COPYING and GPL-3.0-only source notices |
| UFSOOK/S2-PSK/C-OOK candidate implementation | occ.c, receiver.c, spatial.c, cook.c, rolling.c; core/receiver/spatial/cook suites |
| OS/LED/camera abstraction | Public caller-owned C99 API; core has no OS/heap/device/capture calls |
| Linux use | Native library/examples, optimized and sanitizer tests; offline image adapters |
| Zephyr use | Extra module and native_sim sample; final rebuilt executable reports OCC_ZEPHYR_PASS |
| Synthetic optical/video proof | Final13/8/8 FFV1 case sets with asserted counts, plus recovered-symbol fault injection |
| RGB fit, resources and critical-task isolation outline | RGB-AND-INTEGRATION.md, host measurements above, RECOMMENDATION.md |
| Timing/framing/integrity/state limits | PROFILES.md, WIRE-FORMAT.md, observation/tracker and player tests |
| Safety/authority/licensing boundaries | No physical APIs exercised, no shared F1 edits, no secrets/payload instructions; rights and hardware gates documented |

This audit covers the development implementation and local proof of concept,
not remote publication, a hosted-CI result or a product release. Hardware
acquisition/integration and full standards interoperability remain separate next
milestones. Public CI definitions and their coverage are described in CI.md.
