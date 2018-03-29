#!/bin/bash
#
# Simple acf SDK build script to support both CI builds
# and matching host development.

# Sample input parameteters:
# TOOLCHAIN=android-ndk-r10e-api-19-armeabi-v7a-neon-hid-sections-lto
# CONFIG=MinSizeRel
# INSTALL=--strip

if [ ${#} != 3 ]; then
    echo 2>&1 "usage: travis_build.sh TOOLCHAIN CONFIG INSTALL"
    exit 1
fi

TOOLCHAIN="${1}"
CONFIG="${2}"
INSTALL="${3}"

# '--ios-{multiarch,combined}' do nothing for non-iOS builds
polly_args=(
    --toolchain ${TOOLCHAIN}
    --config ${CONFIG}
    --verbose
    --ios-multiarch --ios-combined
    --fwd
    ACF_BUILD_TESTS=YES
    ACF_BUILD_EXAMPLES=YES
    ACF_COPY_3RDPARTY_LICENSES=ON
    ACF_USE_DRISHTI_UPLOAD=ON
    GAUZE_ANDROID_USE_EMULATOR=YES
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
    HUNTER_SUPPRESS_LIST_OF_FILES=ON
    --archive acf
    --jobs 2
    --test
    ${INSTALL}
)

polly.py ${polly_args[@]} --reconfig
