# RGB fit and adapter integration

## Evidence boundary

Read-only review on8October2026 found the F1 devicetree declaring one LP5811 child
called sensor-rgbw, logical index0, colour mapping R/G/B/W, address0x50 and a
development current setting. The live-development harness also asserts four
outputs. These are source configuration/intended mapping, NOT independent
as-built confirmation of a fourth white emitter, channel connectivity or light.
No source/configuration was copied into this GPL core or edited in that lane.

Subsequent bring-up evidence establishes the same physical LED visibly emitting
red, green, blue and white in the commanded single-output order, then off.
Observed mapping is OUT0red/OUT1green/OUT2blue/OUT3white for that tested board.
This closes the earlier fourth-white-emitter uncertainty, not measured current,
calibrated colour, resolvable individual dies or optical carrier-rate proof.
Private images, device identities and authority material are excluded from the
public source package. See HARDWARE-INTEGRATION.md for the engineering limits.

The portable core therefore treats channel bits as logical identifiers. It does
not hard-code LP5811 address, four fitted channels, current, PWM, boost state,
I2C transfer rate or colour/pin assignments. The adapter must establish those
from coordinated as-built bring-up before any independently authorized actuation.

## Useful RGB roles

| Use | Fit for confirmed RGB package | Remaining proof |
|---|---|---|
| One selected colour carrying UFSOOK | Natural baseline; brightness rather than hue conveys data | Visible channel, safe current, switch-rate/jitter and camera exposure |
| Same temporal message on colours in separate runs | Compare contrast and support redundant observation | White balance, spectral sensitivity, cross-talk and exposure saturation |
| Human-readable colour status with temporal fallback | Useful alongside a diagnostic envelope | Approved state meanings and no conflict with telemetry scheduling |
| Spatial S2-PSK using RGB dies | Not assumed suitable; dies may be optically co-located/unresolvable | Two independently visible disjoint image regions, or separate LED sources |
| C-OOK on an RGB channel | Colour is optional; camera needs enough rolling rows | Lens/diffuser/source image geometry and calibrated row timing |
| PHY III CSK / PHY VI colour modes | Standard alternatives, not implemented by this profile set | Different modulation/geometry/calibration requirements and justified fit |

No colour message authenticates board identity. RGB capability does not grant
operation authority or prove that a colour coding profile is robust. Colour
channels can saturate or be altered by auto white balance and ambient sources.
The library's current grayscale adapters deliberately do not claim colour
demodulation. Colour-coded IEEE profiles are alternatives, not silently replaced
by spatial S2-PSK.

## OS and hardware boundary

- Core: C99, caller-owned state/buffers, pure modulation, framing, checksums,
  image processing and deadline planning. No heap, OS or hardware API.
- Linux: finite offline frame/timestamp adapters and synthetic video tools.
  Real capture is a separate component requiring exact camera access coverage.
- Zephyr: extra-module packaging and native-simulator integration sample.
  Real timers/LED drivers are external adapters; millisecond uptime is not
  sufficient carrier-edge timing merely because the sample uses it as an epoch.
- LED adapter: verified logical-to-fitted mapping and independently measured
  interval/jitter capabilities. Must return unsupported when inadequate.
  Core lateness abort returns all-off; physical off completion/readback is the
  adapter's responsibility, not something the core can prove.
- Critical-task isolation: hardware waveform engine preferred where it can
  faithfully express the profile. Otherwise bounded timer/event work on an
  isolated scheduling path; no long I2C calls in pressure/CAN interrupt paths.
  Measure CPU, bus occupancy, deadline slips and memory on the actual target.
  The portable player is not permission to bit-bang a shared I2C bus at kHz rates.

## Before any hardware deployment

Keep prototype bring-up as the single owner of LP5811 identity, electrical state
and fitted light/channel proof. Coordinate before shared F1 writes or board use.
Hardware operations require independently verified exact-scope authorization;
publishing this library does not authorize operating any board or camera.

Required new verified coverage: exact subject, board identity/reservation,
candidate and rollback, source/integration paths, shared-bus/CAN/pressure safety
tests and electrical current/duty ceilings. Exact DPX camera identity, finite
capture and permitted control changes, board-only view, no audio/confidential
content, preservation/restoration and private ROI evidence must be bound too.
Do not inherit authority from the library, another lane or a diagnostic message.

No current or duty value from another lane's source is a safety authorization.
Human-visible indicators remain advisory; optical telemetry never replaces
secure management access. CRCs are not authenticity. Authenticate messages and
provision keys through a separate secure mechanism if unattended/high-impact
decisions depend on source identity; even authenticated diagnostics cannot issue
grants or operational instructions. No credentials or confidential blobs belong
in optical payloads.
