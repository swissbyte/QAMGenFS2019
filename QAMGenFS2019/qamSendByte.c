/*
 * qamSendByte.c
 *
 *  @version 1.0
 *  @author Claudio Hediger, Michael Meier
 */ 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "qamSendByte.h"
#include "outputManagement.h"

#include "protocolhandler.h"
uint8_t volatile ucNoData= 0;


EventGroupHandle_t xDMAProcessEventGroup;

void ucqamSendByte(uint8_t ucByte)
{
	/**
     * Teilt das Byte in 4 Symbole und speichert diese im Buffer
     */
	uint8_t ucQamLUT[4] = {0,8,16,24};
	uint8_t ucSymbol = 0;
	
	for (ucSymbol=0; ucSymbol!=4; ucSymbol ++)
	{

		if(ucActivebuffer)
		{
		ucDataReadyB = 0;
		ucQamSymbolsbufferB[ucSymbol] = ucQamLUT[(ucByte >> (2 * ucSymbol))  & 0x03];
		
		}
		else
		{
		ucDataReadyA = 0;
		ucQamSymbolsbufferA[ucSymbol] = ucQamLUT[(ucByte >> (2 * ucSymbol))  & 0x03];
		}
	}

	if(!ucActivebuffer)
	{
	ucDataReadyA = 1;
	}	
	else
	{
	ucDataReadyB = 1;
	}

}

void vTask_DMAHandler(void *pvParameters) 
{

	vSetDMA_LUT_Offset();

	uint8_t i = 0;
	xByteSent = xSemaphoreCreateMutex();

	xSemaphoreTake(xByteSent, portMAX_DELAY); //Semaphore ist bereits weg....
	
	
	/* CPU LOAD Generator 
		Uncomment this section to Generate some CPU-Load. 
	*/
	/*
	uint32_t lastTick = 0;
	lastTick = xTaskGetTickCount();
	while(1)
	{
		if((xTaskGetTickCount() - lastTick) > 500) 
		{
			// a delay of 50ms generates a load of 98%, 300 a load of 75%
			vTaskDelay(50/portTICK_PERIOD_MS);
			lastTick = xTaskGetTickCount();
		}
	}
	*/
	
	while(1)
	{			
			xSemaphoreTake(xGlobalFrameBuffer_A_Key,portMAX_DELAY);
			//Erster Buffer vorbereiten. 
			if(ucNoData)
			{
				ucqamSendByte(ucGlobalFrameBuffer_A[1]);
				ucNoData = 0;
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 2; i > ucGlobalFrameBuffer_A[0]; i++)
				{
					ucqamSendByte(ucGlobalFrameBuffer_A[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			else
			{
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 1; i >ucGlobalFrameBuffer_A[0]; i++)
				{
					ucqamSendByte(ucGlobalFrameBuffer_A[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}	
			}
			xSemaphoreGive(xGlobalFrameBuffer_A_Key);
			xSemaphoreTake(xGlobalFrameBuffer_B_Key,portMAX_DELAY);
			//Erster Buffer vorbereiten.
			if(ucNoData)
			{
				ucqamSendByte(ucGlobalFrameBuffer_B[1]);
				ucNoData = 0;
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!
				for(i = 2; i > ucGlobalFrameBuffer_B[0]; i++)
				{
					ucqamSendByte(ucGlobalFrameBuffer_B[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			else
			{
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 1; i >ucGlobalFrameBuffer_B[0]; i++)
				{
					ucqamSendByte(ucGlobalFrameBuffer_B[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			xSemaphoreGive(xGlobalFrameBuffer_B_Key);
	}
	
}