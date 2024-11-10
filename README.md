# Wakaama

[![Build](https://github.com/eclipse/wakaama/actions/workflows/build_and_test.yaml/badge.svg)](https://github.com/eclipse/wakaama/actions/workflows/build_and_test.yaml)

[![OpenSSF Scorecard](https://api.scorecard.dev/projects/github.com/eclipse-wakaama/wakaama/badge)](https://scorecard.dev/viewer/?uri=github.com/eclipse-wakaama/wakaama)

Wakaama (formerly liblwm2m) is an implementation of the Open Mobile Alliance's LightWeight M2M
protocol (LWM2M).

Developers mailing list: https://dev.eclipse.org/mailman/listinfo/wakaama-dev

## Security warning

The only official release of Wakaama, version 1.0, is affected by various
security issues ([CVE-2019-9004], [CVE-2021-41040]).

Please use the most recent commit in the main branch. Release 1.0 is not
supported anymore.

[CVE-2019-9004]: https://www.cve.org/CVERecord?id=CVE-2019-9004
[CVE-2021-41040]: https://www.cve.org/CVERecord?id=CVE-2021-41040

## License

This work is dual-licensed under the Eclipse Public License v2.0 and Eclipse Distribution License v1.0.

`SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause`

## Checking out the code

### Using Wakaama as library

```
git clone https://github.com/eclipse/wakaama.git
```

### Working on Wakaama

When working on Wakaama itself, or intending to run the example client application, submodules must be checked out:

```
git clone --recurse-submodules https://github.com/eclipse/wakaama.git
```

## Compiling

Wakaama is a highly configurable library. It is built with CMake.
Look at examples/server/CMakeLists.txt for an example of how to include it.

The different settings can be configured with CMake cache variables (e.g. `cmake -DLOG_LEVEL=INFO`).

### Mode

Wakaama supports multiple modes. At least one mode needs to be defined with CMake cache variables.

- WAKAAMA_MODE_SERVER to enable LWM2M Server interfaces.
- WAKAAMA_MODE_BOOTSTRAP_SERVER to enable LWM2M Bootstrap Server interfaces.
- WAKAAMA_MODE_CLIENT to enable LWM2M Client interfaces.

#### Client Settings

Wakaama supports additional client related options. These are only available if the client mode is enabled. 

- WAKAAMA_CLIENT_INITIATED_BOOTSTRAP to enable LWM2M Bootstrap support in a LWM2M Client.
- WAKAAMA_CLIENT_LWM2M_V_1_0: Restrict the client code to use LwM2M version 1.0
 
Please note: LwM2M version 1.0 is only supported by clients, while servers are backward compatible.

### Data Formats

The following data formats are configurable for Wakaama:

- WAKAAMA_DATA_TLV to enable TLV payload support (implicit except for LWM2M 1.1 clients)
- WAKAAMA_DATA_JSON to enable JSON payload support (implicit when defining LWM2M_SERVER_MODE)
- WAKAAMA_DATA_SENML_JSON to enable SenML JSON payload support (implicit for LWM2M 1.1 or greater when defining LWM2M_SERVER_MODE or LWM2M_BOOTSTRAP_SERVER_MODE)
- WAKAAMA_DATA_SENML_CBOR to enable SenML CBOR payload support (implicit for LWM2M 1.1 or greater when defining LWM2M_SERVER_MODE or LWM2M_BOOTSTRAP_SERVER_MODE)
- WAKAAMA_DATA_SENML_CBOR_FLOAT16_SUPPORT to enable 16-bit floating point encoding support in CBOR.
- WAKAAMA_DATA_OLD_CONTENT_FORMAT to support the deprecated content format values for TLV and JSON.

### CoAP Settings

- WAKAAMA_COAP_RAW_BLOCK1_REQUESTS For low memory client devices where it is not possible to keep a large post or put request in memory to be parsed (typically a firmware write).
  This option enable each unprocessed block 1 payload to be passed to the application, typically to be stored to a flash memory.
- WAKAAMA_COAP_DEFAULT_BLOCK_SIZE CoAP block size used by CoAP layer when performing block-wise transfers. Possible values: 16, 32, 64, 128, 256, 512 and 1024. Defaults to 1024.


### Logging

The logging infrastructure can be configured with CMake cache variables (e.g. `cmake -DWAKAAMA_LOG_LEVEL=INFO`).

- WAKAAMA_LOG_LEVEL: Lowest log level to be enabled. Higher levels are also enabled.
  - One of: DBG, INFO, WARN, ERR, FATAL, LOG_DISABLED (default)
- WAKAAMA_LOG_CUSTOM_HANDLER: Set this define to provide a custom handler function for log entries. See the default implementation for details.
- WAKAAMA_LOG_MAX_MSG_TXT_SIZE: The max. size of the formatted log message. This is only the message without additional data like severity and function name.

### Transport

- WAKAAMA_TRANSPORT: Select the implementation of the transport layer. One of:
  - POSIX_UDP: A simple UDP implementation using the POSIX socket API.
  - TINYDTLS: Use DTLS with the 'tinydtls' library.
  - NONE: No transport layer is provided.

If `NONE` is chosen, the user of Wakaama needs to implement a custom transport layer. Check the available implementations for more information.

### Platform

- WAKAAMA_PLATFORM: Select the implementation of the platform abstraction layer, one of:
  - POSIX: An implementation using the POSIX API.
  - NONE: No platform abstraction layer is provided.

If `NONE` is chosen, the user of Wakaama needs to implement a custom platform abstraction layer. Check the available POSIX implementation for more information.

### Command Line

Wakaama provides a simple CLI library. It can be enabled with:

- WAKAAMA_CLI: If enabled the command line library is added to Wakaama (default: disabled)

## Development

### Dependencies and Tools
- Mandatory:
  - Compiler: GCC and/or Clang
- Optional (but strongly recommended):
  - Build system generator: CMake 3.21+
  - Version control system: Git (and a GitHub account)
  - Git commit message linter: gitlint
  - Build system: ninja
  - C code formatting: clang-format, version 14
  - CMake list files formatting: cmake-format, version 0.6.13
  - Unit testing: CUnit

On Ubuntu 20.04, used in CI, the dependencies can be installed as such:
- `apt install build-essential clang-format clang-format-14 clang-tools-14 cmake gcovr git libcunit1-dev ninja-build python3-pip`
- `pip3 install -r tools/requirements-compliance.txt`

For macOS the development dependencies can be installed as such:

`brew install automake clang-format cmake cunit gcc gitlint gnu-getopt make ninja`

### Code formatting
#### C
New C code must be formatted with [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

The style is based on the LLVM style, but with 4 instead of 2 spaces indentation and allowing for 120 instead of 80
characters per line.

To check if your code matches the expected style, the following commands are helpful:
 - `git clang-format-14 --diff`: Show what needs to be changed to match the expected code style
 - `git clang-format-14`: Apply all needed changes directly
 - `git clang-format-14 --commit main`: Fix code style for all changes since main

If existing code gets reformatted, this must be done in a separate commit. Its commit id has to be added to the file
`.git-blame-ignore-revs` and committed in yet another commit.

#### CMake
All CMake code must be formatted with [cmake-format](https://github.com/cheshirekow/cmake_format).

To check if your code matches the expected style, the following commands are helpful:
 - `tools/ci/run_ci.sh --run-cmake-format`: Test all CMake files, print offending ones
 - `cmake-format --in-place <unformatted-file>`: Apply all needed changes directly to <unformatted-file>

### Running CI tests locally
To avoid unneeded load on the GitHub infrastructure, please consider running `tools/ci/run_ci.sh --all` before pushing.

### Running integration tests locally
```
cd wakaama
tools/ci/run_ci.sh --run-build
pytest -v tests/integration
```

## Examples

The examples can be enabled (or disabled) with the CMake cache variable `WAKAAMA_ENABLE_EXAMPLES` (e.g.
`cmake -DWAKAAMA_ENABLE_EXAMPLES=OFF`).

There are some example applications provided to test the server, client and bootstrap capabilities of Wakaama.
The following recipes assume you are on a unix like platform and you have cmake and make installed.

### Server example

 * ``cmake -S examples/server -B build-server``
 * ``cmake --build build-server``
 * ``./build-server/lwm2mserver [Options]``

The lwm2mserver listens on UDP port 5683. It features a basic command line
interface. Type 'help' for a list of supported commands.

Options are:
```
Usage: lwm2mserver [OPTION]
Launch a LWM2M server on localhost.

Options:
  -4		Use IPv4 connection. Default: IPv6 connection
  -l PORT	Set the local UDP port of the Server. Default: 5683
  -S BYTES	CoAP block size. Options: 16, 32, 64, 128, 256, 512, 1024. Default: 1024
```

### Test client example

 * ``cmake -S examples/client -B build-client -DWAKAAMA_MODE_CLIENT=ON``
 * ``cmake --build build-client``
 * ``./build-client/lwm2mclient [Options]``

Next to lwm2mclient a DTLS enabled variant named lwm2mclient_tinydtls gets built.

The lwm2mclient features nine LWM2M objects:
 - Security Object (id: 0)
 - Server Object (id: 1)
 - Access Control Object (id: 2) as a skeleton
 - Device Object (id: 3) containing hard-coded values from the Example LWM2M
 Client of Appendix E of the LWM2M Technical Specification.
 - Connectivity Monitoring Object (id: 4) as a skeleton
 - Firmware Update Object (id: 5) as a skeleton.
 - Location Object (id: 6) as a skeleton.
 - Connectivity Statistics Object (id: 7) as a skeleton.
 - Test Object (id: 31024) with the following description:

                           Multiple
          Object |  ID   | Instances | Mandatory |
           Test  | 31024 |    Yes    |    No     |

           Resources:
                       Supported    Multiple
           Name | ID | Operations | Instances | Mandatory |  Type   | Range |
           test |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |
           exec |  2 |     E      |    No     |    Yes    |         |       |
           dec  |  3 |    R/W     |    No     |    Yes    |  Float  |       |

The lwm2mclient opens UDP port 56830 and tries to register to a LWM2M Server at
127.0.0.1:5683. It features a basic command line interface. Type 'help' for a
list of supported commands.

Options are:
```
Usage: lwm2mclient [OPTION]
Launch a LWM2M client.
Options:
  -n NAME	Set the endpoint name of the Client. Default: testlwm2mclient
  -l PORT	Set the local UDP port of the Client. Default: 56830
  -h HOST	Set the hostname of the LWM2M Server to connect to. Default: localhost
  -p PORT	Set the port of the LWM2M Server to connect to. Default: 5683
  -4		Use IPv4 connection. Default: IPv6 connection
  -t TIME	Set the lifetime of the Client. Default: 300
  -b		Bootstrap requested.
  -c		Change battery level over time.
  -S BYTES	CoAP block size. Options: 16, 32, 64, 128, 256, 512, 1024. Default: 1024

```

Additional values for the lwm2mclient_tinydtls binary:
```
  -i Set the device management or bootstrap server PSK identity. If not set use none secure mode
  -s Set the device management or bootstrap server Pre-Shared-Key. If not set use none secure mode
```

To launch a bootstrap session:
``./lwm2mclient -b``

### Simpler test client example

 * ``cmake -S examples/lightclient -B build-lightclient``
 * ``cmake --build build-lightclient``
 * ``./build-lightclient/lightclient [Options]``

The lightclient is much simpler that the lwm2mclient and features only four
LWM2M objects:
 - Security Object (id: 0)
 - Server Object (id: 1)
 - Device Object (id: 3) containing hard-coded values from the Example LWM2M
 Client of Appendix E of the LWM2M Technical Specification.
 - Test Object (id: 31024) from the lwm2mclient as described above.

The lightclient does not feature any command-line interface.

Options are:
```
Usage: lwm2mclient [OPTION]
Launch a LWM2M client.
Options:
  -n NAME	Set the endpoint name of the Client. Default: testlightclient
  -l PORT	Set the local UDP port of the Client. Default: 56830
  -4		Use IPv4 connection. Default: IPv6 connection
  -S BYTES	CoAP block size. Options: 16, 32, 64, 128, 256, 512, 1024. Default: 1024
```
### Bootstrap Server example

 * ``cmake -S examples/bootstrap_server -B build-bootstrap``
 * ``cmake --build build-bootstrap``
 * ``./build-bootstrap/bootstrap_server [Options]``

Refer to [examples/bootstrap_server/README](./examples/bootstrap_server/README) for more information.
