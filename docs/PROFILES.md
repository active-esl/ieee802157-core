# Development profile notes

These notes paraphrase requirements from Alex's locally licensed IEEE
802.15.7-2018 standard; do not redistribute that PDF or large extracted passages.
No full MAC/PHY conformance is claimed.

| Candidate | Reference | Implemented initial profile | Unproved/remaining |
|---|---|---|---|
| UFSOOK | 8.6.1.2.1, 8.6.5.2.1, 13.1 | Two camera periods/bit; N*fps and (N-1/2)*fps; delimiter >1kHz then mark, two periods each | Streaming acquisition, clock/exposure tolerance, optional CRC-3, spatial/temporal FEC and MIMO |
| S2-PSK | 8.6.1.2.4, 13.3 | Two spatial channels, Figure179 phase polarity, half-rate line code, 1111 preamble | Exposure/row skew, symbol clock acquisition and FEC |
| C-OOK | 14.3 | Manchester sub-packet preamble, Ab/data/Ab, repetition, calibrated row decoding and custom checked fragments | Full PPDU out-of-band preamble, 4B6B, inner/outer FEC, blind acquisition and asynchronous partial-packet fusion |

UFSOOK default example: camera 30fps, N=4 -> space120Hz and mark105Hz,
delimiter1200Hz. Bit duration approx66.667ms before repetition overhead.
These are selected experiment parameters, not measured DPX characteristics.
Camera fps is a profile input, not inferred from an advertised USB format.
Integer nanosecond segment rounding must be included in phase-error tests.

S2-PSK default experiment: line clock10Hz, carrier1000Hz, two separately
resolvable channels. Half-rate coding -> 5 data bits/s before framing.
Encoder carrier must be an integer multiple of the line clock in this profile.
Figure179 maps in-phase signals to1 and inverse-phase signals to0. The initial
core followed the opposite polarity implied by the informative AnnexI XOR
description; visual review found the discrepancy, and the encoder/demapper now
follow Figure179 using XNOR on confidently classified source states. This
source ambiguity is explicitly not resolved by a conformance claim. Tests
include direct phase-state vectors independent of the packet round trip.

The carrier and camera exposure must be evaluated together; long exposure can
destroy phase information. RGB R/G/B dies in one package cannot be assumed
resolvable merely because the driver can control them independently.

C-OOK default experiment: optical clock2200Hz, Manchester0->01 and1->10,
one Ab bit held constant across repeated sub-packets. This explicit Manchester
convention is currently an adapted-profile choice, not a conformance claim.
Data bits are serialized as caller-supplied bits; byte/field serialization is
implemented separately by the application envelope (LSB first). C-OOK needs a sufficiently large source image for
rolling-shutter rows to reveal the waveform; full-frame brightness is not enough.

Recommended offline C-OOK experiment now uses4400Hz and short24-bit custom
fragments carrying tag/data/CRC8, plus start/end Ab and the Manchester preamble.
Each subpacket has58 optical clocks (~13.18ms), repeated twice before the next
fragment. Twenty-two fragments reconstruct one checked diagnostic envelope.
The selected synthetic row profile has40us readout,10us exposure and a large
720-row source, with a calibrated per-frame optical epoch. This is not proof of
DPX parameters or applicability to a tiny PCB LED. No partial-subpacket fusion
is implemented; missing fragments require a later complete envelope repetition.

The generic CRC primitive is CRC-16/CCITT-FALSE, check vector
123456789 -> 0x29b1. It is NOT IEEE UFSOOK's optional CRC-3.
Application framing will name its own checksum explicitly.

State classification intentionally separates optical signal from valid
diagnostic progress. A repeated valid old frame may keep signal present without
refreshing progress. The caller must establish progress from an explicitly
checked session/sequence scheme. These checks remain unauthenticated.
