#include "mbed.h"

#include "config.h"
#include "networking.h"

int setup_connection(TCPSocket * socket, SpwfSAInterface * spwf, 
        char * ssid, char * seckey, char * host_ip, uint16_t host_port)
{
    int err;

    DEBUGP(("Connecting to AP\r\n"));
            
    if(spwf -> connect(ssid, seckey, NSAPI_SECURITY_WPA2)) {      
        DEBUGP(("Connected.\r\n"));
    } else {
        DEBUGP(("Error connecting to AP.\r\n"));
        return -1;
    }   

    const char *ip = spwf -> get_ip_address();
    const char *mac = spwf -> get_mac_address();
    
    DEBUGP(("\r\nIP Address is: %s\r\n", (ip) ? ip : "No IP"));
    DEBUGP(("MAC Address is: %s\r\n", (mac) ? mac : "No MAC"));    
    
    DEBUGP(("\r\nConnecting to http://%s:%d/\r\n", host_ip, host_port));
    
    err = socket -> connect(host_ip, host_port);

    return err;
}

int send_string(char * str, size_t size, TCPSocket * socket){
    DEBUGP(("Sending: %s; ", str)); 

    // strcpy(buf, str);
    int num = socket -> send(str, size);

    DEBUGP(("[Sent: %d]\r\n", num)); 

    return num;
}
