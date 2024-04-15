SUMMARY = "DISCO II Image processing pipeline Experiments"
SECTION = "DIPP-EXPERIMENTS"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=89e0ce65cb73fcf143363bd750fea72b"

SRC_URI = "file://ipc-experiments/"

S = "${WORKDIR}/ipc-experiments"

DEPENDS = "curl openssl meson-native ninja-native pkgconfig linux-tools-common linux-tools-generic procps build-essential"

inherit meson pkgconfig

do_configure:prepend() {
    cat > ${WORKDIR}/cross.txt <<EOF
[binaries]
c = '${TARGET_PREFIX}gcc'
cpp = '${TARGET_PREFIX}g++'
ar = '${TARGET_PREFIX}ar'
strip = '${TARGET_PREFIX}strip'
pkgconfig = 'pkg-config'
[properties]
needs_exe_wrapper = true
EOF
}

do_configure() {
    export CC="${TARGET_PREFIX}gcc"
    export CXX="${TARGET_PREFIX}g++"
    export CPP="${TARGET_PREFIX}gcc -E"
    export LD="${TARGET_PREFIX}ld"
    export AR="${TARGET_PREFIX}ar"
    export AS="${TARGET_PREFIX}as"
    export NM="${TARGET_PREFIX}nm"
    export RANLIB="${TARGET_PREFIX}ranlib"
    export STRIP="${TARGET_PREFIX}strip"

    export CFLAGS="${TARGET_CC_ARCH} -fstack-protector-strong -O2 -D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -Werror=format-security --sysroot=${STAGING_DIR_TARGET} -I${WORKDIR}/ipc-experiments/include"
    export CXXFLAGS="${CFLAGS}"

    meson setup ${S} ${B} --cross-file ${WORKDIR}/cross.txt -Dprefix=${D}${prefix}
}

do_install() {
    ninja -C ${B} install
    install -d ${D}/usr/share/dipp-experiments
    install -m 0644 ${WORKDIR}/ipc_experiments/run_test ${D}/usr/share/dipp-experiments
}

FILES:${PN} += "${libdir}/*"
FILES_${PN} += "/usr/share/dipp-experiments"