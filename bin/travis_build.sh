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

if [[ $(uname) == "Linux" ]]; then
    GAUZE_ANDROID_EMULATOR_GPU=swiftshader
else
    GAUZE_ANDROID_EMULATOR_GPU=host
fi

if [[ ${TRAVIS} == "true" ]]; then
    GAUZE_ANDROID_USE_EMULATOR=YES # remote test w/ emulator
else
    GAUZE_ANDROID_USE_EMULATOR=NO # support local host testing on a real device
fi

# '--ios-{multiarch,combined}' do nothing for non-iOS builds
polly_args=(
    --toolchain "${TOOLCHAIN}"
    --config "${CONFIG}"
    --verbose
    --ios-multiarch --ios-combined
    --archive acf
    --jobs 2
    "${TEST}"
    "${INSTALL}"
    --fwd
    ACF_USE_DRISHTI_UPLOAD=YES
    ACF_BUILD_SHARED_SDK=${BUILD_SHARED}
    ACF_BUILD_TESTS=YES
    ACF_BUILD_EXAMPLES=YES
    ACF_COPY_3RDPARTY_LICENSES=ON
    ACF_HAS_GPU=${GPU}
    ACF_OPENGL_ES2=ON
    ACF_OPENGL_ES3=OFF
    GAUZE_ANDROID_USE_EMULATOR=${GAUZE_ANDROID_USE_EMULATOR}
    GAUZE_ANDROID_EMULATOR_GPU=${GAUZE_ANDROID_EMULATOR_GPU}
    GAUZE_ANDROID_EMULATOR_PARTITION_SIZE=40
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
    HUNTER_SUPPRESS_LIST_OF_FILES=ON
)

polly.py ${polly_args[@]} --reconfig
