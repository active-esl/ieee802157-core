# SPDX-License-Identifier: GPL-3.0-only
"""Static consistency checks only: NOT BitBake parsing, packaging or QA."""
import hashlib
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[1]
layer = root / "packaging/yocto/meta-light-comms"
recipe = (layer / "recipes-light-comms/light-comms-core/light-comms-core_0.1.0.bb").read_text()
checksum = hashlib.md5((root / "COPYING").read_bytes()).hexdigest()
required = (
    'LICENSE = "GPL-3.0-only"',
    f'file://COPYING;md5={checksum}',
    'SRCREV = "dc31e3ac4ff03081b2fbddcd81e7f5a5a52ad483"',
    'protocol=https;branch=main', 'inherit cmake',
    '-DBUILD_TESTING=OFF', '-DOCC_ENABLE_SANITIZERS=OFF',
    'RDEPENDS:${PN}-dev = ""', 'BBCLASSEXTEND = "native nativesdk"',
)
for item in required:
    if item not in recipe:
        raise SystemExit(f"missing recipe binding: {item}")
if "AUTOREV" in recipe:
    raise SystemExit("unpinned recipe source")
tools = re.search(r'^LIGHT_COMMS_TOOLS = "([^"]+)"$', recipe, re.M)
if not tools:
    raise SystemExit("missing explicit tool targets")
cmake = (root / "CMakeLists.txt").read_text()
for tool in tools.group(1).split():
    if f"add_executable({tool} " not in cmake:
        raise SystemExit(f"unknown tool target: {tool}")
if 'LAYERSERIES_COMPAT_lightcomms = "scarthgap"' not in (layer / "conf/layer.conf").read_text():
    raise SystemExit("missing initial series declaration")
print("YOCTO_METADATA_STATIC_PASS (not BitBake proof)")
