/*
 * protocolhandler.c
 *
 *  Created:	14.06.2019
 *  Author:		C. Hediger
 */ 

#include "avr_compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "protocolhandler.h"
#include "string.h"
#include "Menu_IMU.h"
#include "semphr.h"

#define BUFFER_A					0
#define BUFFER_B					1
#define NO_BUFFER					2

volatile uint8_t ucGlobalFrameBuffer_A[MAX_FRAME_SIZE];
volatile uint8_t ucGlobalFrameBuffer_B[MAX_FRAME_SIZE];

xQueueHandle xALDPQueue;								

SemaphoreHandle_t xGlobalFrameBuffer_A_Key;			
SemaphoreHandle_t xGlobalFrameBuffer_B_Key;			

uint8_t xFillOutputBuffer(struct ALDP_t_class *xALDP_Paket,struct SLDP_t_class *xSLDP_Paket, uint16_t Preamble);
void vWriteToOutputBuffer(uint8_t data);
void vReleaseBufferAndSwitch();
uint8_t xCRC8_calc( uint8_t ucLastCRCValue, uint8_t ucNewCRC_data );

volatile uint8_t ucActiveBuffer = NO_BUFFER;
volatile uint8_t ucActualBufferPos = 0;

void vProtokollHandlerTask( void *pvParameters ) {

	/* Local Paket structures */	
	struct ALDP_t_class xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
	/* Test pattern
	struct ALDP_t_class testPaket;
	testPaket.aldp_hdr_byte_1 = PAKET_TYPE_ALDP|ALDP_SRC_TEST;
	testPaket.aldp_size = 2;
	testPaket.aldp_payload[0] = 0xAA;
	testPaket.aldp_payload[1] = 0x55;
	uint8_t res = xQueueSendToBack(xALDPQueue,(void *)&testPaket,portMAX_DELAY);
	*/

	/* Try to get Buffer A */
	if(ucActiveBuffer == NO_BUFFER)
	{
		xSemaphoreTake(xGlobalFrameBuffer_A_Key,portMAX_DELAY);
		ucActiveBuffer = BUFFER_A;
	}

	while(1)
	{			
		/* we wait maximum 50ms for new pakets to become available 
			after that periode, we release the buffer and send the Bytes*/ 
		if(xQueueReceive(xALDPQueue,&(xALDP_Paket),50/portTICK_PERIOD_MS))
		{
			/* Store the sldp size as length of the aldp payload plus its two header bytes */
			xSLDP_Paket.sldp_size = xALDP_Paket.aldp_size+2;
		
			/* First calculate the sizebyte of the SLDP Paket */
			xSLDP_Paket.sldp_crc8 = xCRC8_calc(0x00,xSLDP_Paket.sldp_size);
		
			/* Now calculate all Bytes from the xALDP Paket, including its size byte and the first header. 
			   this leads us to +2 */
			for ( uint8_t i = 0; i <= xALDP_Paket.aldp_size + 2; i++ ) {
				xSLDP_Paket.sldp_crc8 = xCRC8_calc(xSLDP_Paket.sldp_crc8, xALDP_Paket.aldp_payload[i] );
			} 

			/* now we have all neccessary informations to fill up our output buffer. 
		
			   - We have got a ALDP paket, filled up with sensor informations, directly from the queue
			   - We have created a SLDP paket, filled up with size and CRC informations. 
			   - We know our preamble. */
		
			/* We dont have to check to which buffer we must send our data. Everyhthing gets handled in the vWriteToOutputBuffer */
			xFillOutputBuffer(&xALDP_Paket,&xSLDP_Paket,GLOBAL_FRAME_PREAMBLE);
		}
		else
		{
			/* We did not received any ALDP Pakets during our timeout time. 
			   Therefore we mark our Buffer to be ready to send if we got any data in them!*/
			if(ucActualBufferPos > 0)
			{
				vReleaseBufferAndSwitch();
			}
		}
	}
}

