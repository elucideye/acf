#!/bin/bash

TOOLCHAIN=xcode
CONFIG=Release

ARGS=(
    --verbose
    --toolchain ${TOOLCHAIN}
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    ACF_USE_DRISHTI_CACHE=OFF
    ACF_BUILD_TESTS=ON
    --jobs 8
)

#build.py ${ARGS[@]} --install --reconfig ${*}
build.py ${ARGS[@]} --install ${*}
