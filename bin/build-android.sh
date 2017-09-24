#!/bin/bash

TOOLCHAIN=android-ndk-r10e-api-19-armeabi-v7a-neon-hid-sections
CONFIG=MinSizeRel

ARGS=(
    --verbose
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    --jobs 8
)

build.py --toolchain ${TOOLCHAIN} ${ARGS[@]} --reconfig --install
