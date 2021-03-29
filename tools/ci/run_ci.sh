#!/usr/bin/env bash
#
# Copyright (c) 2020 GARDENA GmbH
#
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v2.0
# and Eclipse Distribution License v1.0 which accompany this distribution.
#
# The Eclipse Public License is available at
#    http://www.eclipse.org/legal/epl-v20.html
# The Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
# Contributors:
#   Reto Schneider, GARDENA GmbH - Please refer to git log

set -eu -o pipefail

readonly REPO_ROOT_DIR="${PWD}"
readonly SCRIPT_NAME="$(basename "$0")"

CMAKE_ARGS="-DCMAKE_BUILD_TYPE=RelWithDebInfo"
RUN_CLEAN=0
RUN_BUILD=0
RUN_TESTS=0
OPT_VERBOSE=0
OPT_SANITIZER=""

HELP_MSG="usage: ${SCRIPT_NAME} <OPTIONS>...
Runs build and test steps in CI.
Select steps to execute with --run- options

Options:
 -v, --verbose             Verbose output
 -a, --all                 Run all steps required for a MR
 -h, --help                Display this help and exit
 --sanitizer TYPE          Enable sanitizer
                           (TYPE: address leak thread undefined)

Available steps (executed by --all):
  --clean                  Remove all build artifacts
  --build                  Build all targets
  --run-tests              Build and execute tests
"

function usage() {
  exit_code=$1

  echo "${HELP_MSG}"
  exit "${exit_code}"
}

function run_clean() {
  rm -rf build-wakaama-tests
  rm -rf build-wakaama-examples
}

function run_build_tests() {
  cmake -GNinja -S tests -B build-wakaama-tests ${CMAKE_ARGS}
  cmake --build build-wakaama-tests
}

function run_build_examples() {
  cmake -GNinja -S examples -B build-wakaama-examples ${CMAKE_ARGS}
  cmake --build build-wakaama-examples
}

function run_build() {
  run_build_tests
  run_build_examples
}

function run_tests() {
  run_build_tests
  build-wakaama-tests/lwm2munittests
}

# Parse Options

ret=0
getopt --test > /dev/null || ret=$?
if [ $ret -ne 4 ]; then
  echo "Error: getopt version is not as expected"
  exit 1
fi

if ! PARSED_OPTS=$(getopt -o vah \
                          -l all \
                          -l build \
                          -l clean \
                          -l help \
                          -l sanitizer: \
                          -l run-tests \
                          -l verbose \
                          --name "${SCRIPT_NAME}" -- "$@");
then
  usage 1
fi

eval set -- "${PARSED_OPTS}"

while true; do
  case "$1" in
    --clean)
      RUN_CLEAN=1
      shift
      ;;
    --build)
      RUN_BUILD=1
      shift 1
      ;;
    --run-tests)
      RUN_TESTS=1
      shift
      ;;
    --sanitizer)
      OPT_SANITIZER=$2
      shift 2
      ;;
    --)
      shift
      break
      ;;
    -v|--verbose)
      OPT_VERBOSE=1
      shift
      ;;
    -a|--all)
      RUN_CLEAN=1
      RUN_BUILD=1
      RUN_TESTS=1
      shift
      ;;
    -h|--help)
      usage 0
      ;;
    *)
      echo "Error: args parsing failed"
      exit 1
      ;;
  esac
done

if [ $# -gt 0 ]; then
  echo "too many arguments"
  usage 1
fi

if [ "${OPT_VERBOSE}" -ne 0 ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_VERBOSE_MAKEFILE=ON"
fi

if [ -n "${OPT_SANITIZER}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DSANITIZER=${OPT_SANITIZER}"
fi

# Run Steps

if [ "${RUN_CLEAN}" -eq 1 ]; then
    run_clean
fi

if [ "${RUN_TESTS}" -eq 1 ]; then
    run_tests
fi

if [ "${RUN_BUILD}" -eq 1 ]; then
    run_build
fi
