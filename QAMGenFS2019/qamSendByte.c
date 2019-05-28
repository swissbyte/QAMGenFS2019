
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

void ucqamSendByte(uint8_t ucbyte)
{
	/**
     * Teilt das Byte in 4 Symbole und speichert diese im Buffer
     */
	uint8_t ucqamLUT[4] = {0,8,16,24};
	uint8_t ucsymbol = 0;
	
	for (ucsymbol=0; ucsymbol!=4; ucsymbol ++)
	{
		ucqamSymbols[ucsymbol] = ucqamLUT[(ucbyte >> (2 * ucsymbol))  & 0x03];
	}
	ucqamSymbolCount = 3;	
}

void vTask_DMAHandler(void *pvParameters) 
{
	
	xDMAProcessEventGroup = xEventGroupCreate();
	EventBits_t uxBits;
	
	ucqamSendByte(0xE4);
	vSetDMA_LUT_Offset();
	while(1)
	{
		uxBits = xEventGroupWaitBits(
		xDMAProcessEventGroup,   
		DMA_EVT_GRP_QAM_FINISHED, 
		pdTRUE,       
		pdFALSE,   
		portMAX_DELAY );
			
		
		if(uxBits & DMA_EVT_GRP_QAM_FINISHED)
		{
		
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x02));
			PORTF.OUT |= 0x04;
			
			ucqamSendByte(0x00);
			vStartQAMTransfer();			
		}

	}
	
}