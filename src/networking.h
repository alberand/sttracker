#ifndef __NETWORKING_H__
#define __NETWORKING_H__

#include "TCPSocket.h"
#include "SpwfInterface.h"
#include "MyBuffer.h"
#include "BufferedSerial.h"

#define HTTP_SERVER_PORT "80"

/* @brief Setup connection to the WiFi access point
 *
 * @param
 * @param
 * @param
 * @param
 * @param
 *
 * @return Error code
 */
int setup_connection(TCPSocket * socket, SpwfSAInterface * spwf, 
        char * ssid, char * seckey, char * host_ip, uint16_t host_port);

/* @brief Send string to the TCP socket
 *
 * @param str string to send
 * @param size size of the string
 * @param socket TCPSocket instance
 *
 * @return Number of sent bytes
 */
int send_string(char * str, size_t size, TCPSocket * socket);

#endif // __NETWORKING_H__
