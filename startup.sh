#!/bin/bash

# Script used for check basic functionallity and availability of the STM32
# board.

#
# Read list of usb-devices and look for STMicroelectronics
#
isconnected()
{
    lsusb | grep STMicroelectronics > /dev/null 2>&1
    if [ $? -ne 0 ]; then
	    echo "Board not found. Is it plugged in?"
	    exit
    fi
}

#
# Upload precompiled application which checks requred functions. Upload is
# operated via STLink.
#
boothwtest(){

}
