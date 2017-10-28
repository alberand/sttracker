#include "mbed.h"

#include "sim808.h"

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

/* @brief Prepare serial link and SIM808 V2 module for work (mainly for GPS
 * data).
 *
 * @return Boolean in case of succesful intialization.
 */
int sim808v2_GPS_setup(ATParser * at)
{
    int status = 1;

    // Power on GPS module
    status = at -> send("AT+CGNSPWR=1") && at -> recv("OK");

    // Set NMEA format
    // status = at -> send("AT+CGNSSEQ=RMC") && at -> recv("OK");

    // Turn on reporting
    if(GPSREPORTING == 1){
        status = at -> send("AT+CGNSTST=0") && at -> recv("OK");
    }

    return status;
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

int GPRS_connect(ATParser * at, Serial * pc_s)
{
    int status = 0;
    char ip[12] = { '\0' };
    char buf[255] = { '\0' };

    // Is PIN ok?
    status = at -> send("AT+CPIN?") && at -> recv("CPIN: READY");
    at -> flush();
    if(status != 0){
        DEBUGP(("PIN is not OK\r\n"));
        return status;
    }

    // Is SIM registreted in the network?
    status = at -> send("AT+CREG?") && at -> recv("CREG: 0,1");
    at -> flush();
    if(status != 0){
        DEBUGP(("SIM is not registreted\r\n"));
        return status;
    }

    // Is GPRS attached?
    status = at -> send("AT+CGATT?") && at -> recv("CGATT: 1");
    at -> flush();
    if(status != 0){
        DEBUGP(("GPRS is not attached\r\n"));
        return status;
    }

    // Reset the IP session if any
    status = at -> send("AT+CIPSHUT") && at -> recv("SHUT OK"); //"SHUT OK");
    at -> flush();
    if(status != 0){
        DEBUGP(("Failed to reset IP\r\n"));
        return status;
    }

    // Check if the IP stack is initialized
    status = at -> send("AT+CIPSTATUS") && at -> recv("OK") && at -> recv("STATE: IP INITIAL");
    at -> flush();
    if(status != 0){
        DEBUGP(("Failed to check if IP stack is initialized\r\n"));
        return status;
    }

    // Setting up a single connection mode
    status = at -> send("AT+CIPMUX=0") && at -> recv("OK");
    if(status != 0){
        DEBUGP(("Fail to set up a single connection mode\r\n"));
        return status;
    }

    // Set up DNS
    /*status = at -> send("AT+CDNSCFG=\"8.8.8.8\",\"8.8.4.4\"") && at -> recv("OK");
    if(status != 0){
        DEBUGP(("Fail to set up DNS\r\n"));
        return status;
    }*/

    status = at -> send("AT+CSTT=\"internet\",\"\",\"\"") && at -> recv("OK");
    if(status != 0){
        DEBUGP(("Failed to set up the APN, USER and PASSWORD for the PDP\r\n"));
        return status;
    }

    // Bring up the GPRS. The response might take some time
    status = at -> send("AT+CIICR") && at -> recv("OK");
    if(status != 0){
        DEBUGP(("Failed to bring up the GPRS\r\n"));
        return status;
    }

    // Get the local IP address
    status = at -> send("AT+CIFSR");
    int i = 0;
    char ch;
    while((ch = at -> getc()) != '\r'){
        ip[i++] = ch;
    }
    DEBUGP(("IP: %s\r\n", ip));

    // Connect
    status = at -> send("AT+CIPSTART=\"TCP\",\"147.32.196.177\",\"5000\"") && at -> recv("CONNECT OK");

    return status;
}
