#!/bin/bash

TOOLCHAIN=xcode
CONFIG=MinSizeRel

ARGS=(
    --verbose
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    --jobs 8
)

build.py --toolchain ${TOOLCHAIN} ${ARGS[@]} --install ${*}


