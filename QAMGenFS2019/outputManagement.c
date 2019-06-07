/*
 * outputManagement.c
 *
 *  @version 1.0
 *  @author Claudio Hediger, Michael Meier
 */ 

#include "stdint.h"
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"
#include "qamSendByte.h"
#include "outputManagement.h"

#include "FreeRTOS.h"
#include "task.h"

#define SYMBOL_BUFFER_SIZE	32
#define DAC_IDLE_VOLTAGE 0


volatile uint8_t ucLutOffset = 0; 
uint8_t ucNextLUTOffset = 0;
volatile uint8_t ucLutCount = 0;
volatile uint8_t ucQamSymbolCount = 0;
volatile uint8_t ucQamBlockTransfer = 0;
uint8_t ucActivebuffer= 0;

uint8_t ucQamSymbolsbufferA[8] = {0,0,0,0,0,0,0,0};
uint8_t ucQamSymbolsbufferB[8] = {0,0,0,0,0,0,0,0};
	
void vSetDMA_LUT_Offset();



const uint16_t usSineLUT[SYMBOL_BUFFER_SIZE * 2] =
{
	/*
	*
	*Two Sinusodial Periods are saved here to easily move around with the offset!
	*/
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
};



void vInitDAC()
{
	/*
	*
	*Initialize DAC
	*/
	DACB.CTRLA = DAC_CH0EN_bm;
	DACB.CTRLB = DAC_CH0TRIG_bm;	
	DACB.CTRLC = DAC_REFSEL_AVCC_gc | DAC_LEFTADJ_bm; 
	DACB.EVCTRL = DAC_EVSEL_0_gc;	
	DACB.CTRLA |= DAC_ENABLE_bm;
	PORTB.DIRSET = 0x04;
}


void vStartQAMTransfer(void)
{
	vSetDMA_LUT_Offset();
}


void vSetDMA_LUT_Offset()
{
	/*
	*
	*Die Transaktion wurde gestartet und wir warten
	*/


	if(ucActivebuffer)
	{
		ucLutOffset = ucQamSymbolsbufferB[ucQamSymbolCount];
	}
	else
	{
		ucLutOffset = ucQamSymbolsbufferA[ucQamSymbolCount];
	}

	if(ucQamBlockTransfer == 1)
	{
		ucQamBlockTransfer = 0;
		if(ucActivebuffer)
		{
			ucActivebuffer=0;
		}
		else
		{
			ucActivebuffer=1;
		}
	}

	if(ucQamSymbolCount != 0)
	{
		ucQamBlockTransfer = 2;
		ucQamSymbolCount --;		
	}
	else 
	{
		if(ucQamBlockTransfer == 2) ucQamBlockTransfer = 1;
	}	
	
	if(ucQamBlockTransfer)
	{	
		DMA.CH0.SRCADDR0	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 0) & 0xFF;
		DMA.CH0.SRCADDR1	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 8) & 0xFF;
		DMA.CH0.CTRLA		|= DMA_CH_ENABLE_bm;		
	}	
}

void vDMAIntHandler()
{
	/*
	*
	* Wenn wir nicht am Ararbeiten von Symbolen sind, dann gehen wir in diesen Task
	*/	


	if(ucQamBlockTransfer == 0)
	{
		
		BaseType_t xHigherPriorityTaskWoken, xResult;
		xHigherPriorityTaskWoken = pdFALSE;
		xResult = xEventGroupSetBitsFromISR(xDMAProcessEventGroup,DMA_EVT_GRP_QAM_FINISHED,&xHigherPriorityTaskWoken );
		if( xResult != pdFAIL )
		{
			/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
			switch should be requested.  The macro used is port specific and will
			be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
			the documentation page for the port being used. */

			//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
	
}

void vInitDMATimer()
{
	/*
	*
	* Timer f�r DMA initialisieren
	*/	

	TCC1.CTRLA = 0; 
	TCC1.CTRLB = TC_WGMODE_NORMAL_gc;
	TCC1.CTRLD = TC_EVACT_RESTART_gc;
	TCC1.CNT = 0;
	TCC1.PER = 0x001F; 
	EVSYS.CH0MUX = EVSYS_CHMUX_TCC1_OVF_gc;
}

void vInitDMA()
{
	/*
	*
	* DMA initialisieren
	*/	


	DMA.CTRL = 0;
	DMA.CTRL = DMA_RESET_bm;
	while ((DMA.CTRL & DMA_RESET_bm) != 0);
	
	DMA.CTRL = DMA_CH_ENABLE_bm; 
	DMA.CH0.REPCNT		= 1;
	DMA.CH0.CTRLA		= DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm;
	DMA.CH0.CTRLB		= 0x1;
	DMA.CH0.ADDRCTRL	= DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_FIXED_gc |  DMA_CH_SRCRELOAD_TRANSACTION_gc;
	DMA.CH0.TRIGSRC		= DMA_CH_TRIGSRC_EVSYS_CH0_gc;
	DMA.CH0.TRFCNT		= SYMBOL_BUFFER_SIZE; 
	DMA.CH0.DESTADDR0	= ((uint16_t)(&DACB.CH0DATAH)>>0) & 0xFF;
	DMA.CH0.DESTADDR1	= ((uint16_t)(&DACB.CH0DATAH)>>8) & 0xFF;
	DMA.CH0.DESTADDR2	= 0;
	DMA.CH0.SRCADDR0	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 8) & 0xFF;
	DMA.CH0.SRCADDR2	= 0;
	
	TCC1.CTRLA			= TC_CLKSEL_DIV64_gc; 
}

ISR(DMA_CH0_vect)
{	
	/*
	*
	* Interrupt nach gesendetem Byte
	*/	

	DMA.CH0.CTRLB |= 0x10;
	vSetDMA_LUT_Offset();
	//vDMAIntHandler();
}
