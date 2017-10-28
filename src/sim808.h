#ifndef __SIM808_H__
#define __SIM808_H__

#include "config.h"

#include "TCPSocket.h"
#include "SpwfInterface.h"
#include "MyBuffer.h"
#include "BufferedSerial.h"

/* @brief Write command `cmd` to the serial port `sr` and add newline '\n'.
 *
 * @param cmd Command to send.
 * @param sr mbed Serial object connected to SIM808_V2 module.
 */
void sim808v2_send_cmd(const char * cmd, BufferedSerial * sr);

/* @brief Prepare serial link and SIM808 V2 module for work (mainly for GPS
 * data).
 *
 * @return Boolean in case of succesful intialization.
 */
int sim808v2_GPS_setup(ATParser * at);

/* @brief Send SIM808 module AT command and wait for OK. If OK received returns
 * 1, 0 otherwise.
 *
 * @param ATParser attached to the SIM808 module
 * @return True if module is answering
 */
bool sim808_is_active(ATParser * at);

/* @brief Testing mode which setup flawless communication between pc and sim808
 *
 * @param ATParser connected to the SIM808 module
 * @param Serial port
 */
void echo_mode(ATParser * at, Serial * pc);

/*
 *
 */
int GPRS_connect(ATParser * at, Serial * pc_s);

#endif // __SIM808_H__
