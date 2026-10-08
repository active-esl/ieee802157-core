# SPDX-License-Identifier: GPL-3.0-only
SUMMARY = "Portable experimental light communications core"
DESCRIPTION = "Hardware-independent GPLv3 C99 optical telemetry library; no IEEE conformance claim"
HOMEPAGE = "https://github.com/active-esl/light-comms-core"
LICENSE = "GPL-3.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=1ebbd3e34237af26da5dc08a4e440464"

SRC_URI = "git://github.com/active-esl/light-comms-core.git;protocol=https;branch=main"
SRCREV = "dc31e3ac4ff03081b2fbddcd81e7f5a5a52ad483"
PV = "0.1.0+git${SRCPV}"
S = "${WORKDIR}/git"

inherit cmake

EXTRA_OECMAKE = "-DBUILD_TESTING=OFF -DOCC_ENABLE_SANITIZERS=OFF"
PACKAGECONFIG ??= ""
PACKAGECONFIG[tools] = ",,,"
LIGHT_COMMS_TOOLS = "occ_wave_csv occ_fixture_json occ_decode_gray occ_decode_colour_fixture occ_decode_spatial occ_fixture_cook occ_decode_rows occ_measure occ_status_stream"
# The pinned publication predates upstream install/example switches.
# Build only the requested targets and install explicitly for this revision.
OECMAKE_TARGET_COMPILE = "occ ${@bb.utils.contains('PACKAGECONFIG', 'tools', d.getVar('LIGHT_COMMS_TOOLS'), '', d)}"

do_install() {
    install -d ${D}${libdir} ${D}${includedir}/occ
    install -m 0644 ${B}/libocc.a ${D}${libdir}/libocc.a
    install -m 0644 ${S}/include/occ/*.h ${D}${includedir}/occ/
    if ${@bb.utils.contains('PACKAGECONFIG', 'tools', 'true', 'false', d)}; then
        install -d ${D}${bindir}
        for tool in ${LIGHT_COMMS_TOOLS}; do
            install -m 0755 ${B}/$tool ${D}${bindir}/$tool
        done
    fi
}

PACKAGES =+ "${PN}-tools"
FILES:${PN}-tools = "${bindir}/occ_*"
# Standard -dev/-staticdev packages own the headers and archive.
# There is no shared-library/runtime package to depend on.
RDEPENDS:${PN}-dev = ""
BBCLASSEXTEND = "native nativesdk"
