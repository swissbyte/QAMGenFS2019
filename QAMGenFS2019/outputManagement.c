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

volatile uint8_t ucbuffer_a[2048];
volatile uint8_t ucbuffer_b[2048]; 

volatile uint8_t uclutOffset = 0; 
uint8_t ucnextLUTOffset = 0;
volatile uint8_t uclutCount = 0;
volatile uint8_t ucqamSymbolCount = 0;
volatile uint8_t ucqamBlockTransfer = 0;

void vSetDMA_LUT_Offset();



const uint16_t ussineLUT[SYMBOL_BUFFER_SIZE * 2] =
{
	/*
	*
	*Two Sinusodial Periods are saved here to easily move around with the offset!
	*/
	0x7f,0xb0,0xd9,0xf4,0xfe,0xf4,0xd9,0xb0,
	0x7f,0x4e,0x25,0xa,0x0,0xa,0x25,0x4e,
	0x7f,0xb0,0xd9,0xf4,0xfe,0xf4,0xd9,0xb0,
	0x7f,0x4e,0x25,0xa,0x0,0xa,0x25,0x4e,
	0x7f,0xb0,0xd9,0xf4,0xfe,0xf4,0xd9,0xb0,
	0x7f,0x4e,0x25,0xa,0x0,0xa,0x25,0x4e,
	0x7f,0xb0,0xd9,0xf4,0xfe,0xf4,0xd9,0xb0,
	0x7f,0x4e,0x25,0xa,0x0,0xa,0x25,0x4e
};

uint8_t ucqamSymbols[8] = {0,0,0,0,0,0,0,0};

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



	uclutOffset = ucqamSymbols[ucqamSymbolCount];

	if(ucqamBlockTransfer == 1)
	{
		ucqamBlockTransfer = 0;
		while ((DMA.CH0.CTRLB & (DMA_CH_CHBUSY_bm | DMA_CH_CHPEND_bm)));
	}

	if(ucqamSymbolCount != 0)
	{
		ucqamBlockTransfer = 2;
		ucqamSymbolCount --;		
	}
	else 
	{
		if(ucqamBlockTransfer == 2) ucqamBlockTransfer = 1;
	}	
	
	if(ucqamBlockTransfer)
	{	
		DMA.CH0.SRCADDR0	= ( (uint16_t) (&ussineLUT[0 + uclutOffset]) >> 0) & 0xFF;
		DMA.CH0.SRCADDR1	= ( (uint16_t) (&ussineLUT[0 + uclutOffset]) >> 8) & 0xFF;
		DMA.CH0.CTRLA		|= DMA_CH_ENABLE_bm;		
	}	
}

void vDMAIntHandler()
{
	/*
	*
	* Wenn wir nicht am Ararbeiten von Symbolen sind, dann gehen wir in diesen Task
	*/	


	if(ucqamBlockTransfer == 0)
	{
		
		BaseType_t xHigherPriorityTaskWoken, xResult;
		xHigherPriorityTaskWoken = pdFALSE;
		xResult = xEventGroupSetBitsFromISR(
									xDMAProcessEventGroup,  
									DMA_EVT_GRP_QAM_FINISHED, 
									&xHigherPriorityTaskWoken );
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
	* Timer für DMA initialisieren
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
	DMA.CH0.SRCADDR0	= ( (uint16_t) (&ussineLUT[0 + uclutOffset]) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1	= ( (uint16_t) (&ussineLUT[0 + uclutOffset]) >> 8) & 0xFF;
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
	vDMAIntHandler();
}
