#!/bin/bash

TOOLCHAIN=ios-10-1-arm64-dep-8-0-hid-sections
CONFIG=MinSizeRel

ARGS=(
    --verbose
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    --jobs 8
)

build.py --toolchain ${TOOLCHAIN} ${ARGS[@]} --reconfig --install ${*}




