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
OPT_BRANCH_SOURCE=
OPT_BRANCH_TARGET=main
OPT_C_EXTENSIONS=""
OPT_C_STANDARD=""
OPT_CLANG_FORMAT="clang-format-18"
OPT_SANITIZER=""
OPT_SCAN_BUILD=""
OPT_SONARQUBE=""
OPT_SOURCE_DIRECTORY="${REPO_ROOT_DIR}"
OPT_BUILD_DIRECTORY="build-wakaama"
OPT_TEST_COVERAGE_REPORT=""
OPT_CODE_CHECKER="full"
OPT_VERBOSE=0
OPT_WRAPPER_CMD=""
RUN_BUILD=0
RUN_CLANG_FORMAT=0
RUN_CLEAN=0
RUN_CMAKE_FORMAT=0
RUN_GITLINT=0
RUN_GIT_BLAME_IGNORE=0
RUN_TESTS=0
RUN_DOXYGEN=0
RUN_CODE_CHECKER=0

HELP_MSG="usage: ${SCRIPT_NAME} <OPTIONS>...
Runs build and test steps in CI.
Select steps to execute with --run- options

Options:
  --branch-source BRANCH    Source branch for MRs
                            (default: current branch)
  --branch-target BRANCH    Target branch for MRs
                            (default: $OPT_BRANCH_TARGET)
  --c-extensions ENABLE     Whether to allow compiler extensions. Defaults to
                            ON.
                            (ENABLE: ON or OFF)
  --c-standard VERSION      Explicitly specify C VERSION to be used
                            (VERSION: 99, 11)
  --clang-format BINARY     Set specific clang-format binary
                            (BINARY: defaults to ${OPT_CLANG_FORMAT})
  --source-directory PATH   Configure CMake using PATH instead of the
                            repositories root directory.
  --build-directory PATH    Configure CMake using PATH as the build directory.
                            Defaults to 'build-wakaama'. Builds from CMake
                            presets are placed into 'build-presets' by default.
  --sanitizer TYPE          Enable sanitizer
                            (TYPE: address leak thread undefined)
  --scan-build BINARY       Enable Clang code analyzer using specified
                            executable
                            (BINARY: e.g. scan-build-10)
  --sonarqube WRAPPER       Collect data for SonarQube
                            (WRAPPER: path to build-wrapper)
  --test-coverage REPORT    Enable code coverage measurement, output REPORT
                            (REPORT: xml html text none)
  --code-checker ACTION     Run the CodeChecker code analyzer to create a baseline,
                            do a full check or a PR check (show just difference to baseline)
                            (TYPE: full, diff, baseline)
  -v, --verbose             Verbose output
  -a, --all                 Run all steps required for a MR
  -h, --help                Display this help and exit

Available steps (executed by --all):
  --run-gitlint            Check git commits with gitlint
  --run-clang-format       Check C code formatting
  --run-git-blame-ignore   Validate .git-blame-ignore-revs
  --run-clean              Remove all build artifacts
  --run-cmake-format       Check CMake files formatting
  --run-build              Build all targets
  --run-tests              Execute tests (works only for top level project)
  --run-doxygen            Build the Doxygen documentation of the code
  --run-code-checker       Run the CodeChecker code analyzer
"

function usage() {
  exit_code=$1

  echo "${HELP_MSG}"
  exit "${exit_code}"
}

function run_clang_format() {
  local patch_file

  command "git-${OPT_CLANG_FORMAT}"

  patch_file="$(mktemp -t clang-format-patch.XXX)"
  # shellcheck disable=SC2064
  trap "{ rm -f -- '${patch_file}'; }" EXIT TERM INT

  (set +o pipefail; "git-${OPT_CLANG_FORMAT}" --diff "${OPT_BRANCH_TARGET}" 2>&1 |
    { grep -v \
      -e 'no modified files to format' \
      -e 'clang-format did not modify any files' || true;
    } > "${patch_file}"
  )

  if [ -s "${patch_file}" ]; then
    cat "${patch_file}"
    exit 1
  fi

  echo "No C code formatting errors found"
}

function run_clean() {
  rm -rf "${OPT_BUILD_DIRECTORY}"
  rm -rf build-presets
}

function run_cmake_format() {
  # Ensure formatting respects the defined style. Running cmake-lint before
  # cmake-format gives a nicer message if formatting does not match the
  # specified format.
  git ls-files '*CMakeLists.txt' '*.cmake' | xargs cmake-lint

  # Ensure formatting is exactly what cmake-format produces. Insisting on this
  # ensures we can run cmake-format on all CMake files at any time.
  if ! git ls-files '*CMakeLists.txt' '*.cmake' | xargs cmake-format --check; then
    echo 'Please run cmake-format on all (changed) CMake files.'
    exit 1
  fi
}

