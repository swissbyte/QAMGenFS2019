/*
 * protokollhandler.c
 *
 * Created: 18.05.2019 17:19:16
 *  Author: philippeppler
 */ 

#include "avr_compiler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "protokollhandler.h"


#define  ANZSENDQUEUE					253							// gemäss Definition im Dokument "ProtokollBeschreibung.pdf" von Claudio

#define Settings_QAM_Ordnung			1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequenz				1<<3

#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3

#define PAKET_TYPE_ALDP					0x01
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02

EventGroupHandle_t xSettings;
EventGroupHandle_t xStatus;

TaskHandle_t SendTask;
xQueueHandle xDataSendQueue;							// Daten zum Verpacken und Senden  (Daten von Cedi)


void vSendTask(void *pvParameters) {
	(void) pvParameters;
	
	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	
	xDataSendQueue = xQueueCreate(ANZSENDQUEUE, sizeof(uint8_t));

	
	uint8_t	uibuffercounter = 0;

	
	uint8_t a = 10;
	uint8_t b = 20;
	uint8_t c = 30;
	uint8_t d = 40;
	uint8_t e = 50;
	
	
	xQueueSendToBack(xDataSendQueue, &a, portMAX_DELAY);
	xQueueSendToBack(xDataSendQueue, &b, portMAX_DELAY);
	xQueueSendToBack(xDataSendQueue, &c, portMAX_DELAY);
	xQueueSendToBack(xDataSendQueue, &d, portMAX_DELAY);
	xQueueSendToBack(xDataSendQueue, &e, portMAX_DELAY);
	
	
	for(;;) {
		PORTF.OUTTGL = 0x01;	
		if (uxQueueMessagesWaiting(xDataSendQueue) > 0) {
			uibuffercounter = 0;

//********** ALDP **********
			
				if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
					if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
						// UART
						xALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_UART;
					}	
					else {
						// Testpattern
						xALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_TEST;
					}
				} 
				else {
					if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
						// I2C
						xALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_I2C;
					}	
					else {
						// n.a. (Error)
					}
				}
			uint8_t xoutBuffer[uxQueueMessagesWaiting(xDataSendQueue)+2];
			xALDP_Paket = (struct ALDP_t_class *) &xoutBuffer[0];
			
			while ((uxQueueMessagesWaiting(xDataSendQueue) > 0) && uibuffercounter < ANZSENDQUEUE ) {
				uint8_t xoutBufferPointer;
				
				xQueueReceive(xDataSendQueue, &xoutBufferPointer , portMAX_DELAY);					// Umsetzung?? Pointer und struct 
				
				xoutBuffer[uibuffercounter+2] = xoutBufferPointer;
				
				
				
				xALDP_Paket->aldp_payload[uibuffercounter] = xoutBuffer[uibuffercounter+2];					// ausgelesener wert aus queue 
				uibuffercounter++;
			}
			xALDP_Paket->aldp_hdr_byte_1 = uibuffercounter;						// ALDP size
		
//******* SLDP *************
			
		//	xSLDP_Paket.sldp_payload = xALDP_Paket;			// SLDP Payload
			xSLDP_Paket.sldp_crc8 = 0x55;					// SLDP CRC8 als Trailer			TBD
			xSLDP_Paket.sldp_size = uibuffercounter + 2;	// SLDP Size als Header
			
			vTaskDelay(50 / portTICK_RATE_MS);				// Delay 50ms
			
//******* SEND *************
			
			//SLDP in buffer schreiben

		}
	}
}
