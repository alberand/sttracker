#include "mbed.h"

#include "TCPSocket.h"
#include "SpwfInterface.h"

//------------------------------------
// Hyperterminal configuration
// 9600 bauds, 8-bit data, no parity
//------------------------------------
Serial pc(USBTX, USBRX);
DigitalOut green(LED1);
DigitalIn mybutton(USER_BUTTON);

SpwfSAInterface spwf(D8, D2, false);

char buf[255] = {'\0'};

void n_blinks(int num){
    for (int i = 0; i < num; i++) {
        green = 1;
        wait(0.2);
        green = 0;
        wait(0.2);
    }
}

int setup_connection(TCPSocket * socket, char * ssid, char * seckey){
    int err;

    pc.printf("Connecting to AP\r\n");
            
    if(spwf.connect(ssid, seckey, NSAPI_SECURITY_WPA2)) {      
        pc.printf("Connected.\r\n");
    } else {
        pc.printf("Error connecting to AP.\r\n");
        return -1;
    }   

    const char *ip = spwf.get_ip_address();
    const char *mac = spwf.get_mac_address();
    
    pc.printf("\r\nIP Address is: %s\r\n", (ip) ? ip : "No IP");
    pc.printf("MAC Address is: %s\r\n", (mac) ? mac : "No MAC");    
    
    pc.printf("\r\nconnecting to http://147.32.196.177:5000/\r\n");
    
    // err = socket.connect("192.168.12.1", 5000);
    err = socket -> connect("147.32.196.177", 5000);

    return err;
}

int send_string(char * str, size_t size, TCPSocket * socket){
    pc.printf("Send: %s\r\n", str); 

    // strcpy(buf, str);
    int num = socket -> send(str, size);

    return num;
}

int main() {
    int err, num;    
    green = 0;
    char * ssid = "Install Windows 10";
    char * seckey = "11235813";  
    TCPSocket socket(&spwf);

    err = setup_connection(&socket, ssid, seckey);
    
    if(err != 0) 
    {
        pc.printf("Could not connect to Socket, err = %d!!\r\n", err); 
        return -1;
    } else {
        pc.printf("Connected to host server\r\n"); 

        // Start session
        num = send_string("@42;I;Ver:1a#", 13, &socket);

        while(1) { 
            if (mybutton == 0){
                // Button Pressed
                wait(0.1);
                n_blinks(2);
                num = send_string("@42;T;Button is pressed#", 24, &socket);
            }
            else{
                num = send_string("@42;D;25.03.17;16:36:27.9;4954.70068N;1447.00391E;0.0;204.4;551.45;19;11;34.0;Vodafone CZ;4G;3332.72876;3744.53149;8571.24219#", 126, &socket);
            }
            pc.printf("Sent %d bytes. Button status: %d.\r\n", num, mybutton);
            wait(0.1);
        }
    }
    
    pc.printf("\r\nClosing Socket\r\n");
    socket.close();
    pc.printf("\r\nUnsecure Socket Test complete.\r\n");
    printf ("Socket closed\n\r");
    spwf.disconnect();
    printf ("WIFI disconnected, exiting ...\n\r");

}

