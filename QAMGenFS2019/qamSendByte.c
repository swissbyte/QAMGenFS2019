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
	while(1)
	{			
			xSemaphoreTake(xGlobalProtocolBuffer_A_Key,portMAX_DELAY);
			//Erster Buffer vorbereiten. 
			if(ucNoData)
			{
				ucqamSendByte(ucglobalProtocolBuffer_A[1]);
				ucNoData = 0;
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 2; i > ucglobalProtocolBuffer_A[0]; i++)
				{
					ucqamSendByte(ucglobalProtocolBuffer_A[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			else
			{
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 1; i >ucglobalProtocolBuffer_A[0]; i++)
				{
					ucqamSendByte(ucglobalProtocolBuffer_A[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}	
			}
			xSemaphoreGive(xGlobalProtocolBuffer_A_Key);
			//xSemaphoreTake(xGlobalProtocolBuffer_B_Key,portMAX_DELAY);
			//Erster Buffer vorbereiten.
			if(ucNoData)
			{
				ucqamSendByte(ucglobalProtocolBuffer_B[1]);
				ucNoData = 0;
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!
				for(i = 2; i > ucglobalProtocolBuffer_B[0]; i++)
				{
					ucqamSendByte(ucglobalProtocolBuffer_B[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			else
			{
				//i = 0 = Size Byte
				//i = 1 = oben verarbeitet
				//i = 2!!!

				for(i = 1; i >ucglobalProtocolBuffer_B[0]; i++)
				{
					ucqamSendByte(ucglobalProtocolBuffer_B[i]);
					xSemaphoreTake(xByteSent, portMAX_DELAY);
				}
			}
			xSemaphoreGive(xGlobalProtocolBuffer_B_Key);
	}
	
}