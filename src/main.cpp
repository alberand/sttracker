#include "mbed.h"

// MBed libs
#include "TCPSocket.h"
#include "SpwfInterface.h"
#include "MyBuffer.h"
#include "BufferedSerial.h"

// Custom libs
#include "utils.h"
#include "minmea.h"

#include "config.h"
#include "networking.h"
#include "sim808.h"
/* TODO:
 * setup_connection - need to throuw host and port as arguments
 *
 */

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

/* @brief Wait for data from SIM808. When occured message with `format` cut it
 * off and put into nmeamsg. TODO: For now it works only for RMC (due to
 * structure in the paramters).
 *
 * @param at ATParser attached to SIM808 module
 * @param nmeamsg Buffer where NMEA message will be put
 * @param format Format of the message which is requred. For exmaple: GPRMC
 * @param frame minmea_sentence_rmc sturcutre which will be filled with data
 *
 */
void wait_for_sample(ATParser * at, char * nmeamsg, char * format, 
        struct minmea_sentence_rmc * frame){
    while(1){
        char ch = wait_for_msg_start(at);
        nmeamsg[0] = ch;
        if(read_msg_header(at, nmeamsg, format)){
            // Read msg until newline
            ch = at -> getc();
            while(ch != '\n'){
                append(nmeamsg, ch);
                ch = at -> getc();
            }

            // Print original msg obtained from SIM808
            pc.printf("Receved from SIM808: %s\r\n", nmeamsg);
            // Just print some parsed data
            // This function fill up frame struct with data
            parse_RMC(nmeamsg, frame);
            pc.printf("\r\n");
            for(int erase = 0; erase < BUFFER_SIZE;erase++)
            {
                  nmeamsg[erase] = '\0';
            }
            
        }
    }


}

/* @brief Send SIM808 module AT command and wait for OK. If OK received returns
 * 1, 0 otherwise.
 *
 * @param ATParser attached to the SIM808 module
 */
bool sim808_is_active(ATParser * at){
    int status = 0;
    status = at -> send("AT") && at -> recv("OK");
    if(status != 0){
        // DEBUGP(("AT is OK\r\n"));
        return 1;
    }

    return 0;
}

int sim808_GSM_rssi(ATParser * at){
    int rssi, ber, status = 0;
    status = at -> send("AT+CSQ") && at -> recv("+CSQ: %d,", &rssi, &ber);

    return rssi;
}

int sim808_GSM_ber(ATParser * at){
    int rssi, ber, status = 0;
    status = at -> send("AT+CSQ") && at -> recv("+CSQ: %d, %d", &rssi, &ber);
    if(status != 0){
        return ber;
    }
}
int main() {
void echo_mode(ATParser * at, Serial * pc){
    int8_t buffer = 0;
    at -> setTimeout(1);
    while(1){
        if(pc -> readable()){
            buffer = pc -> getc();
            at -> putc(buffer);
        }

        if((buffer = at -> getc()) != -1){
            pc -> putc(buffer);
        }
    }
}
    int err, num;    
    int err;    
    green = 0;
    // AP credential
    char ssid[] = "Install Windows 10";
    char seckey[] = "11235813";  
    // Server address
    // char host_ip[] = "192.168.12.1";
    char host_ip[] = "147.32.196.177";
    uint16_t host_port = 5000;
    // Structure to fill in with GPS data
    struct minmea_sentence_rmc frame;
    // Buffer for adding protocol structural symbols

    char format[7] = "GPRMC\0";
    char nmeamsg[BUFFER_SIZE] = { '\0' };

    // Set up Serial link with PC
    pc.baud(115200);

    DEBUGP(("==========================================================\r\n"));
    DEBUGP(("Module start working.\r\n"));
    DEBUGP(("==========================================================\r\n"));

    // Parser for SIM808 messages
    ATParser at = ATParser(module, "\r\n");

    // Create TCP socket to connect to the server
    TCPSocket socket(&spwf);

    if(!WIFION){
        DEBUGP(("WiFi is disabled\r\n"));
    } else {
        DEBUGP(("WiFi is enabled\r\n"));
    }

    if(sim808_is_active(&at) == 1){
        DEBUGP(("SIM808 is active\r\n"));
    } else {
        DEBUGP(("SIM808 is not active\r\n"));
    }

    // pc.printf("Start GSM setup\r\n"); 

    // int status = setupGSM();

    // pc.printf("GSM status: %s\r\n", status); 

    // return 0;

    if(WIFION){
        err = setup_connection(&socket, ssid, seckey, host_ip, host_port);
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
        if(sim808v2_GPS_setup(&at) != 1){
            DEBUGP(("Failed to initialize GPS module of the SIM808 \r\n")); 
            return -1;
        }

        // Start session
        if(WIFION){
            send_string("@42;I;Ver:1a#", 13, &socket);
        }

        while(1) { 
            if (mybutton == 0){
                // Button Pressed
                wait(0.1);
                n_blinks(2, &green);
                if(WIFION){
                    send_string("@42;T;Button is pressed#", 24, &socket);
                }
            }
            else{
                // Wait for data occure in the buffer and read them
                // wait_for_sample(&at, nmeamsg, format, &frame);
                // makeCall(&at);
                echo_mode(&at, &pc);

                /* strcpy(msg, "@42;T;");
                token = strtok(buffer, "\n");
                token = strtok(NULL, "\n");
                strncat(msg, token, 255);
                strcat(msg, "#");
                send_string(msg, 67, &socket);*/
            }
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

