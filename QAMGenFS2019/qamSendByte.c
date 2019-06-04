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
		ucQamSymbolsbufferB[ucSymbol] = ucQamLUT[(ucByte >> (2 * ucSymbol))  & 0x03];
		}
		else
		{
		ucQamSymbolsbufferA[ucSymbol] = ucQamLUT[(ucByte >> (2 * ucSymbol))  & 0x03];
		}
	}
	ucQamSymbolCount = 3;	
}

void vTask_DMAHandler(void *pvParameters) 
{
	
	xDMAProcessEventGroup = xEventGroupCreate();
	EventBits_t uxBits;
	
	ucqamSendByte(0xE4);
	vSetDMA_LUT_Offset();
	while(1)
	{
		//uxBits = xEventGroupWaitBits(xDMAProcessEventGroup,	DMA_EVT_GRP_QAM_FINISHED, pdTRUE, pdFALSE, portMAX_DELAY);
					
		if(DMA_EVT_GRP_QAM_FINISHED && ucQamBlockTransfer==0)
		{
		
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x02));
			PORTF.OUT |= 0x04;
			
			ucqamSendByte(0xcc);
			vStartQAMTransfer();			
		}

	}
	
}