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

#define  AnzSendQueue	253							// gemäss Definition im Dokument "ProtokollBeschreibung.pdf" von Claudio

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
void vLedBlink(void *pvParameters);

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
	
	uint8_t buffer[255];
	uint8_t buffercounter = 0;
	
	struct SLDP_t_class *SLDP_t_class;
	struct ALDP_t_class *ALDP_t_class;
	
	
	
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	for(;;) {
		PORTF.OUTTGL = 0x01;			
			
			while (uxQueueMessagesWaiting(DataSendQueue) > 0) {
				xQueueReceive(DataSendQueue, &buffer[buffercounter], portMAX_DELAY);
			}
		
		
		
			
		vTaskDelay(10 / portTICK_RATE_MS);			// Delay 10ms
	}
}
