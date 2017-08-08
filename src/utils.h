#ifndef __UTILS_H__
#define __UTILS_H__

#include "mbed.h"

#include "MyBuffer.h"

/* @brief Make `num` of blinks by LED `green`.
 *
 * @param num Number of blinks
 * @param LED DegitalOut
 */
void n_blinks(int num, DigitalOut * LED);

/* @brief Prints buffer content to the Serial link. Used for printing
 * SIM808_V2's response to commands.
 *
 * @param buffer Buffer with content to print
 * @param pc Serial link
 */
void print_buffer(MyBuffer <char> * buffer, Serial * pc);

/* @brief Appends character to the string
 *
 * @param s string
 * @param c character
 */
void append(char* s, char c);

#endif // __UTILS_H__
