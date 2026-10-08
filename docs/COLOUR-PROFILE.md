# Custom slow-colour development profile

This is a local synthetic-first experiment for the existing F1 LED package,
not IEEE 802.15.7 CSK/UFSOOK, a production protocol or measured hardware proof.
It supplements the IEEE-derived profiles rather than renaming one of them.

## Format and timing

Four distinct one-bit logical channel masks represent symbols0..3, one active
at a time;4 means dark. Mapping them to fitted outputs requires independently
verified as-built identity and permission. No extra emitter is required.

| Field | Symbols |
|---|---|
| Synchronisation | 0,1,0,2,3,1,2,0,3,2,1,3,0,2,1,0 |
| Payload | Existing22-byte [envelope](WIRE-FORMAT.md),88 dibits, least-significant dibit first in each byte |
| Trailer | 3,1,3,2 |
| Guard | Dark,dark |

Each symbol lasts125ms:110 symbols take13.75s, eight consecutive envelopes110s.
Logical output is continuously active during coloured symbols, not50% carrier
PWM.108/110 symbols are active,98.18% aggregate package duty. Driver PWM/current,
per-die duty, fault indication and thermal limits need separate measurement.
No numeric safe current is established here.

## Decoder boundary

`occ_colour_receiver_push` accepts exactly one independently recovered,
timestamped symbol per period. Gaps, nonmonotonic timestamps, erasures, invalid
symbols, closing/guard mismatch and bad packet integrity abort partial state.
CRC detects corruption, not spoofing. Pin source/session/sequence through the
tracker; duplicates do not advance progress. Keep valid, stale, uncertain and
no-signal outcomes distinct. Do not interpret decoded content as instructions.

The RGB classifier subtracts an ROI-specific dark baseline and compares
normalised hue with four calibrated prototypes. Saturation, insufficient colour
separation or ambiguous nearest matches are rejected. Calibration must be
measured from the selected camera/source, never assumed from nominal LED colours.

The Linux fixture has synthetic calibration only. The finite offline phase
search uses a fixed ROI and configured125ms period, trying24 phases without
receiving a transmitter epoch or expected payload. CRC-valid candidates with
the same identity/session/sequence but conflicting payloads are rejected as
uncertainty; agreeing candidates are deduplicated, not treated as independent
authentication. Camera phase, two unknown transmitter epochs and small drift
are tested. Arbitrary-rate clock recovery, streaming/live capture, row timing
and real white balance are not implemented or proved. Multiple-source ROI acquisition and
overlap rejection remain camera-adapter work. Never configure a camera from
these synthetic constants.

## Local proof and remaining gates

Native tests exercise eight envelopes/110s, all110 single-symbol corruptions,
all110 missing-symbol positions, recovery within two whole repeated envelopes,
capacity/invalid mapping/timestamp boundaries, calibration and tracker states.
Player tests verify every virtual segment and fail-closed lateness abort;100000
deterministic malformed colour/timestamp inputs exercise memory and UB handling.
The offline CLI separately rejects numeric overflow, negative timestamps,
embedded NULs, mixed erasures, overlong lines and excess input records.
Fifteen analytic-exposure/FFV1 cases cover three camera phases, two unknown transmitter epochs, long host uptime, gain, ambient,
small drift, frame gap/recovery, saturation, obstruction, wrong board, bad CRC
and long exposure. These are finite fixtures, not field reliability statistics.
Phase-search tests reject fractional/scalar/overflow timestamps and invalid
duration types before subprocess launch. Candidate tests reject a conflicting
payload even against twenty agreeing candidates, deduplicate true repeats and
retain an unrelated valid sequence; they do not establish sender authenticity.

With installed dependencies only, run from the repository root:

```sh
ctest --test-dir build-colour-debug --output-on-failure
timeout 180 python3 -B tests/synthetic_colour_video.py \
  --build build-colour-debug --output .test-output/colour-video-new
```

Use a fresh output directory. The Zephyr sample exercises virtual colour
encode/decode and prints `OCC_ZEPHYR_COLOUR_PASS`; it does not schedule physical
edges or access an LED. Hardware timing, current/duty, DPX camera access,
independent capture, pressure/CAN isolation and human fallback remain the
[bounded integration gates](RECOMMENDATION.md#bounded-acceptance-gates).
Require independently verified action-bound camera and board grants, and
coordination with Bluetooth/LP5811 bring-up, before any such operation.
