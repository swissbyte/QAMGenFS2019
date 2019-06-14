/*
 * FreeRTOS10_Template2.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos, Michael Meier, Cedric H�uptli
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
#include "semphr.h"
#include "event_groups.h"
#include "stack_macros.h"
#include "queue.h"

#include "mem_check.h"

#include "outputManagement.h"
#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"
#include "protocolhandler.h"

#include "qamSendByte.h"
#include "Menu_IMU.h"


extern void vApplicationIdleHook( void );
TaskHandle_t xTaskDMAHandler;


volatile uint8_t ucCPULoad = 0;

/**
* vApplicationIdleHook calculates the actual CPU load based on Ticks.
* @param none
* @return none
* @author C. Hediger
*/
void vApplicationIdleHook( void )
{	
	static uint32_t idleTicks = 0;
	static uint32_t lastTick = 0;
	static uint32_t beginTicks = 0;
	/* Calculating CPU Load for a period of 3 Seconds. */
	if((xTaskGetTickCount()-beginTicks) > 3000)
	{
		ucCPULoad = (uint8_t)(100.0 - ((idleTicks / 3.0) / 10.0));
		idleTicks = 0;		
		beginTicks = xTaskGetTickCount();
	}
	else
	{
		if(lastTick != xTaskGetTickCount())
		{	
			if((xTaskGetTickCount() - lastTick) == 1)
			{
				idleTicks++;
				lastTick = xTaskGetTickCount();
			}
			else
			{
				lastTick = xTaskGetTickCount();
			}
		}
	}
}

int main(void)
{
	vInitClock();
	vInitDisplay();
	
	vInitDAC();
	vInitDMATimer();
	vInitDMA();
	
	xGlobalFrameBuffer_A_Key = xSemaphoreCreateMutex();
	xGlobalFrameBuffer_B_Key = xSemaphoreCreateMutex();
	xSettingKey = xSemaphoreCreateMutex();	
	xStatusKey = xSemaphoreCreateMutex();	


	xTaskCreate( vMenu, (const char *) "Menu", configMINIMAL_STACK_SIZE, NULL, 1, &xMenu);
	xTaskCreate( vIMU, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xIMU);
	xTaskCreate( vTestpattern, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xTestpattern);
	xTaskCreate( vProtokollHandlerTask, (const char *) "Protocol", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate( vOutput, (const char *) "IMU", configMINIMAL_STACK_SIZE, NULL, 1, &xIO);
	xTaskCreate( vTask_DMAHandler, (const char *) "Modulator", configMINIMAL_STACK_SIZE , NULL, 2, NULL);


	xALDPQueue = xQueueCreate( 10, sizeof(struct ALDP_t_class));
	xDatabriged = xQueueCreate( 10, sizeof(uint8_t) );	
	

	
	vDisplayClear();
	vTaskStartScheduler();
	return 0;
}
