#!/bin/bash

###############################################################
### A simple script for generating the CMake build files    ###
### Any arguments passed to this script will also be passed ###
### to CMake.                                               ###
###############################################################

############################
#         Constants
############################
readonly CUR_DIR="$(realpath "$(dirname "$0")")"

############################
#      Define variables
############################
BUILD_DIR="$CUR_DIR/build" 
BUILD_TYPE="Release"

############################
#         Functions
############################
die() {
    2>&1 echo "$1"
    exit "$2"
}

############################
# Handle command-line args
############################
if [[ $# -gt 0 ]]; then
    for i; do
        case $i in
            "--build-type="*)
                IFS='=' read -ra X <<< "$i"
                eval BUILD_TYPE="${X[1]}"
                unset IFS
            ;;
            "--build-dir="*)
                IFS='=' read -ra X <<< "$i"
                eval BUILD_DIR="${X[1]}"
                unset IFS
            ;;
        esac
    done
fi

readonly BUILD_DIR # prevent BUILD_DIR from changing

if [[ ! -d "$BUILD_DIR" ]]; then
    mkdir -p "$BUILD_DIR" || die "Failed to create build directory!" 1
fi

cd "$BUILD_DIR" || die "Could not enter build directory!" 2

cmake "$CUR_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$@"