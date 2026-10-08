# Development recommendation and bounded next experiment

## Architecture and selection

Use the portable C99 core with thin independent LED/timer and camera adapters.
Start with one proved RGB channel and calibrated UFSOOK as the simplest
single-source experiment. At30fps the176-bit envelope takes about12seconds
including delimiters: suitable for slow diagnostic snapshots, not prompt fault
alerts. Repeat envelopes; keep immediate human-visible faults independent.
S2-PSK takes36seconds at the selected5bps and needs two disjoint sources; RGB
dies do not establish that fit. C-OOK could shorten transmission but currently
needs a large rolling-shutter source image and known optical epoch. Its fixture
assigns one fragment to each camera frame; this is calibrated testing, NOT proof
of an asynchronous continuously transmitting board/camera link.

Keep all three as selectable development profiles, not a negotiated production
stack. The custom22-byte application envelope is described in WIRE-FORMAT.md;
IEEE-derived modulation is described in PROFILES.md. None is claimed conformant.
Boot, heartbeat, build/board labels, fault codes and test outcomes fit this
envelope. They are reports of what a transmitter says, not verified health or
firmware identity. Use per-source ROI tracking and separate state per board;
ambiguous/overlapping sources must produce uncertainty, not a guessed identity.

For Briar, a separately powered camera/host capture path should continue when
the board's serial/RTT/network fails. A finite Linux capture adapter can pass
frames and actual monotonic exposure timestamps to the offline decoder contract.
Do not obtain capture through the board itself. The camera still shares its own
host/power/USB failure modes; optical telemetry does not replace secure access.

Explicit states are valid observation, stale progress despite repeated packets,
uncertain signal/decoding, and no signal after timeout. Validate version, length,
checksum, source/session and sequence before updating state. Do not refresh
progress on repeats, conflicting duplicates, dropped partial packets or old
video. Suggested timeouts are3 packet periods for signal and5 sequence periods
for progress, accounting for these long transmissions and expected repetitions.

## Reuse and alternatives

The existing bounded trial of [OKATEM/OCC](https://github.com/OKATEM/OCC) pinned
`0a24b09d30db19be0828300d0fb6250b6d61b5cb` found a GPLv3 Android/OpenCV demo,
not a portable Linux/Zephyr implementation. Its fixed threshold/transition-table
decoder silently bridged gaps and lacked packet integrity/source/freshness
gates. Retain as research evidence; no source was copied into this core. The
prior trial/source hash is recorded in the F1 OPTICAL-TELEMETRY.md, not retested
or misrepresented as independent interoperability proof here.

IEEE802.15.7-2018 supplies useful camera modulation candidates. Full MAC/FEC,
blind acquisition and independent transmitter/receiver interoperability remain
outside the tested profiles. CSK/colour camera modes require a separate colour
calibration implementation. Simple low-rate Manchester OOK or an RFC1662-derived
envelope are fallback experiments if webcam timing defeats these candidates;
neither would become IEEE-conformant by borrowing a checksum. QR markers help
static board/ROI association but do not report changing firmware status.

GPL-3.0-only is intentional. Review linking/distribution/source obligations for
Linux and Zephyr products before adoption, and obtain appropriate advice about
IEEE/patent rights. The licensed standard is not redistributable merely because
the implementation is GPL. No patent clearance is claimed.

## Firmware and human fallback

Publish a small diagnostic snapshot into a bounded mailbox; acquisition/CAN
paths must never wait for optical transmission. Encoding is allocation-free;
the adapter owns fixed buffers and independently measured scheduling capacity.
Prefer a faithful autonomous waveform engine over high-frequency I2C writes.
Abort late schedules, request off, and report failure to emit; never stretch
symbols silently. Preserve critical task priority and measure bus occupancy.

The core specifies logical binary levels and50% carrier duty, not LED current.
Safe current is the minimum of fitted LED, LP5811, resistor/rail/thermal limits
with engineering margin, verified by bring-up. Account for driver peak current,
PWM, simultaneous colours and average duty; no numeric current ceiling is
asserted without that evidence. Never infer permission from a DTS current value.
Provide a slow, documented human status pattern outside telemetry windows and
an immediate fault indication. Avoid intrusive flashing, unsafe brightness or
colour-only meanings; pattern/count should remain interpretable without hue.

## Bounded acceptance gates

Offline gates already exercised: seven native suites in three build types,
176 single-bit envelope corruptions,2164 dropped/erased-symbol placements,
100000 finite malformed-input iterations,13 UFSOOK,8 S2-PSK and8 C-OOK video
cases, plus Zephyr native simulation. Synthetic exposure integration is separate
from C encoder/demapper logic; lossless video roundtrip is asserted. These are
regression cases, not a statistical field false-positive rate or independent
IEEE interoperability test. Wider phase/row/angle/colour sweeps remain useful.

Proposed next hardware experiment, only after signed coverage and bring-up:

1. Establish as-built mapping, safe current/duty, actual LED edge timing and
   camera formats/exposure/row timing. Refuse a profile if measured capabilities
   exceed configured interval/jitter limits. No address sweep/flash loop.
2. One reserved board, one camera, one colour, fixed geometry; at most3 finite
   capture scenarios, each at most120seconds, no audio or unrelated scene.
   For UFSOOK clean capture, require at least8 consecutive expected envelopes
   within120seconds, zero wrong identities/CRC-valid altered payloads, and
   every transmitted sequence accounted for as accepted or explicitly rejected.
3. In each finite impairment scenario, inject a known frame gap/obstruction,
   require no corrupted packet acceptance, uncertainty or no-signal by its
   configured deadline, and recovery within2 complete repeated envelopes.
   Stop/revert on saturation, unsafe current, driver error or critical-task slip.
4. Before shared-firmware integration, agree exact CAN/pressure requirements
   with their owners. Require zero added missed acquisition/CAN deadlines and
   no new errors over the same bounded baseline/load scenario. Record CPU,
   RAM, bus occupancy and timing; no universal MCU budget is inferred from host
   averages. Separate boards/angle/ambient/colour tests follow only if authorized.

Camera coverage must bind DPX host and exact camera device identity, capture
user, frame format, permitted exposure/gain/white-balance changes and restoration,
private board-only ROI/storage, duration/output bounds and no microphone.
Development coverage must separately bind root subject, exact board/reservation,
candidate source/artifact, permitted build/flash/LED operations, safe ceilings,
shared integration paths, CAN/pressure regressions and rollback. Coordinate with
Bluetooth/LP5811 bring-up before shared edits; its reports cannot grant authority.
Neither the repository, its CI nor a bring-up report provides operational
authority. Exact authorization must cover the independently reviewed hardware
and camera actions; publication is not permission to operate them.

Checksums detect corruption, not spoofing. Authenticated messages and anti-replay
would be needed before unattended high-impact decisions depend on the sender;
that entails separate secure key provisioning and protocol work. Even an
authenticated diagnostic must never carry authority or executable instructions.
No credentials, keys, confidential traces or arbitrary dumps belong in this
channel.