function run_gitlint() {
  commits="${OPT_BRANCH_TARGET}...${OPT_BRANCH_SOURCE}"

  gitlint --commits "${commits}"
}

function run_git_blame_ignore() {
  for commit in $(grep -E '^[A-Za-z0-9]{40}$' .git-blame-ignore-revs); do
    if ! git merge-base --is-ancestor "${commit}" HEAD; then
      echo ".git-blame-ignore-revs: Commit ${commit} is not an ancestor."
      exit
    fi
  done
}

function run_build() {
  # Existing directory needed by SonarQube build-wrapper
  mkdir -p "${OPT_BUILD_DIRECTORY}"

  ${OPT_WRAPPER_CMD} cmake -GNinja -S ${OPT_SOURCE_DIRECTORY} -B "${OPT_BUILD_DIRECTORY}" \
    -DWAKAAMA_PLATFORM=POSIX ${CMAKE_ARGS}
  ${OPT_WRAPPER_CMD} cmake --build "${OPT_BUILD_DIRECTORY}"
}

function run_tests() {
  export CTEST_OUTPUT_ON_FAILURE=ON

  cmake --build "${OPT_BUILD_DIRECTORY}" --target test

  mkdir -p "${REPO_ROOT_DIR}/${OPT_BUILD_DIRECTORY}/coverage"

  if [ -z "${OPT_TEST_COVERAGE_REPORT}" ]; then
    return 0
  fi

  #see https://github.com/koalaman/shellcheck/wiki/SC2089
  gcovr_opts=(-r "${REPO_ROOT_DIR}/" \
    --keep `: # Needed for SonarQube` \
    --exclude "${REPO_ROOT_DIR}"/examples)

  case "${OPT_TEST_COVERAGE_REPORT}" in
    xml)
      gcovr_out="--xml"
      gcovr_file=("${REPO_ROOT_DIR}/${OPT_BUILD_DIRECTORY}/coverage/report.xml")
      ;;
    html)
      gcovr_out="--html --html-details"
      gcovr_file=("${REPO_ROOT_DIR}/${OPT_BUILD_DIRECTORY}/coverage/report.html")
      ;;
    text)
      gcovr_out=""
      gcovr_file=("${REPO_ROOT_DIR}/${OPT_BUILD_DIRECTORY}/coverage/report.txt")
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
  # clean up
  find "${REPO_ROOT_DIR}" -name \*.gcov -exec rm {} \;
}

function run_doxygen() {
  mkdir -p build-wakaama/doxygen
    GIT_REVISION=$(git rev-parse @) WORKING_DIR=$(pwd) DOXYGEN_OUT_DIR=build-wakaama/doxygen \
      doxygen doc/doxygen/Doxyfile
}

function run_code_checker() {
  readonly config_file="${REPO_ROOT_DIR}/tools/code_checker/config.json"
  readonly ignore_file="${REPO_ROOT_DIR}/tools/code_checker/ignore.txt"
  readonly baseline_file="${REPO_ROOT_DIR}/tools/code_checker/reports.baseline"
  readonly code_checker_result_dir="build-wakaama/code_checker_result/"
  readonly code_checker_report="build-wakaama/code_checker_report/"

  set +e +o pipefail
  CodeChecker check --logfile build-wakaama/compile_commands.json \
                    --config "${config_file}" \
                    --ignore "${ignore_file}" \
                    --output ${code_checker_result_dir} \
                    || true  # Always returns a non-zero status if issues are found
  set -e -o pipefail

  if [ "${OPT_CODE_CHECKER}" = "diff" ]; then
    CodeChecker cmd diff -b "${baseline_file}" \
                         -n $code_checker_result_dir \
                         -o html \
                         --export "${code_checker_report}" \
                         --new
  else
    if [ "${OPT_CODE_CHECKER}" = "baseline" ]; then
      output_format="baseline"
      output_location="${baseline_file}"
    else
      output_format="html"
      output_location="${code_checker_report}"
    fi

    CodeChecker parse -e "${output_format}" \
                      -o "${output_location}" \
                      --config "${config_file}" \
                      --ignore "${ignore_file}" \
                      --trim-path-prefix="${REPO_ROOT_DIR}" \
                      "${code_checker_result_dir}"
    fi
}

# Parse Options

if [[ "$OSTYPE" == "darwin"* ]]; then
  getopt=$(brew --prefix gnu-getopt)/bin/getopt
else
  getopt=$(which getopt)
fi

ret=0
$getopt --test > /dev/null || ret=$?
if [ $ret -ne 4 ]; then
  echo "Error: getopt version is not as expected"
  exit 1
fi

