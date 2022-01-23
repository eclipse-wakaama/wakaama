#include <stdint.h>

#include "liblwm2m.h"

void *lwm2m_connect_server(uint16_t secObjInstID, void *userData) {
    (void)userData;
    return (void *)(uintptr_t)secObjInstID;
}

void lwm2m_close_connection(void *sessionH, void *userData) {
    (void)sessionH;
    (void)userData;
    return;
}

int main(void) {}
