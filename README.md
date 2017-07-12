# STM GPS-tracker with WiFi module

This project is implementation of simple GPS tracker based on STM32 nucleo
L476RG board with external WiFi (X-NUCLEO-IDW01M1) and GPS+GPRS module (Adafruit
FONA 808 Shield).

STM board connects to the WiFi acsess point and send GPS coordinates of the
device. On a receiving machine is ran server with open TCP socket which reads
these data and save it to file. 

Used GPS module isn't fully supported by STM board. Serial hardware port used in
Adafruit board use pins 2 and 3 which don't correspond to any of hardware ports
on STM board. Therefore, you need to use so called software serial or physically
connect module's serial port to one of the Nucleo's serial ports. In this project I
used Serial3 which corresponds to pins A0 and A1 on the STM board. So, to make 
it work you need to connect Adafruit module pins in a following way 2 -> A0, 
3 -> A1.

Also, you need to use addition battery to power up Adafruit module. It's not
enough to power it from the USB cable throught the STM board. 
[Why?](https://learn.adafruit.com/adafruit-fona-808-cellular-plus-gps-shield-for-arduino/faqs#faq-1).

# Requirements

Software:
* platformio

Used hardware:
* [STM32 Nucleo L476RG](http://www.st.com/en/evaluation-tools/nucleo-l476rg.html)
* [STM WiFi module](https://developer.mbed.org/components/X-NUCLEO-IDW01M1/)
* [Adafruit FONA 808 Shield](https://www.adafruit.com/product/2636)
* Li-Po battery 3.7V 1600mAh

# Run

1. Run TCP server
2. Configure program with correct parameters for WiFi AP and TCP socket
3. Compile and upload project `pio run -t upload -e nucleo`
