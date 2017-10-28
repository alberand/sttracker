#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "mbed.h"

#define WIFION 0
#define GPSREPORTING 1

// To compile without DEBUG mode add `-c -DDEBUG=0` to compiler
#define DEBUG 1

#if DEBUG
    #define DEBUGP(x) pc.printf x
#else
    #define DEBUGP(x) do {} while (0)
#endif

extern Serial pc;

// AP credential
extern char ssid[];
extern char seckey[];  

// IP address and port of the server
extern char host_ip[];
extern int host_port;

#endif // __CONFIG_H__
