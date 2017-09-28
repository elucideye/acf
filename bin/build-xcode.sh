#!/bin/bash

TOOLCHAIN=xcode
CONFIG=MinSizeRel

ARGS=(
    --verbose
    --toolchain ${TOOLCHAIN}
    --config ${CONFIG}
    --fwd HUNTER_CONFIGURATION_TYPES=${CONFIG}
    --jobs 8
)

build.py ${ARGS[@]} --install ${*}
