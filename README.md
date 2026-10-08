# Portable light communications core

[![Native build and tests](https://github.com/active-esl/light-comms-core/actions/workflows/native.yml/badge.svg)](https://github.com/active-esl/light-comms-core/actions/workflows/native.yml)
[![Synthetic video tests](https://github.com/active-esl/light-comms-core/actions/workflows/synthetic.yml/badge.svg)](https://github.com/active-esl/light-comms-core/actions/workflows/synthetic.yml)
[![Zephyr native simulation](https://github.com/active-esl/light-comms-core/actions/workflows/zephyr.yml/badge.svg)](https://github.com/active-esl/light-comms-core/actions/workflows/zephyr.yml)
[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-blue.svg)](COPYING)
[![C99](https://img.shields.io/badge/language-C99-blue.svg)](include/occ/occ.h)
[![Development: experimental](https://img.shields.io/badge/status-experimental-orange.svg)](docs/PROFILES.md)

GPL-3.0-only development library for light communications and LED-to-camera
telemetry experiments on Linux and Zephyr, maintained for AESL. It includes
selected IEEE 802.15.7-derived modulation experiments and a custom slow-colour
profile; the library is not limited to implementing that standard.
Public repository:
[`active-esl/light-comms-core`](https://github.com/active-esl/light-comms-core),
renamed from `ieee802157-core` without replacing its history. Stable `occ` APIs
and the existing Zephyr build-library identifier remain compatible.
Not a production stack or an IEEE-conformant implementation. Workflow badges
describe hosted repository runs, not unpublished local changes, hardware proof
or standard conformance.

## Boundary

The allocation-free C99 core accepts explicit timing profiles and logical channel
identifiers. It generates waveform segments and provides elementary demappers,
CRC-16/CCITT-FALSE and observation timeout classification. No core code calls an
OS, camera API, LED driver, I2C bus, GPIO, timer, or heap allocator.

The application owns buffers, time origin, source tracking, pixel classification,
exposure integration, protocol acquisition, payload schema, and scheduling.
A driver must report achievable rate/jitter/channel capabilities; it must reject
profiles it cannot actually emit. Looking up a waveform in software is not proof
that an LP5811 or RTOS can reproduce it.

## Development implementation

Implemented initial primitives:

- UFSOOK single-channel profile: frequency pair derived from camera fps,
  four-video-period opening/closing delimiters and two-frame payload bit duration.
- S2-PSK two-spatial-source profile: phase relationships, half-rate line coding
  and opening/closing preamble.
- C-OOK adapted Manchester sub-packets with a single asynchronous bit and
  configurable repetition; no FEC or complete PPDU/MAC yet.
- Pure waveform level lookup and elementary confident-sample demapping.
- Application CRC and valid/stale/uncertain/no-signal timeout primitives.
- Fixed 22-byte diagnostic envelope, bounded symbol-stream framing receivers,
  integrity checks and source/session/sequence progress tracking.
- Native Linux waveform exporter and tested Zephyr native-simulator sample.
- Pure deadline player with declared timing/channel capabilities and fail-closed
  all-off output on excessive lateness; no physical driver binding.
- Portable calibrated grayscale ROI classifier and finite offline Linux video
  adapter; thirteen lossless synthetic UFSOOK video cases pass.
- Disjoint-ROI S2-PSK image demapper and calibrated multi-frame symbol grouping;
  eight lossless synthetic two-source video cases pass.
- Portable calibrated rolling-shutter C-OOK row reconstruction, checked short
  fragments and envelope assembly; eight synthetic rolling-source videos pass.
- Custom slow-colour encoder, RGB classifier and framed receiver for one LED
  package with four distinguishable colours:125ms symbols,13.75s/envelope.
  Fifteen synthetic video cases pass with configured-period offline phase
  acquisition, without a supplied optical epoch. Live capture and arbitrary-rate
  clock recovery are unproved. This profile is not IEEE CSK or UFSOOK.

Ten suites (eight native plus two optional Python adapter fixtures)
pass in Debug, Release and ASan/UBSan builds, including
100000 deterministic malformed-input iterations. Host resource measurements and
RGB/integration assessment are documented in [test evidence](docs/TESTING.md)
and [RGB fit](docs/RGB-AND-INTEGRATION.md).
Arbitrary-rate optical-clock acquisition is not claimed by these profiles
and remains a documented deployment limitation.
Synthetic tests are not real-camera proof. C-OOK partial-sub-packet fusion
remains unimplemented.
See the [architecture recommendation and next test gates](docs/RECOMMENDATION.md).
The [local readiness checkpoint](docs/LOCAL-READINESS.md) separates measured
fixture results from the unproved hardware and camera requirements.
The next hardware milestone has a [public integration assessment](docs/HARDWARE-INTEGRATION.md):
the current manual LP5811 API cannot emit the selected carriers at its configured
bus rate; an independently proved waveform path is required before deployment.
For the initial fitted LED, the selected local experiment is the
[custom slow-colour profile](docs/COLOUR-PROFILE.md), not an auxiliary emitter.
It reduces switching rate but does not establish safe current, physical timing
or independent camera acquisition. Human fallback and critical-task isolation
remain integration gates.
The [finite Linux status handoff](docs/LINUX-STATUS-HANDOFF.md) emits explicit
valid/stale/uncertain/no-signal NDJSON from validated envelopes and host events;
it does not itself capture a camera or bind Briar.

## Build (installed tools only)

Requires CMake3.20+, a C99 compiler and a build tool (examples use Ninja).
Python3 enables the eighth status-handoff fixture. Synthetic video tests also
require NumPy and FFmpeg. No runtime package dependency is required by the core.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
build/occ_wave_csv
```

Sanitizer build with GCC or Clang:

```sh
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DOCC_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel 2
ctest --test-dir build-asan --output-on-failure
```

Offline synthetic-video test (uses installed NumPy/FFmpeg; new output directory
per meaningful attempt, no camera access):

```sh
python3 tests/synthetic_video.py --build build --output .test-output/ufsook-01
python3 tests/synthetic_spatial_video.py --build build --output .test-output/s2psk-01
python3 tests/synthetic_rolling_video.py --build build --output .test-output/cook-03
python3 tests/synthetic_colour_video.py --build build --output .test-output/colour-new
```

See [test evidence and limits](docs/TESTING.md). The Linux adapter reads raw
grayscale frames from standard input and explicit monotonic timestamps from a
file. Camera ownership, controls, timestamps and ROI tracking are outside it.

Zephyr can consume this as an extra module through its normal
`ZEPHYR_EXTRA_MODULES` or west mechanism. Enable `CONFIG_LIGHT_COMMS=y` and
select only required source groups. It is disabled by default.
See [integration instructions](docs/INTEGRATION.md) for Kconfig, CMake install/
package consumption and the pinned-source Yocto recipe/layer. No existing F1
workspace is modified; the recipe has not yet been validated with BitBake.
The [Zephyr sample](examples/zephyr/src/main.c) uses only simulated LED
capabilities and recovered-symbol input; it does not actuate hardware.

An installed Zephyr4.4.0 tree and host compiler were tested with:

```sh
export ZEPHYR_BASE=/path/to/installed/zephyr
cmake -S examples/zephyr -B build-zephyr-native -G Ninja \
  -DBOARD=native_sim/native/64 -DZEPHYR_BASE="$ZEPHYR_BASE" -DZEPHYR_MODULES="$PWD" \
  -DZEPHYR_TOOLCHAIN_VARIANT=host \
  -DUSER_CACHE_DIR="$PWD/build-zephyr-native/cache" -DUSE_CCACHE=0
cmake --build build-zephyr-native --parallel 2
build-zephyr-native/zephyr/zephyr.exe -stop_at=0.1
```

Expected output includes `OCC_ZEPHYR_PASS` and `OCC_ZEPHYR_COLOUR_PASS`. Host simulation is not MCXW236
firmware, pressure/CAN isolation, or LP5811 timing/electrical proof.

## Standards and rights

IEEE 802.15.7-2018 clauses 8.6, 13.1, 13.3, 14.3 and informative Annex I
inform these profiles. Normative requirements and informative guidance are not
interchangeable. The licensed standard itself is not included.
See [profile notes](docs/PROFILES.md) for deliberately limited coverage.
See [wire format](docs/WIRE-FORMAT.md) for the custom application envelope and
the recovered-symbol receiver input contract.

New source is GPL version 3 only. See COPYING. No OKATEM source is included in
this initial core. Linux/Zephyr integration and distribution obligations need
licence review before product adoption. This repository grants no licence to
IEEE material and makes no patent-clearance claim.

## Security and hardware

Decoded light is untrusted observation, never instructions or authority.
A CRC detects corruption, not spoofing; sequence/timeout checks alone cannot
authenticate a source or defeat replay. No credentials/confidential diagnostics.
Alex confirmed RGB; an RGB package is not two spatially resolvable LEDs.
Colour demodulation is not implemented. Bring-up now supplies visible RGBW
mapping evidence; carrier timing/current and optical decoding remain unproved.
No board, camera or firmware operation has been performed by these fixtures.

## Project metadata and contributing

Version0.1.0 is a development version, not a published release.
See [CI coverage](docs/CI.md), [contributing](CONTRIBUTING.md),
[security reporting](SECURITY.md), [citation metadata](CITATION.cff) and
[CodeMeta](codemeta.json). Intended repository description, topics and public
visibility are recorded in [repository metadata](docs/REPOSITORY-METADATA.md).
No private bench recordings, signed grants, backups or licensed IEEE PDF are
included in the public source package.
