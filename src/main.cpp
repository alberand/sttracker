#include "mbed.h"

#include "TCPSocket.h"
#include "SpwfInterface.h"

#include "utils.h"

//#define DEBUG 0

// "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A"
// 1,1,20170717100030.000,50.080988,14.388322,82.200,

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
// Init serial with SIM808_V2, Serial3 for STM32 Nucleo L476G
Serial module(A0, A1);

// Informing LED
DigitalOut green(LED1);
// Enable button
DigitalIn mybutton(USER_BUTTON);

// Interface to the WiFi module
SpwfSAInterface spwf(D8, D2, false);

// Global response buffer
int buf_ind = 0;
char buffer[BUFFER_SIZE];

/* @brief Interrupt handler stores bytes received from SIM808 module into global
 * buffer.
 */
void SIM808_V2_IRQHandler(void)                    
{    
    char ch = module.getc();

    buffer[buf_ind] = ch;
    buf_ind++; 
} 

/* @brief Write command `cmd` to the serial port `sr` and add newline '\n'.
 *
 * @param cmd Command to send.
 * @param sr mbed Serial object connected to SIM808_V2 module.
 */
void sim808v2_send_cmd(const char * cmd, Serial * sr)
{
    sr -> puts(cmd);
    sr -> puts("\n");
}

/* @brief Clear global buffer.
 *
 * Clear global buffer by filling it with zeros and returning buffer index to
 * the starting position.
 */
void sim808v2_clear_buffer(void)
{
    uint16_t i;
    for(i = 0; i < BUFFER_SIZE; i++)
    {
        buffer[i] = 0x00;
    }
    buf_ind = 0;
}

/* @brief Prints buffer content. Used for printing SIM808_V2's response to
 * commands.
 *
 */
void sim808v2_print_buffer(void)
{
    pc.printf("Response: %s\r\n", buffer);
    sim808v2_clear_buffer(); 
}

/* @brief Make `num` of blinks by LED `green`.
 *
 * @param num Number of blinks
 */
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
    
    err = socket -> connect("192.168.12.1", 5000);
    // err = socket -> connect("147.32.196.177", 5000);

    return err;
}

int send_string(char * str, size_t size, TCPSocket * socket){
    pc.printf("Send: %s\r\n", str); 

    // strcpy(buf, str);
    int num = socket -> send(str, size);

    return num;
}

/* @brief Check if command passed and returned "OK".
 *
 * @return Boolean, True if identical.
 */
int sim808v2_cmd_pass(void)
{
    char * token;
    wait(1);
    token = strtok(buffer, "\n");
    token = strtok(NULL, "\n");
    int status = strncmp(token, "OK", 2);

    return status == 0;
}

/* @brief Prepare serial link and SIM808 V2 module for work (mainly for GPS
 * data).
 *
 * @return Boolean in case of succesful intialization.
 */
int sim808v2_setup(void)
{
    int status = 1;
    // Configure connection to the SIM808 module
    module.baud(9600);
    module.attach(&SIM808_V2_IRQHandler, SerialBase::RxIrq);

    // Check status
    sim808v2_send_cmd("AT", &module);
    status = sim808v2_cmd_pass();
    sim808v2_clear_buffer();

    // Power on GPS module
    sim808v2_send_cmd("AT+CGNSPWR=1", &module);
    status = sim808v2_cmd_pass();
    sim808v2_clear_buffer();

    // Set NMEA format
    sim808v2_send_cmd("AT+CGNSSEQ=RMC", &module);
    status = sim808v2_cmd_pass();
    sim808v2_clear_buffer();

    return status;
}

int main() {
    int err, num;    
    green = 0;
    char * token;
    char * ssid = "Install Windows 10";
    char * seckey = "11235813";  
    char msg[BUFFER_SIZE] = {'\0'};
    pc.baud(115200);
    TCPSocket socket(&spwf);

    // Initialize GPS module. Note: Don't know why, but SIM808 initialization
    // should be before connection to socket.
    if(sim808v2_setup() != 1){
        pc.printf("Failed to initialize SIM808 module\r\n"); 
        return -1;
    }

    err = setup_connection(&socket, ssid, seckey);
    
    if(err != 0) 
    {
        pc.printf("Could not connect to Socket, err = %d!!\r\n", err); 
        return -1;
    } else {
        pc.printf("Connected to host server\r\n"); 

        // Start session
        num = send_string("@42;I;Ver:1a#", 13, &socket);
        DEBUG(("Sent %d bytes. \r\n", num));

        sim808v2_print_buffer();

        while(1) { 
            if (mybutton == 0){
                // Button Pressed
                wait(0.1);
                n_blinks(2);
                num = send_string("@42;T;Button is pressed#", 24, &socket);
            }
            else{
                sim808v2_send_cmd("AT+CGNSINF", &module);
                wait(1);
                strcpy(msg, "@42;T;");
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
    
    pc.printf("\r\nClosing Socket\r\n");
    socket.close();
    pc.printf("\r\nUnsecure Socket Test complete.\r\n");
    printf ("Socket closed\n\r");
    spwf.disconnect();
    printf ("WIFI disconnected, exiting ...\n\r");

}

