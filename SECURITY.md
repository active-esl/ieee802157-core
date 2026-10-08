# Security policy

Experimental development software: no supported production release or security
maintenance SLA is promised. Optical payloads and images are untrusted input.
CRC, counters and timeouts do not authenticate a source or defeat malicious
replay. No observation can authorize operations or replace secure management.

For a vulnerability, use GitHub private vulnerability reporting if enabled for
this repository. If unavailable, arrange a private report with the maintainers
through an established trusted contact; do not publish sensitive details in an
issue. For ordinary non-sensitive bugs, include a minimal synthetic reproduction,
compiler/profile details and expected versus observed behavior.

Never include secrets, private board/camera data, credentials, grants or device
backups in public reports. Report malformed-input handling, buffer/overflow
issues, unsafe uncertainty/freshness claims and integration boundary failures.
Hardware shutdown and electrical safety are external adapter responsibilities;
a software all-off request is not proof of physical shutdown after a bus failure.
