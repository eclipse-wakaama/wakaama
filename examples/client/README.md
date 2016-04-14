## Implementation details

The lwm2mclient features nine LWM2M objects:
 - Security Object (id: 0)
 - Server Object (id: 1)
 - Access Control Object (id: 2) as a skeleton
 - Device Object (id: 3) containing hard-coded values from the Example LWM2M
 Client of Appendix E of the LWM2M Technical Specification.
 - Connectivity Monitoring Object (id: 2) as a skeleton
 - Firmware Update Object (id: 5) as a skeleton.
 - Location Object (id: 6) as a skeleton.
 - Connectivity Statistics Object (id: 7) as a skeleton.
 - a test object (id: 1024) with the following description:

                           Multiple
          Object |  ID  | Instances | Mandatoty |
           Test  | 1024 |    Yes    |    No     |

           Ressources:
                       Supported    Multiple
           Name | ID | Operations | Instances | Mandatory |  Type   | Range |
           test |  1 |    R/W     |    No     |    Yes    | Integer | 0-255 |
           exec |  2 |     E      |    No     |    Yes    |         |       |
           dec  |  3 |    R/W     |    No     |    Yes    |  Float  |       |

## Command line options
Options are:
- -n NAME	Set the endpoint name of the Client. Default: testlwm2mclient
- -l PORT	Set the local UDP port of the Client. Default: 56830
- -h HOST	Set the hostname of the LWM2M Server to connect to. Default: localhost
- -p HOST	Set the port of the LWM2M Server to connect to. Default: 5683
- -4		Use IPv4 connection. Default: IPv6 connection
- -t TIME	Set the lifetime of the Client. Default: 300
- -b		Bootstrap requested.
- -c		Change battery level over time.
  
If DTLS feature enable:
- -i Set the device management or bootstrap server PSK identity. If not set use none secure mode
- -s Set the device management or bootstrap server Pre-Shared-Key. If not set use none secure mode

## DTLS
This client can support DTLS, enable by using DTLS=1.
The DTLS feature requires tinyDTLS on the posix platform implementation.
TinyDTLS is included as a GIT submodule and will be configured automatically for you by cmake.
If that fails, you have to make sure that you have installed libtool and autoreconf and you need to run the following commands
in __wakaama/platforms/Linux/tinydtls__:
* git submodule update --init
* autoreconf -i
* ./configure