if ! PARSED_OPTS=$($getopt -o vah \
                          -l all \
                          -l branch-source: \
                          -l branch-target: \
                          -l c-extensions: \
                          -l c-standard: \
                          -l clang-format: \
                          -l help \
                          -l run-build \
                          -l run-clang-format \
                          -l run-clean \
                          -l run-cmake-format \
                          -l run-gitlint \
                          -l run-git-blame-ignore \
                          -l run-tests \
                          -l run-doxygen \
                          -l run-code-checker \
                          -l sanitizer: \
                          -l scan-build: \
                          -l sonarqube: \
                          -l source-directory: \
                          -l build-directory: \
                          -l test-coverage: \
                          -l code-checker: \
                          -l verbose \
                          --name "${SCRIPT_NAME}" -- "$@");
then
  usage 1
fi

eval set -- "${PARSED_OPTS}"

while true; do
  case "$1" in
    --branch-source)
      OPT_BRANCH_SOURCE=$2
      shift 2
      ;;
    --branch-target)
      OPT_BRANCH_TARGET=$2
      shift 2
      ;;
    --c-extensions)
      OPT_C_EXTENSIONS=$2
      shift 2
      ;;
    --c-standard)
      OPT_C_STANDARD=$2
      shift 2
      ;;
    --clang-format)
      OPT_CLANG_FORMAT=$2
      shift 2
      ;;
    --run-clang-format)
      RUN_CLANG_FORMAT=1
      shift
      ;;
    --run-clean)
      RUN_CLEAN=1
      shift
      ;;
    --run-cmake-format)
      RUN_CMAKE_FORMAT=1
      shift
      ;;
    --run-build)
      RUN_BUILD=1
      shift 1
      ;;
    --run-gitlint)
      RUN_GITLINT=1
      shift
      ;;
    --run-git-blame-ignore)
      RUN_GIT_BLAME_IGNORE=1
      shift
      ;;
    --run-tests)
      RUN_TESTS=1
      shift
      ;;
    --run-doxygen)
      RUN_DOXYGEN=1
      shift
      ;;
    --run-code-checker)
      RUN_CODE_CHECKER=1
      # Analyzing works only when code gets actually built
      RUN_CLEAN=1
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
    --source-directory)
      OPT_SOURCE_DIRECTORY=$2
      shift 2
      ;;
    --build-directory)
      OPT_BUILD_DIRECTORY=$2
      shift 2
      ;;
    --test-coverage)
      OPT_TEST_COVERAGE_REPORT=$2
      shift 2
      ;;
    --code-checker)
      OPT_CODE_CHECKER=$2
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
      RUN_CLANG_FORMAT=1
      RUN_CLEAN=1
      RUN_CMAKE_FORMAT=1
      RUN_GITLINT=1
      RUN_GIT_BLAME_IGNORE=1
      RUN_BUILD=1
      RUN_TESTS=1
      RUN_DOXYGEN=1
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

if [ "${RUN_CODE_CHECKER}" = "1" ] && [ -n "${OPT_SONARQUBE}" ]; then
  echo "--sonarqube and --code-checker can not be enabled at the same time"
  exit 1
fi

if [ -n "${OPT_SONARQUBE}" ]; then
  OPT_TEST_COVERAGE_REPORT="${OPT_TEST_COVERAGE_REPORT:-none}"
  OPT_WRAPPER_CMD="${OPT_SONARQUBE} \
    --out-dir ${OPT_BUILD_DIRECTORY}/sonar-cloud-build-wrapper-output"
fi

if [ -n "${OPT_TEST_COVERAGE_REPORT}" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCOVERAGE=ON"
fi

if [ -n "${OPT_SCAN_BUILD}" ]; then
  # Excluding tests as `scan-build` does not know macros of
  # `CUnit` which leads to a lot of noisy issues in the report.
  # Also ignoring third-party `tinydtls` library.
  OPT_WRAPPER_CMD="${OPT_SCAN_BUILD} \
    -o ${OPT_BUILD_DIRECTORY}/clang-static-analyzer \
    --exclude tests \
    --exclude examples/shared/tinydtls"
fi

if [ "${RUN_CODE_CHECKER}" = "1" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug"
fi

# Run Steps

if [ "${RUN_GITLINT}" -eq 1 ]; then
  run_gitlint
fi

if [ "${RUN_CLANG_FORMAT}" -eq 1 ]; then
  run_clang_format
fi

if [ "${RUN_GIT_BLAME_IGNORE}" -eq 1 ]; then
  run_git_blame_ignore
fi

if [ "${RUN_CLEAN}" -eq 1 ]; then
  run_clean
fi

if [ "${RUN_CMAKE_FORMAT}" -eq 1 ]; then
  run_cmake_format
fi

if [ "${RUN_BUILD}" -eq 1 ]; then
  run_build
fi

if [ "${RUN_TESTS}" -eq 1 ]; then
  run_tests
fi

if [ "${RUN_DOXYGEN}" -eq 1 ]; then
  run_doxygen
fi

if [ "${RUN_CODE_CHECKER}" = "1" ]; then
  run_code_checker
fi

