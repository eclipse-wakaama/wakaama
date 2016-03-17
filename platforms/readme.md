# Platforms
A platform directory (for example "Linux") have to implement the following methods:

* lwm2m_malloc(size_t s)
* void lwm2m_free(void * p)
* char * lwm2m_strdup(const char * str)
* int lwm2m_strncmp(const char * s1, const char * s2,  size_t n)
* time_t lwm2m_gettime(void)
* void lwm2m_printf(const char * format, ...)

## Requirements for the examples directory

To make the examples directory compile on your platform,
your platform has to provide a bsd socket API including select(), FD_ZERO(), FD_SET() and FD_ISSET()
and you also have to implement the following methods:

* int create_socket(const char * portStr, int ai_family);
* connection_t * connection_find(connection_t * connList, struct sockaddr_storage * addr, size_t addrLen);
* connection_t * connection_new_incoming(connection_t * connList, int sock, struct sockaddr * addr, size_t addrLen);
* connection_t * connection_create(connection_t * connList, int sock, char * host, char * port, int addressFamily);
* void connection_free(connection_t * connList);
* int connection_send(connection_t *connP, uint8_t * buffer, size_t length);


