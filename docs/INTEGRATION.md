# Consuming light-comms-core

The library remains allocation-free C99 with caller-owned buffers. These build
integrations do not select LED/camera hardware, start capture, schedule a task,
flash firmware or establish safe current/timing. GPL-3.0-only applies to linking
and distribution; the module/recipe is not a licence exception. Review product
source/distribution obligations before adoption.

## Zephyr component

Add this project to a west manifest (pin an immutable reviewed commit), then
run normal workspace update/module discovery. Example manifest entry:

```yaml
projects:
  - name: light-comms-core
    url: https://github.com/active-esl/light-comms-core
    revision: <reviewed-packaging-commit-sha>
    path: modules/lib/light-comms-core
```

Replace the placeholder with the published packaging commit, not `main` for a
reproducible build. Alternatively, append the local checkout to
`ZEPHYR_EXTRA_MODULES` before `find_package(Zephyr ...)` in the application.
No edits to the F1 workspace are needed in this repository.

The module name is `light_comms`; the existing build-library name
`ieee802157_core` is retained for compatibility. The module is **off by default**.
Enable `CONFIG_LIGHT_COMMS=y` in `prj.conf`; optional source groups default on
when enabled. A small custom slow-colour transmitter/receiver configuration is:

```ini
CONFIG_LIGHT_COMMS=y
CONFIG_LIGHT_COMMS_PLAYER=y
CONFIG_LIGHT_COMMS_SLOW_COLOUR=y
CONFIG_LIGHT_COMMS_RECEIVER=n
CONFIG_LIGHT_COMMS_IMAGE=n
CONFIG_LIGHT_COMMS_SPATIAL=n
CONFIG_LIGHT_COMMS_COOK=n
CONFIG_LIGHT_COMMS_ROLLING=n
```

Include `<occ/colour.h>`, `<occ/player.h>` and `<occ/packet.h>`. Keep diagnostics
in a bounded mailbox; the application owns LED/timer adapters and scheduling.
No driver, timer thread, current setting or board reservation is implicit.
`LIGHT_COMMS_RECEIVER` is the recovered **binary** receiver; slow-colour has its
own receiver in `LIGHT_COMMS_SLOW_COLOUR`. Spatial requires IMAGE; rolling
requires IMAGE and COOK. Disabled groups have no implementation linked, although
headers remain available when the module is enabled. Waveform/CRC/packet helpers
in `occ.c`/`packet.c` are always included while enabled; Kconfig does not remove
individual functions within these compilation units. Linker garbage collection
is a separate toolchain setting, not a claim of zero size.

Tests cover the full hardware-free sample, module disabled, and slow-colour/player
subset with generated source-selection checks. Other arbitrary combinations and
MCXW236 firmware/timing remain unproved.

## Linux CMake package

```sh
cmake -S . -B build -DBUILD_TESTING=OFF -DOCC_BUILD_EXAMPLES=OFF \
  -DCMAKE_INSTALL_PREFIX=/chosen/prefix
cmake --build build
cmake --install build
```

Use an appropriate owned prefix, not an unapproved system install. Consumers:

```cmake
find_package(light_comms_core 0.1 CONFIG REQUIRED)
target_link_libraries(my_application PRIVATE light_comms::occ)
```

The install exports `libocc.a`, public `occ/` headers, CMake package metadata
and COPYING. Set `OCC_BUILD_EXAMPLES=ON` to install the finite Linux CLI tools;
these fixture/raw-frame tools are not a camera capture service. Python/FFmpeg
synthetic generators are test dependencies, not core runtime dependencies.

## Yocto recipe/layer

Add `packaging/yocto/meta-light-comms` from this checkout to BBLAYERS, or place
the recipe in an already-owned layer. The initial declared series is
**scarthgap**, with OE-Core only. Declaration is an intended compatibility target,
not observed BitBake compatibility. Other series need explicit validation.

The recipe pins published source
`dc31e3ac4ff03081b2fbddcd81e7f5a5a52ad483` over HTTPS, never AUTOREV. That
source contains the slow-colour core but predates upstream install switches.
The recipe builds explicit CMake targets and installs headers/archive itself
for this exact pin; it does not pretend to consume the later package export.
Update SRCREV and recipe together when adopting a newer reviewed release.

Standard packages provide `light-comms-core-dev` headers and
`light-comms-core-staticdev` archive for linked applications/SDKs. A library-only
recipe may have no runtime `${PN}` package; -dev therefore has no dependency on
an empty runtime package. Include the staticdev package in an SDK when needed
(for example using the distribution's `staticdev-pkgs` SDK image feature).
For optional command-line tools:

```bitbake
PACKAGECONFIG:append:pn-light-comms-core = " tools"
IMAGE_INSTALL:append = " light-comms-core-tools"
```

An application recipe should use `DEPENDS += "light-comms-core"` and include
`occ/` headers, linking `-locc` (or explicit CMake find_path/find_library). The
pinned recipe does not install CMake package exports or pkg-config metadata.
Native/nativesdk variants are declared via BBCLASSEXTEND; they need their own
build evidence. No Python/OpenCV dependency or camera service is required.

**Evidence limitation:** recipe pin, licence checksum, paths and target choices
are statically reviewed; no BitBake fetch/parse/compile/package/QA/SDK proof is
claimed. Run those checks only on a separately approved remote Yocto executor
with an exact branch, isolated workspace, capacity, host-lock and bounded budget.
No local-workstation BitBake, target deployment or production image adoption.
Acceptance should include target and native builds, `package_qa`, inspection of
dev/staticdev/tools ownership, an application linking through the recipe sysroot,
and tools-off/tools-on configurations. Capture exact OE revision and artifact
licence/source provenance with results.
