   /**
  ******************************************************************************
  * @file    MQTT_SPWF_interface.c
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

#include "MQTT_SPWF_interface.h"
#include "MQTTClient.h"
#include "TLocalBuffer.h"

/** @addtogroup MIDDLEWARES
* @{
*/ 


/** @defgroup  MQTT_MODULE
  * @brief MQTT Interface
  * @{
  */


/** @defgroup MQTT_MODULE_Private_Defines
  * @{
  */
#define APPLICATION_BUFFER_SIZE         512    // Need to be the same as TLOCALBUFFER_SIZE
/**
  * @}
  */


/** @addtogroup MQTT_MODULE_Private_Variables
  * @{
  */
uint8_t receive_data[APPLICATION_BUFFER_SIZE];
uint32_t application_idx = 0;
uint8_t  sock_id;

static TLocalBuffer localBufferReading;

unsigned long MilliTimer;
uint32_t intcounter=0;
WiFi_Status_t status = WiFi_MODULE_SUCCESS;  
// all failure return codes must be negative, as per MQTT Paho
typedef enum
{ 
  MQTT_SUCCESS           = 1,
  MQTT_ERROR           = -1 
} Mqtt_Status_t;

Mqtt_Status_t mqtt_intf_status;

/**
  * @}
  */



/**
  * @brief  Interface Nucleo Time Handler 
  * @param  None
  * @retval None
  */
void SysTickIntHandlerMQTT(void) {
	MilliTimer++;
}

/**
  * @brief  Check timeout 
  * @param  timer : Time structure
  * @retval 1 for time expired, 0 otherwise
  */
char expired(Timer* timer) {
	long left = timer->end_time - MilliTimer;
       // printf("left %ld \r\n", left);  

	return (left < 0);
}

/**
  * @brief  Countdown in ms 
  * @param  timer : Time structure, timout
  * @retval None
  */
void countdown_ms(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + timeout;
}

/**
  * @brief  Countdown in us 
  * @param  timer : Time structure, timout
  * @retval None
  */
void countdown(Timer* timer, unsigned int timeout) {
	timer->end_time = MilliTimer + (timeout * 1000);
}

/**
  * @brief  Milliseconds left before timeout
  * @param  timer : Time structure
  * @retval Left milliseconds
  */
int left_ms(Timer* timer) {
	long left = timer->end_time - MilliTimer;
	return (left < 0) ? 0 : left;
}

/**
  * @brief  Init timer structure
  * @param  timer : Time structure
  * @retval None
  */
void InitTimer(Timer* timer) {
	timer->end_time = 0;
        MilliTimer = 0;
}

/**
  * @brief  Connect with MQTT Broker via TCP or TLS via anonymous authentication
  * @param  net : Network structure
  *         hostname : server name 
  *         port_number : server port number 
  *         protocol : 't' for TCP, 's' for TLS
  * @retval Status : OK/ERR
  */
int spwf_socket_create (Network* net, uint8_t * hostname, uint32_t port_number,uint8_t * protocol){

  status = wifi_socket_client_open(hostname, port_number,  protocol, &sock_id);
  net->my_socket = sock_id;
  mqtt_intf_status = MQTT_ERROR;
  if(status==WiFi_MODULE_SUCCESS)
  {  
      /* Initialize buffer to manage data read via callbacks from WiFi module */
      LocalBufferInit(&localBufferReading);  
      mqtt_intf_status = MQTT_SUCCESS;
     return mqtt_intf_status;
  }
  else
     return mqtt_intf_status;
  
 }
 
/**
  * @brief  Close socket 
  * @param  n : Network structure
  * @retval None
  */
void spwf_socket_close (Network* n)
{
    wifi_socket_client_close (n->my_socket);
}

/**
  * @brief  Socket write 
  * @param  net : Network structure
  *         buffer : buffer to write
  *         lenBuf : lenght data in buffer 
  *         timeout : timeout for writing to socket  
  * @retval lenBuff : number of bytes written to socket 
  */
int spwf_socket_write (Network* net, unsigned char* buffer, int lenBuf, int timeout){
   mqtt_intf_status = MQTT_ERROR;

   if(wifi_socket_client_write(net->my_socket, lenBuf, (char *)buffer) !=  WiFi_MODULE_SUCCESS){
           return mqtt_intf_status;
   }    
   return lenBuf;
 }

/**
  * @brief  Wrapping function to read data from an buffer queue, which is filled by WiFi callbacks 
  * @param  net : Network structure 
  *         i : buffer to fill with data read
  *         ch : number of bytes to read
  *         timeout : timeout for writing to socket 
  * @retval sizeReceived : number of bytes read
  */
int spwf_socket_read (Network* net, unsigned char* i, int ch, int timeout){
    
  int sizeReceived;
 
  sizeReceived = LocalBufferGetSizeBuffer(&localBufferReading);
  
  if(sizeReceived > 0) 
  {    
    if(sizeReceived >= ch)
    {
      LocalBufferPopBuffer(&localBufferReading, i, ch);
      return ch;
    }
    else
    {  
      LocalBufferPopBuffer(&localBufferReading, i, sizeReceived);
      return sizeReceived;
    }     
  }
  return 0;
 }

/**
  * @brief  Map MQTT network interface with SPWF wrapper functions 
  * @param  n : Network structure 
  * @retval None
  */
void NewNetwork(Network* n) {
	n->my_socket =0;
	n->mqttread = spwf_socket_read;
	n->mqttwrite = spwf_socket_write;
        n->disconnect = spwf_socket_close;
}


/**
  * @brief  Wifi callback for data received  
  * @param  data_ptr : pointer to buffer to be filled with data
  *         message_size : message size (equal to chunk_size if less than APPLICATION_BUFFER_SIZE bytes) 
  *         chunk_size : chunk size (used in case message size is larger than APPLICATION_BUFFER_SIZE)
  * @retval None
  */
void ind_wifi_socket_data_received(uint8_t socket_id,uint8_t * data_ptr, uint32_t message_size, uint32_t chunck_size)
//void ind_wifi_socket_data_received(uint8_t * data_ptr, uint32_t message_size, uint32_t chunck_size)
{

  if ( message_size > APPLICATION_BUFFER_SIZE )
 {
     printf("\r\n[E] Error ind_wifi_socket_data_received. Exceeded APPLICATION_BUFFER_SIZE \r\n");
     return;
 }

 if ( message_size == chunck_size){
    memcpy(receive_data, data_ptr, chunck_size);       
 }
 else
 {     
     memcpy(&receive_data[application_idx], data_ptr, chunck_size);
     application_idx += chunck_size;
    
     if (application_idx == message_size){              
       application_idx = 0;      
     }
     else
       return;
  }

  LocalBufferPushBuffer(&localBufferReading,(char *)&receive_data,message_size);
}


/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2013 STMicroelectronics *****END OF FILE****/

