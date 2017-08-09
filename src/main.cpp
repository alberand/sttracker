#include "mbed.h"

// MBed libs
#include "TCPSocket.h"
#include "SpwfInterface.h"
#include "MyBuffer.h"
#include "BufferedSerial.h"

// Custom libs
#include "utils.h"
#include "minmea.h"


// "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
// 1,1,20170717100030.000,50.080988,14.388322,82.200,

#define WIFION 0
//#define DEBUG 0
#ifdef DEBUG
    #define DEBUG(x) pc.printf x
#else
    #define DEBUG(x) do {} while (0)
#endif

/* @brief Maximum size of the buffer used for obtaining SIM808 response to the
 * commands.
 */
#define BUFFER_SIZE 255

// Init serial with PC
Serial pc(USBTX, USBRX);
// Init serial with SIM808_V2, Serial4 for STM32 Nucleo L476G
BufferedSerial module(A0, A1);

// Informing LED
DigitalOut green(LED1);
// Enable button
DigitalIn mybutton(USER_BUTTON);

// Interface to the WiFi module
SpwfSAInterface spwf(D8, D2, false);

// Global response buffer
MyBuffer <char> sim808_buffer;

/* @brief Interrupt handler stores bytes received from SIM808 module into global
 * buffer.
 */
void SIM808_V2_IRQHandler(void)                    
{    
    char ch = module.getc();

    sim808_buffer.put(ch);
}

/* @brief Write command `cmd` to the serial port `sr` and add newline '\n'.
 *
 * @param cmd Command to send.
 * @param sr mbed Serial object connected to SIM808_V2 module.
 */
void sim808v2_send_cmd(const char * cmd, BufferedSerial * sr)
{
    sr -> puts(cmd);
    sr -> puts("\n");
}

int setup_connection(TCPSocket * socket, char * ssid, char * seckey){
    int err;
    // char * host_ip = "192.168.12.1";
    char * host_ip = "147.32.196.177";
    int host_port = 5000;

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
    
    pc.printf("\r\nconnecting to http://%s:%d/\r\n", host_ip, host_port);
    
    err = socket -> connect(host_ip, host_port);

    return err;
}

int send_string(char * str, size_t size, TCPSocket * socket){
    pc.printf("Send: %s\r\n", str); 

    // strcpy(buf, str);
    int num = socket -> send(str, size);

    return num;
}

/* @brief Take a string and parse it into struct 
 *
 * Take string in GPS fromat RMC and parse it into struct using Minmea library.
 *
 * @param string to parse
 * @param struct to fill in
 */
void parse_RMC( const char * msg, struct minmea_sentence_rmc frame )
{
    //struct minmea_sentence_rmc frame;
    if (minmea_parse_rmc(&frame, msg)) {
        pc.printf("$RMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\r\n",
                frame.latitude.value, frame.latitude.scale,
                frame.longitude.value, frame.longitude.scale,
                frame.speed.value, frame.speed.scale);
        pc.printf("$RMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\r\n",
                minmea_rescale(&frame.latitude, 1000),
                minmea_rescale(&frame.longitude, 1000),
                minmea_rescale(&frame.speed, 1000));
        pc.printf("$RMC floating point degree coordinates and speed: (%f,%f) %f\r\n",
                minmea_tocoord(&frame.latitude),
                minmea_tocoord(&frame.longitude),
                minmea_tofloat(&frame.speed));
    }
}

/* @brief Check if command passed and returned "OK". Firstly, wait for 1 second
 * and then look for OK in the response buffer. Also, clears buffer.
 *
 * @return Boolean, True if identical.
 */
int sim808v2_cmd_pass(void)
{
    int status;
    int pos = 0;
    char response[2];

    // Wait 
    wait(1);

    while(sim808_buffer.available())
    {
        response[0] = sim808_buffer.get();
        response[1] = sim808_buffer.get();
        if(!(status = strncmp(response, "OK", 2))){
            break;
        }
    }

    return status == 0;
}

/* @brief Prepare serial link and SIM808 V2 module for work (mainly for GPS
 * data).
 *
 * @return Boolean in case of succesful intialization.
 */
