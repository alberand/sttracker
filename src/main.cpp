#include "mbed.h"

// MBed libs
#include <TCPSocket.h>
#include <SpwfInterface.h>
#include <MyBuffer.h>
#include <BufferedSerial.h>
//#include "mongoose.h"

// Custom libs
#include <minmea.h>

#include "config.h"
#include "networking.h"
#include "utils.h"
#include "sim808.h"

/* TODO:
 * setup_connection - need to throuw host and port as arguments
 *
 */

/* @brief Maximum size of the buffer used for obtaining SIM808 response to the
 * commands.
 */
#define BUFFER_SIZE 255

#define MODULE_ID 42

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

/* @brief Format data into packet's string 
 *
 * @param pointer to string buffer
 * @param structure containing GPS data
 */
void format_packet(char * packet_s, struct minmea_sentence_rmc * frame_rmc,
        struct minmea_sentence_gga * frame_gga){
    // Add packet header
    char buf[20] = { '\0' };
    sprintf(packet_s, 
            "@%d;D;%02d.%02d.%02d;%02d:%02d:%02d.%d;%f;%f;%f;%f;%f;%d;%d#", 
            MODULE_ID,
            frame_rmc -> date.day, frame_rmc -> date.month, 
            frame_rmc -> date.year,
            frame_rmc -> time.hours, frame_rmc -> time.minutes, 
            frame_rmc -> time.seconds, frame_rmc -> time.microseconds/10000,
            frame_rmc -> latitude.value,
            frame_rmc -> longitude.value,
            frame_rmc -> speed.value,
            frame_rmc -> course.value,
            frame_gga -> altitude.value,
            frame_gga -> satellites_tracked,
            frame_gga -> fix_quality
    );
}

/* @brief Wait for data from SIM808. When occured message with `format` cut it
 * off and put into nmeamsg. TODO: For now it works only for RMC (due to
 * structure in the paramters).
 *
 * @param at ATParser attached to SIM808 module
 * @param nmeamsg Buffer where NMEA message will be put
 * @param format Format of the message which is requred. For exmaple: GPRMC
 * @param frame minmea_sentence_rmc sturcutre which will be filled with data
 * @return boolean True if package read
 */
int wait_for_sample(ATParser * at, char * packet){
    char ch = '\0';
    char nmeamsg[MINMEA_MAX_LENGTH] = { '\0' };
    // We need both format because they contain different data
    struct minmea_sentence_rmc frame_rmc;
    struct minmea_sentence_gga frame_gga;

    int parsed = 0;
    while(1){
        // Read until newline
        memset(nmeamsg, 0, sizeof(char)*MINMEA_MAX_LENGTH);

        ch = at -> getc();
        while(ch != '\r'){
            if(ch == '\n'){
                ch = at -> getc();
                continue;
            }
            append(nmeamsg, ch);
            ch = at -> getc();
        }

        // DEBUGP(("Receved from SIM808: %s\r\n", nmeamsg));

        switch(minmea_sentence_id(nmeamsg, false)){
            case MINMEA_SENTENCE_RMC:
                // Parse RMC to frame_rmc
                // DEBUGP(("Parsing RMC: %s\r\n", nmeamsg));
                minmea_parse_rmc(&frame_rmc, nmeamsg);
                parsed++;
                break;

            case MINMEA_SENTENCE_GGA:
                // Parse GGA to frame_gga
                // DEBUGP(("Parsing GGA: %s\r\n", nmeamsg));
                minmea_parse_gga(&frame_gga, nmeamsg);
                parsed++;
                break;

            case MINMEA_INVALID:
                // DEBUGP(("Invalid line\r\n"));
                break;

            default:
                if(parsed == 2){
                    goto exit_loop;
                }
                continue;
        }
    }

    exit_loop: ;

    format_packet(packet, &frame_rmc, &frame_gga);

    return 1;
}

/* @brief Function which contains main loop with data processing. Curruntly is
 * used for testing
 */
int mainloop(TCPSocket * socket, ATParser * at){
    int status, is_connected = 0;    
    char packet[BUFFER_SIZE];

    // Turn off LED
    green = 0;

    // Start session
    if(WIFION){
        send_string("@42;I;Ver:1a#", 13, socket);
    }

    while(1) { 
        if (mybutton == 0){
            // Button Pressed
            wait(0.1);
            n_blinks(2, &green);
            if(WIFION){
                send_string("@42;T;Button is pressed#", 24, socket);
            }
        }
        else{
            //DEBUGP(("Switching to 'echo' mode\r\n"));
            //echo_mode(at, &pc);
            // DEBUGP(("Connecting to GPRS\r\n"));
            // GPRS_connect(at, &pc);
            if(is_connected == 0){
                DEBUGP(("Connecting to GPRS\r\n"));
                if(GPRS_connect(at, &pc) == 1){
                    DEBUGP(("[OK]\r\n"));
                    is_connected = 1;    

                    // Request initiation of the request
                    at -> send("AT+CIPSEND=%d", 13);
                    while(at -> getc() != '>'){
                        ;
                    }
                    at -> write("@42;I;Ver:1a#", 13);
                    at -> putc(0x1A);

                    wait(0.2);

                    DEBUGP(("Sending Data message"));

                    at -> send("AT+CIPSEND=%d", 23);
                    while(at -> getc() != '>'){
                        ;
                    }
                    at -> send("@42;T;12:00:30;MESSAGE#");
                    at -> putc(0x1A);

                    wait(0.2);

                } else{
                    DEBUGP(("[FAIL]\r\n"));
                }
            } else{ 
                int status = 0;

                status = at -> send("AT+CIPSEND=%d", 23);
                DEBUGP(("Status is: %d\r", status));
                while(at -> getc() != '>'){
                    ;
                }
                status = at -> write("@42;T;12:00:30;MESSAGE#", 23);
                status = at -> putc(0x1A);
                at -> recv("SEND OK");
                at -> flush();

                wait(0.2);
            }


            // Wait for data occure in the buffer and read them
            /*if(wait_for_sample(at, packet)){
                pc.printf("--> Packet: ");
                pc.printf(packet);
                pc.printf("\r\n");
            }*/

            /*send_string(msg, 67, &socket);*/
            wait(0.1);
        }
        /* Reset buffer to zeros */
        memset(packet, '\0', sizeof(char)*BUFFER_SIZE);
    }
}


int main()
{
    int err = 0;
    // Set up Serial link with PC
    pc.baud(115200);

    DEBUGP(("==========================================================\r\n"));
    DEBUGP(("Module start working.\r\n"));
    DEBUGP(("==========================================================\r\n"));

    // Parser for SIM808 messages
    ATParser at = ATParser(module, "\r\n");
    // at.setTimeout(1000);

    at.debugOn(1);

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

    if(WIFION){
        err = setup_connection(&socket, &spwf, ssid, seckey, host_ip, 
                host_port);
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
        } else {
            DEBUGP(("GPS module is initialized\r\n")); 
        }

        /* Run main loop with all the processing */
        mainloop(&socket, &at);
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

