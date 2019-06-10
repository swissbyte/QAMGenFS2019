/*
 * protocolhandler.c
 *
 * Created: 18.05.2019 17:19:16
 *  Author: philippeppler
 */ 

#include "avr_compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "protocolhandler.h"
#include "string.h"

/* Constants */
#define ANZSENDQUEUE					32							// according to document "ProtokollBeschreibung.pdf" from Claudio

/* xQuelle */
#define PAKET_TYPE_ALDP					0x01
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02
#define ALDP_SRC_ERROR					0xFF

/* xSettings */
#define Settings_QAM_Order				1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequency				1<<3
/* xStatus */
#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3
/* Buffer */
#define ACTIVEBUFFER_A					0
#define ACTIVEBUFFER_B					1

EventGroupHandle_t xSettings;							// Settings from GUI
EventGroupHandle_t xStatus;								// something from Cedi
uint8_t ucglobalProtocolBuffer_A[ PROTOCOLBUFFERSIZE ] = {};
uint8_t ucglobalProtocolBuffer_B[ PROTOCOLBUFFERSIZE ] = {};

xQueueHandle xALDPQueue;								// Data to pack and send



void vProtokollHandlerTask( void *pvParameters ) {
	(void) pvParameters;
	
	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
/* Debugging
	PORTF.DIRSET = PIN0_bm;		
	PORTF.OUT = 0x01;
*/	

	xALDPQueue = xQueueCreate( ANZSENDQUEUE, sizeof(uint8_t) );

	
	uint8_t	ucbuffercounter = 0;
	
	uint8_t ucProtocolBuffer_A_Counter = 1;
	uint8_t ucProtocolBuffer_B_Counter = 1;
	
	uint8_t ucActiveBuffer = ACTIVEBUFFER_A;

	xGlobalProtocolBuffer_A_Key = xSemaphoreCreateMutex();
	xGlobalProtocolBuffer_B_Key = xSemaphoreCreateMutex();
	
	xSemaphoreTake( xGlobalProtocolBuffer_A_Key, portMAX_DELAY );


	for(;;) {
		
		
/*	Debbuging
			PORTF.OUTTGL = 0x01;
			uint8_t a = 10;
			uint8_t b = 20;
			uint8_t c = 30;
			uint8_t d = 40;
			uint8_t e = 50;
			
			xQueueSendToBack(xALDPQueue, &a, portMAX_DELAY);
			xQueueSendToBack(xALDPQueue, &b, portMAX_DELAY);
			xQueueSendToBack(xALDPQueue, &c, portMAX_DELAY);
			xQueueSendToBack(xALDPQueue, &d, portMAX_DELAY);
			xQueueSendToBack(xALDPQueue, &e, portMAX_DELAY);
*/
			
		if (uxQueueMessagesWaiting( xALDPQueue ) > 0) {
			
/* save data from Queue into Buffer */
			ucbuffercounter = 2;
			uint8_t xSendQueueBuffer[ uxQueueMessagesWaiting( xALDPQueue ) + 2 ];
						
			while( ( uxQueueMessagesWaiting( xALDPQueue ) > 0 ) && (ucbuffercounter < ANZSENDQUEUE ) ) {
				uint8_t xoutBufferPointer;
				xQueueReceive( xALDPQueue, &xoutBufferPointer , portMAX_DELAY );					 
				xSendQueueBuffer[ ucbuffercounter ] = xoutBufferPointer;
				ucbuffercounter++;
			}
/* ALDP Source in byte 1 */
			if( ( xEventGroupGetBits( xSettings ) & Settings_Source_Bit1 ) ) {
				if( ( xEventGroupGetBits( xSettings ) & Settings_Source_Bit1 ) ) {
					// UART
					xSendQueueBuffer [0 ] = ALDP_SRC_UART;
				}
				else {
					// Testpattern
					xSendQueueBuffer[ 0 ] = ALDP_SRC_TEST;
				}
			}
			else {
				if( ( xEventGroupGetBits( xSettings ) & Settings_Source_Bit1 ) ) {
					// I2C
					xSendQueueBuffer[ 0 ] = ALDP_SRC_I2C;
				}
				else {
					// n.a. (Error)
					xSendQueueBuffer[ 0 ] = ALDP_SRC_ERROR;
				}
			}

/* ALDP Size in byte 2 */
			xSendQueueBuffer[ 1 ] = ucbuffercounter-2;		
				

/* ALDP and SLDP */			
			xSLDP_Paket.sldp_size = sizeof( xSendQueueBuffer );
			xSLDP_Paket.sldp_payload = &xSendQueueBuffer[ 0 ];
			xALDP_Paket = ( struct ALDP_t_class * ) xSLDP_Paket.sldp_payload;		
			
			uint8_t ucOutBuffer[ xSLDP_Paket.sldp_size + 2 ];
			ucOutBuffer[ 0 ] = xSLDP_Paket.sldp_size;

			for(uint8_t i = 0; i != xSLDP_Paket.sldp_size; i++ )	{
				ucOutBuffer[ i + 1 ] = xSLDP_Paket.sldp_payload[ i ];
			}

/* calculating CRC8 */			
			xSLDP_Paket.sldp_crc8 = 0;
			for ( uint8_t i = 0; i <= xSLDP_Paket.sldp_size; i++ ) {
				xSLDP_Paket.sldp_crc8 = xCRC_calc(xSLDP_Paket.sldp_crc8, ucOutBuffer[ i ] );
			}

			ucOutBuffer[ xSLDP_Paket.sldp_size + 1 ] = xSLDP_Paket.sldp_crc8;			
			
			
/* Sendbuffer-handler */
			/* Buffer A overflow? => Buffer switch from A to B */
			if( ( ucActiveBuffer == ACTIVEBUFFER_A ) && ( ( ucProtocolBuffer_A_Counter + xSLDP_Paket.sldp_size + 2 ) > PROTOCOLBUFFERSIZE ) ) {	
					ucActiveBuffer = ACTIVEBUFFER_B;
					ucProtocolBuffer_B_Counter = 1;	
					xSemaphoreGive(xGlobalProtocolBuffer_A_Key);	
					xSemaphoreTake( xGlobalProtocolBuffer_B_Key, portMAX_DELAY );
			}
			/* Buffer B overflow? => Buffer switch from B to A */
			if( ( ucActiveBuffer == ACTIVEBUFFER_B ) && ( ( ucProtocolBuffer_B_Counter + xSLDP_Paket.sldp_size + 2 ) > PROTOCOLBUFFERSIZE ) ) {
					ucActiveBuffer = ACTIVEBUFFER_A;	
					ucProtocolBuffer_A_Counter = 1;
					xSemaphoreGive(xGlobalProtocolBuffer_B_Key);
					xSemaphoreTake( xGlobalProtocolBuffer_A_Key, portMAX_DELAY );
			}

	
	
/* Copy Data into global Sendbuffer */
/* Todo: harmonize these by outsourcing in a seperate function*/	
			
			if( ucActiveBuffer == ACTIVEBUFFER_A ) {
				memcpy( ucglobalProtocolBuffer_A + ucProtocolBuffer_A_Counter, ucOutBuffer, sizeof( ucOutBuffer ) );				// copy the Data into Buffer A
				ucglobalProtocolBuffer_A[0] = ucProtocolBuffer_A_Counter+xSLDP_Paket.sldp_size + 2;
				ucProtocolBuffer_A_Counter += xSLDP_Paket.sldp_size + 2;
				
			}
			
			else if( ucActiveBuffer == ACTIVEBUFFER_B ) {
				memcpy( ucglobalProtocolBuffer_B + ucProtocolBuffer_B_Counter, ucOutBuffer, sizeof(ucOutBuffer ) );			// copy the Data into Buffer B
				ucglobalProtocolBuffer_B[ 0 ] = ucProtocolBuffer_B_Counter+xSLDP_Paket.sldp_size + 2;
				ucProtocolBuffer_B_Counter += xSLDP_Paket.sldp_size + 2;
			}
			
		}
		else {
			vTaskDelay( 100 / portTICK_RATE_MS );				// Delay 100ms (collecting data to send)
		}

	}
}


// CRC8 Function (ROM=39 / RAM=4 / Average => 196_Tcy / 24.5_us for 8MHz clock)    https://www.ccsinfo.com/forum/viewtopic.php?t=37015 (Original code by T. Scott Dattalo)
uint8_t xCRC_calc( uint8_t uiCRC, uint8_t uiCRC_data ) 
{ 
	uint8_t i = (uiCRC_data ^ uiCRC) & 0xff;
	uiCRC = 0;
	if(i & 1)
		uiCRC ^= 0x5e; 
	if(i & 2)
		uiCRC ^= 0xbc;
	if(i & 4)
		uiCRC ^= 0x61;
	if(i & 8)
		uiCRC ^= 0xc2;
	if(i & 0x10)
		uiCRC ^= 0x9d;
	if(i & 0x20)
		uiCRC ^= 0x23;
	if(i & 0x40)
		uiCRC ^= 0x46;
	if(i & 0x80)
		uiCRC ^= 0x8c;
	return(uiCRC);
}
	
