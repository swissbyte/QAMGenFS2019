/*
 * QAMGenFS2019.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos, philippeppler
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "queue.h"

#include "mem_check.h"

#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

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

struct ALDP_t_class
{
	uint8_t aldp_hdr_byte_1;
	uint8_t aldp_hdr_byte_2;
	uint8_t *aldp_payload;
};

struct SLDP_t_class
{
	uint8_t sldp_size;
	uint8_t *sldp_payload;
	uint8_t sldp_crc8;
};

extern void vApplicationIdleHook( void );
void vSendTask(void *pvParameters);

TaskHandle_t SendTask;
xQueueHandle DataSendQueue;							// Daten zum Verpacken und Senden

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vSendTask, (const char *) "SendTask", configMINIMAL_STACK_SIZE+1000, NULL, 1, NULL);
	DataSendQueue = xQueueCreate(AnzSendQueue, sizeof(uint8_t));

	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}

void vSendTask(void *pvParameters) {
	(void) pvParameters;
	
	uint8_t buffercounter = 0;
	
	struct SLDP_t_class *SLDP_Paket;
	struct ALDP_t_class *ALDP_Paket;
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	for(;;) {
		PORTF.OUTTGL = 0x01;	
			
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
				xQueueReceive(DataSendQueue, ALDP_Paket->aldp_payload + buffercounter, portMAX_DELAY);
				buffercounter++;
			}
			ALDP_Paket->aldp_hdr_byte_1 = buffercounter;
		
		
			
		vTaskDelay(50 / portTICK_RATE_MS);			// Delay 50ms
	}
}
