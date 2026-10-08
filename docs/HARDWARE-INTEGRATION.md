# Hardware integration limits

The AESL bring-up evidence establishes visible OUT0red/OUT1green/OUT2blue/
OUT3white operation on its tested RGBW prototype. It does not establish measured
current, calibrated colour or the optical carrier timing needed by these profiles.
Private receipts, recordings, device identities and authority material are not
part of this public repository. Core channel bits remain logical identifiers,
not universal LP5811 pin assignments.

The currently reviewed manual LP5811 channel API performs6register writes and
13register reads per update, including package blanking/readback and fault checks.
With ordinary single-byte I2C transactions, that is at least630SCL clocks:
6.3ms at100kHz, ignoring scheduling, stretching and bus overhead. UFSOOK1200Hz
delimiter needs416.667us half-periods;120/105Hz payload needs4.167/4.762ms.
Thus the existing API at its configured rate cannot emit the selected profile.
Even400kHz gives a1.575ms lower bound, insufficient for the delimiter.

TI SNVSCC4C sections7.3.3/7.3.4/Table7-3 document12/24kHz brightness PWM
and autonomous animation timing with a90ms minimum nonzero sloper/pause.
Those controls do not directly supply the selected105/120/1200Hz waveforms.
This rules out simply connecting the existing API or animation options, not
every possible separately engineered switching path. Do not remove fault or
readback safeguards to obtain a favorable benchmark.

A real adapter needs reviewed high-resolution timing, independently measured
capabilities, source-current/duty/thermal limits, critical-task/bus isolation and
physical shutdown evidence. Reject profiles that cannot be faithfully emitted.
Co-located RGBW dies are not automatically two spatially resolvable S2-PSK
sources. C-OOK needs enough rolling-shutter source rows and calibrated timing.

Coordinate with the board/driver owner before shared edits or operation. Exact
hardware/camera/candidate authorization, independent capture, current limits and
rollback are separate prerequisites. See RECOMMENDATION.md for the proposed
bounded clean/impairment/deadline tests; none is claimed complete by this library.
