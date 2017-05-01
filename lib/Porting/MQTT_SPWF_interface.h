   /**
  ******************************************************************************
  * @file    MQTT_SPWF_interface.h
  * @author  Central LAB
  * @version V1.0.0
  * @date    17-Oct-2015
  * @brief   MQTT Paho wrapper functions for STM32Nucleo and SPWF WiFi module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  * 
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


#ifndef __TARGET_SOCKET_H__
#define __TARGET_SOCKET_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "wifi_interface.h"
#include "stm32_spwf_wifi.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 


/** @defgroup  MQTT_MODULE
  * @brief MQTT Interface
  * @{
  */


/**
  * @brief  timer structure
  */	
typedef struct timer {
	unsigned long systick_period;
	unsigned long end_time;
}Timer;

/**
  * @brief network structure, contain socket id, and pointer to MQTT read,write and disconnect functions 
  */	
typedef struct network
{
	int my_socket;
	int (*mqttread) (struct network*, unsigned char*, int, int);
	int (*mqttwrite) (struct network*, unsigned char*, int, int);
        void (*disconnect) (struct network*);
}Network;

void SysTickIntHandlerMQTT(void);
char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);
void InitTimer(Timer*);

void NewNetwork(Network*);   
int spwf_socket_create (Network*, uint8_t*, uint32_t, uint8_t*);
int spwf_socket_write (Network*, unsigned char*, int, int);
int spwf_socket_read (Network*, unsigned char*, int, int);
void spwf_socket_close (Network* n);
void ind_wifi_socket_data_received(uint8_t socket_id,uint8_t * data_ptr, uint32_t message_size, uint32_t chunck_size);
//void ind_wifi_socket_data_received(uint8_t * data_ptr, uint32_t message_size, uint32_t chunck_size);
void ind_wifi_socket_client_remote_server_closed(uint8_t * socket_closed_id);

#endif /* __TARGET_SOCKET_H__ */

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2013 STMicroelectronics *****END OF FILE****/

