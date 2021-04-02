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
OPT_C_EXTENSIONS=""
OPT_C_STANDARD=""
OPT_VERBOSE=0
OPT_SANITIZER=""
OPT_TEST_COVERAGE_REPORT=""
OPT_SCAN_BUILD=""
OPT_SONARQUBE=""
OPT_WRAPPER_CMD=""

HELP_MSG="usage: ${SCRIPT_NAME} <OPTIONS>...
Runs build and test steps in CI.
Select steps to execute with --run- options

Options:
 -v, --verbose             Verbose output
 -a, --all                 Run all steps required for a MR
 -h, --help                Display this help and exit
 --sanitizer TYPE          Enable sanitizer
                           (TYPE: address leak thread undefined)
 --scan-build BINARY       Enable Clang code analyzer using specified
                           executable
                           (BINARY: e.g. scan-build-10)
 --test-coverage REPORT    Enable code coverage measurement, output REPORT
                           (REPORT: xml html text none)
 --c-standard VERSION      Explicitly specify C VERSION to be used
                           (VERSION: 99, 11)
 --c-extensions ENABLE     Whether to allow compiler extensions. Defaults to
                           ON.
                           (ENABLE: ON or OFF)
 --sonarqube WRAPPER       Collect data for SonarQube
                           (WRAPPER: path to build-wrapper)

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
  rm -rf build-wakaama
}

function run_build() {
  # Existing directory eeded by SonarQube build-wrapper
  mkdir -p build-wakaama

  ${OPT_WRAPPER_CMD} cmake -GNinja -S . -B build-wakaama ${CMAKE_ARGS}
  ${OPT_WRAPPER_CMD} cmake --build build-wakaama
}

function run_tests() {
  build-wakaama/tests/lwm2munittests

  mkdir -p "${REPO_ROOT_DIR}/build-wakaama/coverage"

  if [ -z "${OPT_TEST_COVERAGE_REPORT}" ]; then
    return 0
  fi

  #see https://github.com/koalaman/shellcheck/wiki/SC2089
  gcovr_opts=(-r "${REPO_ROOT_DIR}/build-wakaama" \
    --keep `: # Needed for SonarQube` \
    --exclude "${REPO_ROOT_DIR}"/examples \
    --exclude "${REPO_ROOT_DIR}"/tests)

  case "${OPT_TEST_COVERAGE_REPORT}" in
    xml)
      gcovr_out="--xml"
      gcovr_file=("${REPO_ROOT_DIR}/build-wakaama/coverage/report.xml")
      ;;
    html)
      gcovr_out="--html --html-details"
      gcovr_file=("${REPO_ROOT_DIR}/build-wakaama/coverage/report.html")
      ;;
    text)
      gcovr_out=""
      gcovr_file=("${REPO_ROOT_DIR}/build-wakaama/coverage/report.txt")
      ;;
    none)
      gcovr "${gcovr_opts[@]}" >/dev/null
      echo "Coverage measured, but no report generated"
      return 0
      ;;
    *)
      echo "Error: Unsupported coverage output format: " \
           "${OPT_TEST_COVERAGE_REPORT}"
      usage 1
      ;;
  esac

  gcovr "${gcovr_opts[@]}" $gcovr_out -o "${gcovr_file[@]}"
  echo Coverage file "${gcovr_file[@]}" ready
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
                          -l c-extensions: \
                          -l c-standard: \
                          -l clean \
                          -l help \
                          -l sanitizer: \
                          -l scan-build: \
                          -l sonarqube: \
                          -l run-tests \
                          -l test-coverage: \
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
    --c-extensions)
      OPT_C_EXTENSIONS=$2
      shift 2
      ;;
    --c-standard)
      OPT_C_STANDARD=$2
      shift 2
      ;;
    --run-tests)
      RUN_TESTS=1
      shift
      ;;
    --sanitizer)
      OPT_SANITIZER=$2
      shift 2
      ;;
    --scan-build)
      OPT_SCAN_BUILD=$2
      # Analyzing works only when code gets actually built
      RUN_CLEAN=1
      shift 2
      ;;
    --sonarqube)
      OPT_SONARQUBE=$2
      # Analyzing works only when code gets actually built
      RUN_CLEAN=1
      shift 2
      ;;
    --test-coverage)
      OPT_TEST_COVERAGE_REPORT=$2
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

if [ -n "${OPT_C_EXTENSIONS}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_C_EXTENSIONS=${OPT_C_EXTENSIONS}"
fi

if [ -n "${OPT_C_STANDARD}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_C_STANDARD=${OPT_C_STANDARD}"
fi

if [ -n "${OPT_SANITIZER}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DSANITIZER=${OPT_SANITIZER}"
fi

if [ -n "${OPT_SCAN_BUILD}" ] && [ -n "${OPT_SONARQUBE}" ]; then
  echo "--sonarqube and --scan-build can not be enabled at the same time"
  exit 1
fi

if [ -n "${OPT_SONARQUBE}" ]; then
  OPT_TEST_COVERAGE_REPORT="${OPT_TEST_COVERAGE_REPORT:-none}"
  OPT_WRAPPER_CMD="${OPT_SONARQUBE} \
    --out-dir build-wakaama/sonar-cloud-build-wrapper-output"
fi

if [ -n "${OPT_TEST_COVERAGE_REPORT}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCOVERAGE=ON"
fi

if [ -n "${OPT_SCAN_BUILD}" ]; then
  OPT_WRAPPER_CMD="${OPT_SCAN_BUILD} \
    -o build-wakaama/clang-static-analyzer"
fi

# Run Steps

if [ "${RUN_CLEAN}" -eq 1 ]; then
  run_clean
fi

if [ "${RUN_BUILD}" -eq 1 ]; then
  run_build
fi

if [ "${RUN_TESTS}" -eq 1 ]; then
  run_tests
fi
