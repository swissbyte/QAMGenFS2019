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
#include "semphr.h"

#define SYMBOL_BUFFER_SIZE	32
#define DAC_IDLE_VOLTAGE 0


volatile uint8_t ucLutOffset = 0; 
uint8_t ucNextLUTOffset = 0;
volatile uint8_t ucLutCount = 0;
volatile uint8_t ucQamSymbolCount = 3;
volatile uint8_t ucQamBlockTransfer = 0;

volatile uint8_t ucDataReady = 0;
uint8_t ucActivebuffer=0;
uint8_t ucDataReadyA=0;
uint8_t ucDataReadyB=0;

uint8_t ucQamSymbolsbufferA[8] = {0,0,0,0,0,0,0,0};
uint8_t ucQamSymbolsbufferB[8] = {0,0,0,0,0,0,0,0};


	

void vConfigureDMASource(void);


const uint16_t usSineLUT[SYMBOL_BUFFER_SIZE * 2] =
{
	/*
	*
	*Two Sinusodial Periods are saved here to easily move around with the offset!
	*/
	
	115,137,159,179,196,211,221,228,
	230,228,221,211,196,179,159,137,
	115,93,71,51,34,19,9,2,
	0,2,9,19,34,51,71,93,
	115,137,159,179,196,211,221,228,
	230,228,221,211,196,179,159,137,
	115,93,71,51,34,19,9,2,
	0,2,9,19,34,51,71,93,
	
	
	
	/*
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,
	0xfe,0xf4,0xd9,0xb0,0x7f,0x4e,0x25,0xa,
	0x0,0xa,0x25,0x4e,0x7f,0xb0,0xd9,0xf4,*/
};



void vInitDAC()
{
	/*
	*
	*Initialize DAC
	*/
	DACB.CTRLA = DAC_CH0EN_bm;
	DACB.CTRLB = DAC_CH0TRIG_bm;	
	DACB.CTRLC = DAC_REFSEL_INT1V_gc | DAC_LEFTADJ_bm; 
	DACB.EVCTRL = DAC_EVSEL_0_gc;	
	DACB.CTRLA |= DAC_ENABLE_bm;
	PORTB.DIRSET = 0x04;
}


void vDoDMAStuff(void)
{
	
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
			ucQamSymbolCount = 4;	
			xSemaphoreGiveFromISR(xByteSent,NULL);
		}

		if(ucQamSymbolCount != 1)
		{
			ucQamBlockTransfer = 2;
			ucQamSymbolCount --;
		}
		else
		{
			if(ucQamBlockTransfer == 2) ucQamBlockTransfer = 1;
			ucQamSymbolCount --;
		}
		
		if(ucQamBlockTransfer)
		{
			vConfigureDMASource();
		}
	
	/*
		vConfigureDMASource();
		ucQamSymbolCount++;
		if(ucQamSymbolCount == 3)
		{
			ucQamSymbolCount = 0;
			xSemaphoreGiveFromISR(xByteSent,NULL);			
		}
		else
		{
			if(ucQamBlockTransfer == 1)
			{
				if(ucActivebuffer) ucActivebuffer=0;
				else ucActivebuffer=1;
				ucQamBlockTransfer = 0;
			}
			
		}*/
	
}

void vConfigureDMASource(void)
{
	DMA.CH0.SRCADDR0	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 8) & 0xFF;
	DMA.CH0.CTRLA		|= DMA_CH_ENABLE_bm;
}


void vSetDMA_LUT_Offset()
{
	/*
	*
	*Die Transaktion wurde gestartet und wir warten
	*/
	
	if(ucActivebuffer)
	{
		if(ucDataReadyB)
		{
			ucLutOffset = ucQamSymbolsbufferB[ucQamSymbolCount];
			vDoDMAStuff();
		}
		else
		{
			//Keine Daten....
			ucNoData = 1;
			ucLutOffset =0;
			//DACB.CH0DATA = 0x00;
			vConfigureDMASource();
		}
	}
	else
	{
		if(ucDataReadyA)
		{
			ucLutOffset = ucQamSymbolsbufferA[ucQamSymbolCount];
			vDoDMAStuff();
		}
		else
		{
			//Keine Daten....
			ucNoData = 1;
			ucLutOffset =0;
			vConfigureDMASource();
		}
	}
}

/*void vDMAIntHandler()
{
	
	*
	* Wenn wir nicht am Ararbeiten von Symbolen sind, dann gehen wir in diesen Task
	


	if(ucQamBlockTransfer == 0)
	{
		
		BaseType_t xHigherPriorityTaskWoken, xResult;
		xHigherPriorityTaskWoken = pdFALSE;
		xResult = xEventGroupSetBitsFromISR(xDMAProcessEventGroup,DMA_EVT_GRP_QAM_FINISHED,&xHigherPriorityTaskWoken );
		if( xResult != pdFAIL )
		{
			* If xHigherPriorityTaskWoken is now set to pdTRUE then a context
			switch should be requested.  The macro used is port specific and will
			be either portYIELD_FROM_ISR() or portEND_SWITCHING_ISR() - refer to
			the documentation page for the port being used. 

			//portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
		}
	}
	
}*/

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
	TCC1.PER = 320-1; 
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
	
	
	DMA.CH0.REPCNT		= 1;
	DMA.CH0.CTRLA		= DMA_CH_BURSTLEN_2BYTE_gc | DMA_CH_SINGLE_bm ;
	DMA.CH0.CTRLB		= 0x1;
	DMA.CH0.ADDRCTRL	= DMA_CH_SRCDIR_INC_gc | DMA_CH_DESTDIR_FIXED_gc |  DMA_CH_SRCRELOAD_TRANSACTION_gc;
	DMA.CH0.TRIGSRC		= DMA_CH_TRIGSRC_EVSYS_CH0_gc;
	DMA.CH0.TRFCNT		= SYMBOL_BUFFER_SIZE*2; 
	DMA.CH0.DESTADDR0	= ((uint16_t)(&DACB.CH0DATAH)>>0) & 0xFF;
	DMA.CH0.DESTADDR1	= ((uint16_t)(&DACB.CH0DATAH)>>8) & 0xFF;
	DMA.CH0.DESTADDR2	= 0;
	DMA.CH0.SRCADDR0	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 0) & 0xFF;
	DMA.CH0.SRCADDR1	= ( (uint16_t) (&usSineLUT[0 + ucLutOffset]) >> 0) & 0xFF;
	DMA.CH0.SRCADDR2	= 0;
	
	DACB.CH0DATAL = 0;
	DMA.CTRL = DMA_CH_ENABLE_bm; 
	
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
