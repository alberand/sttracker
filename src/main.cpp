#include "mbed.h"

Serial pc(USBTX, USBRX);
DigitalOut myled(LED1);

int main() {
    int i = 1;
    pc.printf("Echoes back to the screen anything you type\n");
    while(1) {
        wait_ms(500);
        pc.printf("This program runs since %d seconds.\n", i++);
        myled = !myled;
    }
}
