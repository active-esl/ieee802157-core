# Independent decoder-to-Briar status handoff

`occ_status_stream` is a finite offline Linux adapter using the existing core
packet validator/tracker/timeouts. It emits line-buffered NDJSON observations,
not commands. It does not capture a camera, open a device, watch files, start a
timer, reset sessions, connect a network or operate a board.

Invocation: `build/occ_status_stream EXPECTED_BOARD SIGNAL_TIMEOUT_NS PROGRESS_TIMEOUT_NS`.
Input is bounded host-fixture text, one newline-terminated record:

- `MONOTONIC_NS HEX`: the existing22-byte envelope encoded as44hex characters.
- `MONOTONIC_NS uncertain`: an explicitly uncertain optical observation.
- `MONOTONIC_NS tick`: an independent host timeout event, with no optical sample.

This text is an offline integration/test contract, not another optical wire
format or a board command channel. Unknown tokens, overlong records, backward
or equal timestamps, extra fields and more than10000records terminate with
exit2. Consumer must treat malformed input/exit failure as adapter failure,
not as a successful health report. The whole workload still needs an external
timeout because a pipe could stall; this adapter does not implement background
monitoring. EOF is not a new healthy observation.

Output fields include time_ns, state (valid/stale/uncertain/no_signal), accepted,
progressed, expected_board and packet. Packet is null unless the envelope passed
CRC/source/session/sequence/duplicate checks; accepted fields are numeric board,
session, sequence, kind, status and build labels. No free-form diagnostic blob or
executable text exists. Every record explicitly says authenticated=false and
authoritative=false. "Accepted" means syntactically/integrity accepted, not a
verified identity, build attestation or healthy board.

Identical repeats update reception but not progress. Conflicting duplicates,
changed sessions, wrong board labels and backward/ambiguous sequences produce
uncertainty without altering the accepted tracker identity/progress. CRC failure
likewise produces uncertainty. Independent ticks evaluate the configured
timeouts without inventing new signal; thus repeated old video becomes stale
and a stalled capture becomes no-signal if the host continues timeout events.
Counters and checksums do not defeat malicious replay or spoofing.

Future independent camera integration must provide real monotonic capture
timestamps and validated envelopes from the image decoders; host timers, not
the board, supply timeout ticks when capture stalls. No such camera/timer/Briar
binding is installed or exercised here. Passing these fixtures does not prove
real capture continuity or board failures. Session reset remains a deliberate
application decision rather than a message-triggered automatic action.

`tests/test_status_stream.py` verifies all four states, repeated packets,
progress/stale/outage deadlines, CRC damage, wrong source/session, backward
sequence, conflicting contents, packet-null behavior, malformed records and
the10000record bound. Optional Python discovery registers this eighth CTest
suite when the already-installed interpreter is available; no dependency install.
