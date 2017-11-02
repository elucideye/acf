#!/bin/bash

TOOLCHAIN=raspberrypi3-cxx11
#TOOLCHAIN=raspberrypi3-gcc-pic-hid-sections
CONFIG=Release

ARGS=(
    --verbose
    --toolchain ${TOOLCHAIN}
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    ACF_USE_DRISHTI_CACHE=OFF
    ACF_BUILD_TESTS=OFF
    ACF_BUILD_OGLES_GPGPU=OFF
    --jobs 8
)

build.py ${ARGS[@]} --install --reconfig ${*}
#build.py ${ARGS[@]} --install ${*}
