# Diagnostic application envelope v1

This is a **custom application envelope**, carried by standard-derived optical
profiles. It is not the IEEE MAC frame, not a conformance declaration, and not
a secure management protocol. No commands or confidential diagnostic blob exist.

Exactly22 bytes, little-endian multi-byte fields, LSB-first bits within each byte:

| Byte offset | Width | Meaning |
|---|---|---|
| 0 | 2 | Magic ASCII OC |
| 2 | 1 | Envelope version1 |
| 3 | 1 | Kind: boot1, heartbeat2, identity3, fault4, test5 |
| 4 | 4 | Non-secret board label |
| 8 | 4 | Boot/session label |
| 12 | 2 | Sequence number, modulo65536 |
| 14 | 2 | Status code; definitions supplied by the application |
| 16 | 4 | Non-secret build label, not a cryptographic build attestation |
| 20 | 2 | CRC-16/CCITT-FALSE over bytes0..19 |

CRC parameters: polynomial0x1021, initial0xffff, no reflection, xorout0.
Unknown version/kind, bad magic, wrong checksum, incomplete frame or erasure
must not produce an accepted packet. A CRC is neither authentication nor proof
that firmware is healthy. Payload equality and sequence checks do not prevent
malicious video replay.

A tracker is explicitly configured for one expected board label and one tracked
image region. It locks to the first valid observed session; a session change
requires application-visible reset rather than silently discarding old progress.
Forward sequence movement is distance1..32767; zero is a repeat; the other half
of the16-bit space is rejected as backward/ambiguous. Sender emission must be
frequent enough to avoid this ambiguity. Repeated unchanged packets update
signal reception, never last-progress time.
Different kind/status/build at the same sequence is an inconsistent duplicate:
it is rejected without updating reception or progress. The sender must advance
sequence whenever its envelope contents change.

Signal and progress timeouts are explicit application configuration. Starting
proposal: signal timeout3 expected packet periods; progress timeout5 expected
sequence periods. These are not measured hardware values and must include
packet transmission/repetition/acquisition latency. A stopped camera means no
new observation, not a successful board health check.

## Receiver boundaries

Framing state machines accept timestamped **recovered symbols**, not arbitrary
camera pixels. UFSOOK consumes one exposure-classified camera-period sample;
MID is recognized only in its high-frequency delimiter. S2-PSK consumes one
spatial-phase symbol after camera samples have been grouped per line-symbol
period. C-OOK consumes one reconstructed rolling-shutter optical-clock sample.

Timing outside configured period +/- tolerance, nonmonotonic timestamps and
erasures reset partial packet state. A complete closing delimiter is required
for UFSOOK/S2-PSK; C-OOK requires matching start/end Ab plus complete fixed
packet length. CRC then gates output. Outage is never bridged silently.

The original whole-envelope C-OOK symbol receiver is retained as a unit-test
primitive, not the recommended camera profile. The rolling-shutter path now
uses custom short fragments:

- 3-byte payload: tag (3-bit generation,5-bit byte index), one envelope byte,
 CRC-8/SMBUS over tag/data (poly0x07, init0, no reflection/xorout).
- Manchester C-OOK subpacket: preamble011100, start Ab,24 data bits,end Ab.
- At4400Hz this is58 optical clocks, repeated twice; Ab stays identical for the
 repetitions and changes between the selected fragment packets.
- Assembly requires indices0..21 in order and one generation. Exact duplicate
 fragments are ignored without refreshing assembly progress; loss, conflicting
 duplicates, bad CRC8 or timeout aborts. Final envelope CRC16 still gates output.

This custom fragmentation/checksum layer is not IEEE MAC or standard FEC.
Generation wraps after8 and is not replay protection. The initial calibrated
image path requires enough source rows for complete repeated subpackets;
partial-subpacket fusion and blind optical-clock acquisition are not implemented.
Deployment distance, real camera timing and suitability for the fitted LED
remain unproved.
