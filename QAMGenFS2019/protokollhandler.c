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


#define  AnzSendQueue					253							// gemäss Definition im Dokument "ProtokollBeschreibung.pdf" von Claudio

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
xQueueHandle DataSendQueue;							// Daten zum Verpacken und Senden


void vSendTask(void *pvParameters) {
	(void) pvParameters;
	
	struct SLDP_t_class *SLDP_Paket;
	struct ALDP_t_class *ALDP_Paket;
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	
	DataSendQueue = xQueueCreate(AnzSendQueue, sizeof(uint8_t));

	
	uint8_t	buffercounter = 0;

	
	uint8_t a = 0x10;
	uint8_t b = 0x20;
	uint8_t c = 0x30;
	uint8_t d = 0x40;
	uint8_t e = 0x50;
	
	
	xQueueSendToBack(DataSendQueue, &a, portMAX_DELAY);
	xQueueSendToBack(DataSendQueue, &b, portMAX_DELAY);
	xQueueSendToBack(DataSendQueue, &c, portMAX_DELAY);
	xQueueSendToBack(DataSendQueue, &d, portMAX_DELAY);
	xQueueSendToBack(DataSendQueue, &e, portMAX_DELAY);
	
	
	for(;;) {
		PORTF.OUTTGL = 0x01;	
		if (uxQueueMessagesWaiting(DataSendQueue) > 0) {
			buffercounter = 0;

//********** ALDP **********
			
				if (xEventGroupGetBits(xSettings) & Settings_Source_Bit1 == 1) {
					if (xEventGroupGetBits(xSettings) & Settings_Source_Bit1 == 1) {
						// UART
						ALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_UART;
					}	
					else {
						// Testpattern
						ALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_TEST;
					}
				} 
				else {
					if (xEventGroupGetBits(xSettings) & Settings_Source_Bit1 == 1) {
						// I2C
						ALDP_Paket->aldp_hdr_byte_1 = ALDP_SRC_I2C;
					}	
					else {
						// n.a. (Error)
					}
				}
			

			while (uxQueueMessagesWaiting(DataSendQueue) > 0) {
				//xQueueReceive(DataSendQueue, ALDP_Paket->aldp_payload*[buffercounter], portMAX_DELAY);		// Umsetzung?? Pointer und struct 
				xQueueReceive(DataSendQueue, &ALDP_Paket[buffercounter+2], portMAX_DELAY);		// Umsetzung?? Pointer und struct 
				buffercounter++;
			}
			ALDP_Paket->aldp_hdr_byte_1 = buffercounter;						// ALDP size
		
//******* SLDP *************
			
			SLDP_Paket->sldp_payload = &ALDP_Paket;		// SLDP Payload
			SLDP_Paket->sldp_crc8 = 0x00;				// SLDP CRC8			TBD
			SLDP_Paket->sldp_size = buffercounter + 2;	// SLDP Size
			
			vTaskDelay(50 / portTICK_RATE_MS);				// Delay 50ms
			
//******* SEND *************
			
			//Aufruf der Sendefunktion

		}
	}
}
