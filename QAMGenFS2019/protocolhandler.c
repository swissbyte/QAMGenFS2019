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
#include "semphr.h"

/* Constants */
#define ANZSENDQUEUE					32							// according to document "ProtokollBeschreibung.pdf" from Claudio
#define	PROTOCOLBUFFERSIZE				32
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

#define GLOBAL_SLDP_PREAMBLE			0x55A1

EventGroupHandle_t xSettings;							// Settings from GUI
EventGroupHandle_t xStatus;								// something from Cedi


xQueueHandle xALDPQueue;								// Data to pack and send

SemaphoreHandle_t xGlobalProtocolBuffer_A_Key;			//A-Resource for ucGlobalProtocolBuffer_A
SemaphoreHandle_t xGlobalProtocolBuffer_B_Key;			//A-Resource for ucGlobalProtocolBuffer_B

/* global variables */
uint8_t ucglobalProtocolBuffer_A[ PROTOCOLBUFFERSIZE ] = {};
uint8_t ucglobalProtocolBuffer_B[ PROTOCOLBUFFERSIZE ] = {};
uint8_t ucActualBufferPos = 0;

uint8_t xFillOutputBuffer(struct ALDP_t_class *xALDP_Paket,struct SLDP_t_class *xSLDP_Paket, uint16_t Preamble);
void xWriteToOutputBuffer(uint8_t data);

volatile uint8_t ucActiveBuffer = ACTIVEBUFFER_A;

void vProtokollHandlerTask( void *pvParameters ) {
	(void) pvParameters;
	
	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	


	xALDPQueue = xQueueCreate( ANZSENDQUEUE, sizeof(uint8_t) );

	
	uint8_t	ucbuffercounter = 0;
	
	uint8_t ucProtocolBuffer_A_Counter = 1;
	uint8_t ucProtocolBuffer_B_Counter = 1;
	
	

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
			
/* we wait maximum 10ms for new pakets to become available */ 
	if(xQueueReceive(xALDPQueue,xALDP_Paket,10/portTICK_RATE_MS))
	{
		/* Store the sldp size as length of the aldp payload plus its two header bytes */
		xSLDP_Paket.sldp_size = xALDP_Paket->aldp_hdr_byte_2+2;
		
		/* First calculate the sizebyte of the SLDP Paket */
		xSLDP_Paket.sldp_crc8 = xCRC_calc(0x00,xSLDP_Paket.sldp_size);
		
		/* Now calculate all Bytes from the xALDP Paket, including its size byte and the first header. 
		   this leads us to +2 */
		for ( uint8_t i = 0; i <= xALDP_Paket->aldp_hdr_byte_2 + 2; i++ ) {
			xSLDP_Paket.sldp_crc8 = xCRC_calc(xSLDP_Paket.sldp_crc8, xALDP_Paket->aldp_payload[i] );
		} 

		/* now we have all neccessary informations to fill up our output buffer. 
		
		   - We have got a ALDP paket, filled up with sensor informations, directly from the queue
		   - We have created a SLDP paket, filled up with size and CRC informations. 
           - We know our preamble. */
		
		/* We dont have to check to which buffer we must send our data. Everyhthing gets handled in the xWriteToOutputBuffer */
		xFillOutputBuffer(xALDP_Paket,&xSLDP_Paket,GLOBAL_SLDP_PREAMBLE);
		 
	}
	else
	{
		/* We did not received any ALDP Pakets during our timeout time. 
		   Therefore we mark our Buffer to be ready to send if we got any data in them!*/
		if(ucActualBufferPos > 0)
		{
			if(ucActiveBuffer == ACTIVEBUFFER_A) xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
			else xSemaphoreGive(xGlobalProtocolBuffer_B_Key);
			ucActualBufferPos = 0;
		}
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
	
uint8_t xFillOutputBuffer(struct ALDP_t_class *xALDP_Paket,struct SLDP_t_class *xSLDP_Paket, uint16_t Preamble)
{
	uint8_t i = 0;
	
	/* First we always write our preamble */
	xWriteToOutputBuffer(Preamble & 0x00FF);
	xWriteToOutputBuffer(Preamble & 0xFF00 >> 8);
	
	/* After the Preamble, we must write the Header of the SLDP Paket. This is, the size-byte*/
	xWriteToOutputBuffer(xSLDP_Paket->sldp_size);
	
	/* Now from here, there is the ALDP Paket information.*/
	xWriteToOutputBuffer(xALDP_Paket->aldp_hdr_byte_1);
	xWriteToOutputBuffer(xALDP_Paket->aldp_hdr_byte_2);
	
	/* For the payload, we will use memcopy*/
	while(i!=xALDP_Paket->aldp_hdr_byte_2)
	{
		xWriteToOutputBuffer(xALDP_Paket->aldp_payload[i]);
		i++;
	}
	
	/* Now we have written all neccessary Data into our outputbuffer. Except for the crc byte. */
	xWriteToOutputBuffer(xSLDP_Paket->sldp_crc8);
	
	/*We have finished the writing into the buffer and return the ammount of written bytes.*/
	return 4 + xALDP_Paket->aldp_hdr_byte_2 + 2;	
}

void xWriteToOutputBuffer(uint8_t data)
{
	/* Check if we hit the Protocollbuffersize constraint. */
	if(ucActualBufferPos == PROTOCOLBUFFERSIZE)
	{
		/* If we hit the limit, then we mark the buffer to be ready to send
		   we also change to the next buffer and reset our counter to zero.*/
		if(ucActiveBuffer == ACTIVEBUFFER_A) 
		{
			ucActiveBuffer = ACTIVEBUFFER_B;
			xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
		}
		else
		{
			ucActiveBuffer = ACTIVEBUFFER_A;
			xSemaphoreGive(xGlobalProtocolBuffer_B_Key);			
		}
		ucActualBufferPos = 0;
	}
	
	/* Copy the incoming data to the appropriate buffer */
	if(ucActiveBuffer == ACTIVEBUFFER_A) ucglobalProtocolBuffer_A[ucActualBufferPos] = data;
	else ucglobalProtocolBuffer_B[ucActualBufferPos] = data;
	ucActualBufferPos++;
}