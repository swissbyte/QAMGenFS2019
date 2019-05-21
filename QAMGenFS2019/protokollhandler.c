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
#include "string.h"

// KONSTANTEN
#define ANZSENDQUEUE					253							// gemäss Definition im Dokument "ProtokollBeschreibung.pdf" von Claudio
#define	PROTOKOLLBUFFERGROESSE			300
// xQuelle
#define PAKET_TYPE_ALDP					0x01
#define ALDP_SRC_UART					0x00
#define ALDP_SRC_I2C					0x01
#define ALDP_SRC_TEST					0x02
#define ALDP_SRC_ERROR					0xFF


// xSettings
#define Settings_QAM_Ordnung			1<<0
#define Settings_Source_Bit1			1<<1
#define Settings_Source_Bit2			1<<2
#define Settings_Frequenz				1<<3
// xStatus
#define Status_Error					1<<1
#define Status_Daten_ready				1<<2
#define Status_Daten_sending			1<<3
// xProtokollBufferStatus
#define BUFFER_A						1<<0
#define BUFFER_B						1<<1

EventGroupHandle_t xSettings;							// Settings vom GUI von Cedi
EventGroupHandle_t xStatus;								// auch irgendwas von Cedi

EventGroupHandle_t xProtokollBufferStatus;				// Eventbits für Status von Buffer von Protokoll-Task zu Modulator-Task

TaskHandle_t SendTask;
xQueueHandle xDataSendQueue;							// Daten zum Verpacken und Senden  (Daten von Cedi)

//globale Variablen
uint8_t uiglobalProtokollBuffer_A[PROTOKOLLBUFFERGROESSE];
uint8_t uiglobalProtokollBuffer_B[PROTOKOLLBUFFERGROESSE];


void vProtokollHandlerTask(void *pvParameters) {
	(void) pvParameters;
	
	struct ALDP_t_class *xALDP_Paket;
	struct SLDP_t_class xSLDP_Paket;
	
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	
	xDataSendQueue = xQueueCreate(ANZSENDQUEUE, sizeof(uint8_t));

	
	uint8_t	uibuffercounter = 0;


	// Debbuging	
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
			
//********** Daten aus Queue in Buffer speichern **********
			uibuffercounter = 2;
			uint8_t xSendQueueBuffer[uxQueueMessagesWaiting(xDataSendQueue)+2];
						
			while ((uxQueueMessagesWaiting(xDataSendQueue) > 0) && uibuffercounter < ANZSENDQUEUE ) {
				uint8_t xoutBufferPointer;
				xQueueReceive(xDataSendQueue, &xoutBufferPointer , portMAX_DELAY);					 
				xSendQueueBuffer[uibuffercounter] = xoutBufferPointer;
				uibuffercounter++;
			}
//********** ALDP Quelle in byte 1 **********
			if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
				if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
					// UART
					xSendQueueBuffer[0] = ALDP_SRC_UART;
				}
				else {
					// Testpattern
					xSendQueueBuffer[0] = ALDP_SRC_TEST;
				}
			}
			else {
				if ((xEventGroupGetBits(xSettings) & Settings_Source_Bit1) == 1) {
					// I2C
					xSendQueueBuffer[0] = ALDP_SRC_I2C;
				}
				else {
					// n.a. (Error)
					xSendQueueBuffer[0] = ALDP_SRC_ERROR;
				}
			}

//********** ALDP Size in byte 2 **********			
			xSendQueueBuffer[1] = uibuffercounter-2;
			
	// Daten aus Queue sind nun im xSendQueueBuffer			
				

//********** ALDP und SLDP mit Daten befüllen **********				
			xSLDP_Paket.sldp_size = sizeof(xSendQueueBuffer);
			xSLDP_Paket.sldp_payload = &xSendQueueBuffer[0];
			xALDP_Paket = (struct ALDP_t_class *) xSLDP_Paket.sldp_payload;
			
//******* SendeBuffer beschreiben *************
			
			//SLDP in buffer schreiben
			uint8_t outBuffer[xSLDP_Paket.sldp_size + 2];
			outBuffer[0] = xSLDP_Paket.sldp_size;

			uint8_t i = 0;
			for (i = 0; i != xSLDP_Paket.sldp_size; i++)	{
				outBuffer[i + 1] = xSLDP_Paket.sldp_payload[i];
			}
			xSLDP_Paket.sldp_crc8 = 0x66;															// CRC8 berechnen!
			outBuffer[xSLDP_Paket.sldp_size + 1] = xSLDP_Paket.sldp_crc8;			
			
			
			if ((xEventGroupGetBits(xSettings) & BUFFER_A) == 1) {
				memcpy(uiglobalProtokollBuffer_A, outBuffer, xSLDP_Paket.sldp_size+2);
				
			}
			
			
			
			
			vTaskDelay(50 / portTICK_RATE_MS);				// Delay 50ms
		}
	}
}
