#include "mbed.h"

#include "TCPSocket.h"
#include "SpwfInterface.h"


//------------------------------------
// Hyperterminal configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(USBTX, USBRX);
DigitalOut myled(LED1);

SpwfSAInterface spwf(D8, D2, false);

int main() {
    int err;    
    char * ssid = "Install Windows 10";
    char * seckey = "11235813";  
    
    pc.printf("\r\nconnecting to AP\r\n");
            
    if(spwf.connect(ssid, seckey, NSAPI_SECURITY_WPA2)) {      
        pc.printf("\r\nnow connected\r\n");
    } else {
        pc.printf("\r\nerror connecting to AP.\r\n");
        return -1;
    }   

    const char *ip = spwf.get_ip_address();
    const char *mac = spwf.get_mac_address();
    
    pc.printf("\r\nIP Address is: %s\r\n", (ip) ? ip : "No IP");
    pc.printf("\r\nMAC Address is: %s\r\n", (mac) ? mac : "No MAC");    
    
    pc.printf("\r\nconnecting to http://192.168.12.1:5000/\r\n");
    
    TCPSocket socket(&spwf);
    // err = socket.connect("192.168.12.1", 5000);
    err = socket.connect("147.32.196.177", 5000);
    if(err != 0) 
    {
        pc.printf("\r\nCould not connect to Socket, err = %d!!\r\n", err); 
        return -1;
    } else {
        pc.printf("\r\nconnected to host server\r\n"); 
        char buf[] = "@42;I;Ver:1a#";
        int num = socket.send(buf, sizeof(buf) - 1);
        while(1) { 
            wait(1);
            myled = !myled;
            char buf[] = "@42;D;25.03.17;16:36:27.9;4954.70068N;1447.00391E;0.0;204.4;551.45;19;11;34.0;Vodafone CZ;4G;3332.72876;3744.53149;8571.24219#";
            int num = socket.send(buf, sizeof(buf) - 1);
            pc.printf("Sent %d bytes.\r\n", num);
        }
    }
    
    /* char buffer[100];
    int count = 0;
    pc.printf("\r\nReceiving Data\r\n"); 
    count = socket.recv(buffer, sizeof buffer);
    
    if(count > 0)
    {
        buffer [count]='\0';
        printf("%s\r\n", buffer);  
    }
    else pc.printf("\r\nData not received\r\n");
    */

    pc.printf("\r\nClosing Socket\r\n");
    socket.close();
    pc.printf("\r\nUnsecure Socket Test complete.\r\n");
    printf ("Socket closed\n\r");
    spwf.disconnect();
    printf ("WIFI disconnected, exiting ...\n\r");

}

