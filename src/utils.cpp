#include "utils.h"

void n_blinks(int num, DigitalOut * LED)
{
    for (int i = 0; i < num; i++) {
        LED ->operator=(1);
        wait(0.2);
        LED ->operator=(0);
        wait(0.2);
    }
}

void print_buffer(MyBuffer <char> * buffer, Serial * pc)
{
    int pos = 0;
    char buf[buffer -> getSize()];

    while(buffer -> available()){
        buf[pos++] = buffer -> get();
    }
    pc -> printf("Response: %s\r\n", buf);
}

void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}
