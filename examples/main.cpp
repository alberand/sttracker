#include "mbed.h"
 
DigitalOut red(D5);
DigitalOut blue(D8);
DigitalOut green(D9);
 
int main(void) 
{
    while(1) 
    {
        red = 0; //red on
        wait(1.0); // 1 sec
        red = 1; //red off
        green = 0; //green on
        wait(1.0); // 1 sec
        green = 1; //green off
        blue = 0; //blue on
        wait(1.0); // 1 sec
        blue = 1; // blue off
        wait(1.0); // 1 sec
    }
}