/**
* xCRC8_calc calculates the CRC8 checksum for a given byte.
* source: https://www.ccsinfo.com/forum/viewtopic.php?t=37015
* @param ucLastCRCValue, ucNewCRC_data
* @return calculated CRC8 value
* @author T. Scott Dattalo, modified by C. Hediger
*/
uint8_t xCRC8_calc( uint8_t ucLastCRCValue, uint8_t ucNewCRC_data )
{ 
	uint8_t i = (ucNewCRC_data ^ ucLastCRCValue) & 0xff;
	ucLastCRCValue = 0;
	if(i & 1)
		ucLastCRCValue ^= 0x5e; 
	if(i & 2)
		ucLastCRCValue ^= 0xbc;
	if(i & 4)
		ucLastCRCValue ^= 0x61;
	if(i & 8)
		ucLastCRCValue ^= 0xc2;
	if(i & 0x10)
		ucLastCRCValue ^= 0x9d;
	if(i & 0x20)
		ucLastCRCValue ^= 0x23;
	if(i & 0x40)
		ucLastCRCValue ^= 0x46;
	if(i & 0x80)
		ucLastCRCValue ^= 0x8c;
	return(ucLastCRCValue);
}
	
/**
* xFillOutputBuffer fills the outputbuffer with a SLDP Paket and a ALDP Paket.
* This function also handles the insertion of the Preamble into the frame
* @param ALDP_Paket, SLDP_Paket, Preamble
* @return amount of sent bytes
* @author C. Hediger
*/
uint8_t xFillOutputBuffer(struct ALDP_t_class *xALDP_Paket,struct SLDP_t_class *xSLDP_Paket, uint16_t Preamble)
{
	uint8_t i = 0;
	
	/* First we always write our preamble */
	vWriteToOutputBuffer((Preamble >> 8) & 0x00FF);
	vWriteToOutputBuffer(Preamble & 0x00FF);
	
	/* After the Preamble, we must write the Header of the SLDP Paket. This is, the size-byte*/
	vWriteToOutputBuffer(xSLDP_Paket->sldp_size);
	
	/* Now from here, there is the ALDP Paket information.*/
	vWriteToOutputBuffer(xALDP_Paket->aldp_hdr_byte_1);
	vWriteToOutputBuffer(xALDP_Paket->aldp_size);
	
	/* For the payload, we will use memcopy*/
	while(i!=xALDP_Paket->aldp_size)
	{
		vWriteToOutputBuffer(xALDP_Paket->aldp_payload[i]);
		i++;
	}
	
	/* Now we have written all neccessary Data into our outputbuffer. Except for the crc byte. */
	vWriteToOutputBuffer(xSLDP_Paket->sldp_crc8);
	
	/*We have finished the writing into the buffer and return the ammount of written bytes.*/
	return 4 + xALDP_Paket->aldp_size+ 2;	
}

/**
* vWriteToOutputBuffer is responsible for writing into the active output buffer. 
* This function also handles buffer overflow conditions and switches the buffer in such a case.
* @param uint8_t data input data to write
* @return Nothing
* @author C. Hediger
*/
void vWriteToOutputBuffer(uint8_t data)
{
	/* Check if we hit the Protocollbuffersize constraint. 
	   we subtract one because the first byte is the size byte.*/
	if(ucActualBufferPos == MAX_FRAME_SIZE-1)
	{
		/* If we hit the limit, then we mark the buffer to be ready to send
		   we also change to the next buffer and reset our counter to zero.*/
		vReleaseBufferAndSwitch();
	}
	
	/* Copy the incoming data to the appropriate buffer 
	   we always write to an offset of one, because position 0 is reserved for the size byte*/
	if(ucActiveBuffer == BUFFER_A) ucGlobalFrameBuffer_A[ucActualBufferPos+1] = data;
	else ucGlobalFrameBuffer_B[ucActualBufferPos+1] = data;
	ucActualBufferPos++;
}
	
/**
* vReleaseBufferAndSwitch is responsible for writing the size information into the first byte
* this function also handles the switch from Buffer A to B including waiting for the appropriate Semaphore.
* @param args Unused
* @return Nothing
* @author C. Hediger
*/
void vReleaseBufferAndSwitch()
{
	if(ucActiveBuffer == BUFFER_A)
	{
		/* Write FrameSize into Frame */
		ucGlobalFrameBuffer_A[0] = ucActualBufferPos;
		xSemaphoreGive(xGlobalFrameBuffer_A_Key);
		xSemaphoreTake(xGlobalFrameBuffer_B_Key, portMAX_DELAY);
		ucActiveBuffer = BUFFER_B;
	}
	else
	{
		/* Write FrameSize into Frame */
		ucGlobalFrameBuffer_B[0] = ucActualBufferPos;
		xSemaphoreGive(xGlobalFrameBuffer_B_Key);
		xSemaphoreTake(xGlobalFrameBuffer_A_Key, portMAX_DELAY);
		ucActiveBuffer = BUFFER_A;
	}
	ucActualBufferPos = 0;
}