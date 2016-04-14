## Implementation details

The lightclient features the mandatory LWM2M objects 0, 1 and 3 and additionally a custom object:
 - Security Object (id: 0)
 - Server Object (id: 1)
 - Device Object (id: 3) containing hard-coded values from the Example LWM2M
 Client of Appendix E of the LWM2M Technical Specification.
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
 - -n NAME	Set the endpoint name of the Client. Default: testlightclient
 - -l PORT	Set the local UDP port of the Client. Default: 56830
 - -4		Use IPv4 connection. Default: IPv6 connection

