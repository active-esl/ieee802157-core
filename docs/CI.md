# CI coverage and provenance

Three GitHub Actions workflows are prepared for the public active-esl repository:

| Workflow | Coverage |
|---|---|
| native.yml | GCC/Clang Debug and Release, plus both compiler sanitizer builds; seven native suites and installed-Python status handoff |
| synthetic.yml | Separate UFSOOK13, S2-PSK8 and C-OOK8 analytic exposure/FFV1/decoder case sets |
| zephyr.yml | Pinned Zephyr4.4.0 host native_sim build and finite0.1s simulation requiring OCC_ZEPHYR_PASS |

All target [self-hosted, Linux, X64, aesl, esl-proxmox], the AESL Proxmox runner,
with an ubuntu:24.04 job container, read-only contents permission, no persisted
checkout credential, a5minute whole-job cutoff and bounded test execution.
Only pushes to this repository's main branch trigger execution. PR and manual
triggers are deliberately absent: unreviewed fork code must not execute on the
persistent runner. Review a contribution before integrating it into main.
Containers limit dependency pollution but are not a strong isolation boundary
from the runner; do not expose host secrets or devices or relax its controls.
Runner registration, public-repository access and existing cross-organisation
job hooks/host lock must be verified before the first push. Matrix concurrency
is one per workflow; the host lock must serialize across workflows/organisations.
They request no deployment, hardware, camera/microphone, remote bench, secret
or publication credentials. No private fixture or licensed PDF is
required, copied or uploaded as an artifact. Build output remains ephemeral.

Checkout is pinned tod23441a48e516b6c34aea4fa41551a30e30af803, the public
actions/checkout v6 ref observed8October2026; its immutable action.yml declares
Node24. The deprecated v4 runtime was corrected before publication.
Zephyr4.4.0 peeled commit is
684c9e8f32e4373a21098559f748f06915f950c9, matching the installed tree used in
local simulation. These pins identify source, not a security-review assertion.
Review and update pins deliberately rather than silently following mutable tags.

Ubuntu package versions and Zephyr's transitive Python requirement resolution
are not fully locked: these workflows are development regression CI, not a
bit-reproducible release pipeline or artifact-derived SBOM. That limitation must
be resolved before a controlled product release. Jobs install dependencies only
inside disposable job containers, never through host sudo; local testing uses
already-installed tools. The container image tag is not digest-pinned yet.

The sanitizer option OCC_ENABLE_SANITIZERS instruments the core and linked test/
adapter targets with address/undefined checks and frame pointers under GCC or
Clang. Python assertions are enabled in native/video CI. Library code has no
dependency on Python, FFmpeg, GitHub Actions or Zephyr itself.

Badges point to these actual workflow filenames and will acquire status only
after publication and runs. Local syntax/build validation is not a hosted-CI
pass. Neither hosted CI nor synthetic video proves real-camera behavior,
MCXW236 resource/deadline headroom, safe LED output or IEEE conformance.

Local preparation checks: workflow YAML/trigger/permission/pin/badge structure,
CFF/CodeMeta version/licence/destination consistency and fresh GCC sanitizer
build with all eight suites pass. A fresh Zephyr4.4.0 native_sim build with
external workspace modules disabled also reports OCC_ZEPHYR_PASS, verifying
that the sample only needs the pinned base tree and this extra module.
Clang and hosted job execution have not been
run during this preparation. Full-schema/actionlint validation is not claimed.

## Initial ESL-runner evidence and setup corrections

Initial commit a162622 ran on esl-proxmox-runner. Five native matrix jobs passed;
Clang's sanitizer job failed linking because its runtime archives were absent.
All three synthetic jobs failed the C compiler smoke test because Scrt1.o/crti.o
were absent. Zephyr failed package discovery because the sample reads the
ZEPHYR_BASE environment hint, not just the cache variable supplied by CI.

The correction installs build-essential (including libc development/startup
files), installs libclang-rt-18-dev alongside Ubuntu24.04's Clang18, and exports
ZEPHYR_BASE before configuring the Zephyr sample. Dependency installation now
precedes checkout so Git/certificates are available and checkout can perform
its normal workspace handling on the persistent runner. All package changes
remain inside the job container; host dependencies/permissions are unchanged.
These diagnosed corrections are not proof of a successful corrected run.