int sim808v2_setup(ATParser * at)
{
    int status = 1;
    // Configure connection to the SIM808 module
    // module.baud(9600);
    // module.attach(&SIM808_V2_IRQHandler, SerialBase::RxIrq);

    // Check status
    status = at -> send("AT") && at -> recv("OK");
    // sim808v2_send_cmd("AT", &module);
    // status = sim808v2_cmd_pass();
    // sim808_buffer.clear();

    // Power on GPS module
    status = at -> send("AT+CGNSPWR=1") && at -> recv("OK");
    // sim808v2_send_cmd("AT+CGNSPWR=1", &module);
    // status = sim808v2_cmd_pass();
    // sim808_buffer.clear();

    // Set NMEA format
    status = at -> send("AT+CGNSSEQ=RMC") && at -> recv("OK");
    // sim808v2_send_cmd("AT+CGNSSEQ=RMC", &module);
    // status = sim808v2_cmd_pass();
    // sim808_buffer.clear();

    // Turn on reporting
    status = at -> send("AT+CGNSTST=1") && at -> recv("OK");
    // sim808v2_send_cmd("AT+CGNSTST=0", &module);
    // status = sim808v2_cmd_pass();
    // sim808_buffer.clear();

    return status;
}

int main() {
    int err, num;    
    green = 0;
    char * token;
    char * ssid = "Install Windows 10";
    char * seckey = "11235813";  
    // Structure to fill in with GPS data
    struct minmea_sentence_rmc frame;
    // Buffer for adding protocol structural symbols
    char msg[BUFFER_SIZE] = {'\0'};
    char rmc[BUFFER_SIZE] = { '\0' };
    char ch;

    ATParser at = ATParser(module, "\r\n");
    pc.baud(115200);
    TCPSocket socket(&spwf);

    // Initialize GPS module. Note: Don't know why, but SIM808 initialization
    // should be before connection to socket.
    if(sim808v2_setup() != 1){
        pc.printf("Failed to initialize SIM808 module\r\n"); 
        return -1;
    }

    if(WIFION){
        err = setup_connection(&socket, ssid, seckey);
    } else{
        err = 0;
    }
    
    if(err != 0) 
    {
        pc.printf("Could not connect to Socket, err = %d!!\r\n", err); 
        return -1;
    } else {
        pc.printf("Connected to host server\r\n"); 

        // Start session
        num = send_string("@42;I;Ver:1a#", 13, &socket);
        DEBUG(("Sent %d bytes. \r\n", num));

        // Start session
        if(WIFION){
            num = send_string("@42;I;Ver:1a#", 13, &socket);
            DEBUG(("Sent %d bytes. \r\n", num));
        }

        while(1) { 
            if (mybutton == 0){
                // Button Pressed
                wait(0.1);
                n_blinks(2, &green);
                if(WIFION){
                    num = send_string("@42;T;Button is pressed#", 24, &socket);
                }
            }
            else{
                sim808v2_send_cmd("AT+CGNSINF", &module);
                wait(1);
                strcpy(msg, "@42;T;");
                ch = at.getc();
                if (ch == '$'){
                    rmc[0] = ch;
                    at.read((rmc + 1), 5);

                    if(strcmp((rmc + 1), "GPRMC") == 0){
                        ch = at.getc();
                        while(ch != '\n'){
                            append(rmc, ch);
                            ch = at.getc();
                        }
                        pc.printf("%s\r\n", rmc);
                        parse_RMC(rmc, frame);
                        pc.printf("\r\n");
                        for(int erase = 0; erase < BUFFER_SIZE;erase++)
                        {
                              rmc[erase] = '\0';
                        }
                    }
                }
                token = strtok(buffer, "\n");
                token = strtok(NULL, "\n");
                strncat(msg, token, 60);
                strcat(msg, "#");
                num = send_string(msg, 67, &socket);
                clear_buffer();
            }
            DEBUG(("Sent %d bytes. \r\n", num));
            wait(0.1);
        }
    }
    
    if(WIFION){
        pc.printf("Closing Socket\r\n");
        socket.close();
        pc.printf("Unsecure Socket Test complete.\r\n");
        printf ("Socket closed\n\r");
        spwf.disconnect();
        printf ("WIFI disconnected, exiting ...\n\r");
    }

}

