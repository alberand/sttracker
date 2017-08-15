#include "mbed.h"

// MBed libs
#include "TCPSocket.h"
#include "SpwfInterface.h"
#include "MyBuffer.h"
#include "BufferedSerial.h"

// Custom libs
#include "utils.h"
#include "minmea.h"
#include "DFRobot_sim808.h"

/* TODO:
 * setup_connection - need to throuw host and port as arguments
 *
 */

#define WIFION 0
// To compile with DEBUG mode on add `-c -DDEBUG=0` to compiler
#define DEBUG 1

#if DEBUG
    #define DEBUGP(x) pc.printf x
#else
    #define DEBUGP(x) do {} while (0)
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

// Global response buffer use for responses from SIM808
MyBuffer <char> sim808_buffer;

/* @brief Interrupt handler stores bytes received from SIM808 module into global
 * buffer.
 */
void SIM808_V2_IRQHandler(void)                    
{    
    // Currently unsued
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
    // char host_ip[] = "192.168.12.1";
    char host_ip[] = "147.32.196.177";
    int host_port = 5000;

    DEBUGP(("Connecting to AP\r\n"));
            
    if(spwf.connect(ssid, seckey, NSAPI_SECURITY_WPA2)) {      
        DEBUGP(("Connected.\r\n"));
    } else {
        DEBUGP(("Error connecting to AP.\r\n"));
        return -1;
    }   

    const char *ip = spwf.get_ip_address();
    const char *mac = spwf.get_mac_address();
    
    DEBUGP(("\r\nIP Address is: %s\r\n", (ip) ? ip : "No IP"));
    DEBUGP(("MAC Address is: %s\r\n", (mac) ? mac : "No MAC"));    
    
    DEBUGP(("\r\nconnecting to http://%s:%d/\r\n", host_ip, host_port));
    
    err = socket -> connect(host_ip, host_port);

    return err;
}

/* @brief Send string to the TCP socket
 *
 * @param str string to send
 * @param size size of the string
 * @param socket TCPSocket instance
 */
int send_string(char * str, size_t size, TCPSocket * socket){
    DEBUGP(("Send: %s\r\n", str)); 

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
void parse_RMC( const char * msg, struct minmea_sentence_rmc * frame )
{
    //struct minmea_sentence_rmc frame;
    if (minmea_parse_rmc(frame, msg)) {
#if DEBUG
   pc.printf("$RMC: raw coordinates and speed: (%d/%d,%d/%d) %d/%d\r\n",
           frame -> latitude.value, frame -> latitude.scale,
           frame -> longitude.value, frame -> longitude.scale,
           frame -> speed.value, frame -> speed.scale);
   pc.printf("$RMC fixed-point coordinates and speed scaled to three decimal places: (%d,%d) %d\r\n",
           minmea_rescale(&frame -> latitude, 1000),
           minmea_rescale(&frame -> longitude, 1000),
           minmea_rescale(&frame -> speed, 1000));
   pc.printf("$RMC floating point degree coordinates and speed: (%f,%f) %f\r\n",
           minmea_tocoord(&frame -> latitude),
           minmea_tocoord(&frame -> longitude),
           minmea_tofloat(&frame -> speed));
#else
   ;
#endif
    }
}

/* @brief Check if command passed and returned "OK". Firstly, wait for 1 second
 * and then look for OK in the response buffer. Also, clears buffer.
 *
 * @return Boolean, True if identical.
 */
int sim808v2_cmd_pass(void)
{
    int status = 1;
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

/* @brief Wait for the NMEA messages starting character. In other words for `$`
 * sign.
 *
 * @param at ATParser instance associated with SIM808
 * @return Received character
 */
char wait_for_msg_start(ATParser * at){
    char ch;

    while((ch = at -> getc())){
        if(ch == '$'){
            return ch;
        }
        // DEBUGP(("Char from SIM808: %c\r\n", ch));
    }

    return '\0';
}

/* @brief This function should be called after `wait_for_msg_start`. Read next 5
 * chars following `$` sign. They represents format of the NMEA message. If
 * header is not equal to `format` then leave buffer empty. Otherwise, add
 * header to the `buffer`.
 *
 * @param at ATParser instance associated with SIM808
 * @param nmeamsg Buffer with NMEA message
 * @param format Name of the format (for example, GPRMC)
 * @return True if header equal to format
 */
bool read_msg_header(ATParser * at, char * nmeamsg, char * format){
    at -> read((nmeamsg + 1), 5);

    if(strcmp((nmeamsg + 1), format) == 0){
        return 1;
    } else{
        for(int i=0; i < 5; i++){
            *(nmeamsg + 1 + i) = '\0';
        }
        return 0;
    }
}
int main() {
    int err, num;    
    green = 0;
    char * token;
    // AP credential
    char ssid[] = "Install Windows 10";
    char seckey[] = "11235813";  
    // Structure to fill in with GPS data
    struct minmea_sentence_rmc frame;
    // Buffer for adding protocol structural symbols
    char msg[BUFFER_SIZE] = {'\0'};
    char rmc[BUFFER_SIZE] = { '\0' };
    char ch;

    ATParser at = ATParser(module, "\r\n");

    // Set up Serial link with PC
    pc.baud(115200);
    // Create TCP socket to connect to the server
    TCPSocket socket(&spwf);

    if(!WIFION){
        DEBUGP(("WiFi is disabled\r\n"));
    }

    // pc.printf("Start GSM setup\r\n"); 

    // int status = setupGSM();

    // pc.printf("GSM status: %s\r\n", status); 

    // return 0;

    if(WIFION){
        err = setup_connection(&socket, ssid, seckey);
    } else{
        err = 0;
    }
    
    if(err != 0) 
    {
        DEBUGP(("Could not connect to Socket, err = %d!!\r\n", err));
        return -1;
    } else {
        if(WIFION){
            DEBUGP(("Connected to host server\r\n"));
        }

        // Initialize GPS module
        if(sim808v2_setup(&at) != 1){
            DEBUGP(("Failed to initialize SIM808 module\r\n")); 
            return -1;
        }

        // Start session
        if(WIFION){
            num = send_string("@42;I;Ver:1a#", 13, &socket);
            DEBUGP(("Sent %d bytes. \r\n", num));
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
                // Wait for data occure in the buffer and read them
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
                        // Print original msg obtained from SIM808
                        pc.printf("Receved from SIM808: %s\r\n", rmc);
                        // Just print some parsed data
                        // This function fill up frame struct with data
                        parse_RMC(rmc, frame);
                        pc.printf("\r\n");
                        for(int erase = 0; erase < BUFFER_SIZE;erase++)
                        {
                              rmc[erase] = '\0';
                        }
                    }
                }

                /* strcpy(msg, "@42;T;");
                token = strtok(buffer, "\n");
                token = strtok(NULL, "\n");
                strncat(msg, token, 255);
                strcat(msg, "#");
                num = send_string(msg, 67, &socket);*/
            }
            // DEBUGP(("Sent %d bytes. \r\n", num));
            // wait(0.1);
        }
    }
    
    // For now unreachable
    if(WIFION){
        DEBUGP(("Closing Socket\r\n"));
        socket.close();
        DEBUGP(("Unsecure Socket Test complete.\r\n"));
        DEBUGP(("Socket closed\n\r"));
        spwf.disconnect();
        DEBUGP(("WIFI disconnected, exiting ...\n\r"));
    }

}

