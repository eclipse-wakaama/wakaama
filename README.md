Wakaama (formerly liblwm2m) is an implementation of the OMA LightWeight M2M
protocol (LWM2M) in C suitable for gateway devices as well as embedded devices.

Lightweight M2M is a protocol from the Open Mobile Alliance for M2M or IoT device management
and communication. It uses CoAP, a light and compact protocol with an efficient resource data model,
for the application layer communication between LWM2M Servers and LWM2M Clients.

Developers mailing list: https://dev.eclipse.org/mailman/listinfo/wakaama-dev

## Compiling
Wakaama is not a library but files to be built with your application.
The integration is very easy if you also use the cmake buildsystem. Please
have a look at one of the CMakeLists.txt files in the __examples__ directory.

Compilation switches for enabling the server/client/bootstrap implementations:
 - ``LWM2M_CLIENT_MODE`` to enable LWM2M Client interfaces.
 - ``LWM2M_SERVER_MODE`` to enable LWM2M Server interfaces.
 - ``LWM2M_BOOTSTRAP_SERVER_MODE`` to enable LWM2M Bootstrap Server interfaces.
 - ``LWM2M_BOOTSTRAP`` to enable LWM2M Bootstrap support in a LWM2M Client.
You cannot compile a library with bootstrap client and server support!

Compilation switches for additional features:
 - ``LWM2M_SUPPORT_JSON`` to enable JSON payload support (implicit when defining LWM2M_SERVER_MODE)

Automatically determined compilation flags:
 - ``LWM2M_LITTLE_ENDIAN`` if your target platform uses little-endian format. CMake determines this
   automatically, you may overwrite this in a cross compilation file. If you do not use cmake, you have
   define this if appropriate.

## Network stack and platform integration
Wakaama does not assume any specific network stack or platform calls to be present. You have to provide
some platform abstraction methods and implement the glue code for your network stack yourself instead.

Look at the __platforms__ directory for examples.

## Examples
There are some example applications provided to test the server, client and bootstrap capabilities of Wakaama.
The following recipes assume you are on a unix like platform, you have cmake and make installed and your
current directory is the wakaama repository root. Some of the examples also compile for Windows systems.

### Server example
Unix with make:
 * ``mkdir build && cd build``
 * ``cmake ../examples/server``
 * ``make``
 * ``./lwm2mserver [Options]``

The lwm2mserver listens on UDP port 5683. It features a basic command line
interface. Type 'help' for a list of supported commands.

Options are:
 - -4		Use IPv4 connection. Default: IPv6 connection


### Client example
Unix with make:
 * ``mkdir build && cd build``
 * ``cmake -DDTLS=0|1 ../examples/client`` Set DTLS to 1 if you want this feature.
 * ``make``
 * ``./lwm2mclient [Options]``

The DTLS feature requires tinydtls. CMake will try to download and configure tinydtls for you.

The lwm2mclient opens udp port 56830 and tries to register to a LWM2M Server at
127.0.0.1:5683 (if IPv4 enabled). It features a basic command line interface. Type 'help' for a
list of supported commands. To launch a bootstrap session: ``./lwm2mclient -b``

Important options are:
 - -4		Use IPv4 connection. Default: IPv6 connection

Further implementation, command line and tinydtls details are documented in [[examples/client/README.md]]

### Simpler client example
Unix with make:
 * ``mkdir build && cd build``
 * ``cmake ../examples/lightclient``
 * ``make``
 * ``./lightclient [Options]``

If you use Visual Studio 2010 or newer on a Windows operating system:
 - Open CMake and choose [liblwm2m directory]/examples/lightclient as source dir and an arbitrary directory as build dir.
 - Choose your VS compiler in the popoup dialog,
 - Click __configure__ and __generate__ and open the ``.sln`` project file in the build directory.

Important options are:
 - -4		Use IPv4 connection. Default: IPv6 connection

Further implementation details are documented in [[examples/lightclient/README.md]]

## Source Layout
    -+- core                   (the LWM2M engine)
     |    |
     |    +- er-coap-13        (Erbium's CoAP engine from
     |                          http://people.inf.ethz.ch/mkovatsc/erbium.php, modified
     |                          to run on linux)
     |
     +- platforms              (example ports on various platforms)
     |
     +- tests                  (test cases)
     |
     +- examples
          |
          +- bootstrap_server  (a command-line LWM2M bootstrap server)
          |
          +- client            (a command-line LWM2M client with several test objects)
          |
          +- lightclient       (a very simple command-line LWM2M client with several test objects)
          |
          +- misc              (application unit-testing miscellaneous utility functions of the core)
          |
          +- server            (a command-line LWM2M server)
          |
          +- utils             (utility functions for connection handling and command-
                                line interface)

