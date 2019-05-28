/*
 * outputManagement.h
 *
 *  @version 1.0
 *  @author Claudio Hediger, Michael Meier
 */ 


#ifndef DMA_H_
#define DMA_H_

void vInitDMA();
void vInitDMATimer();
void vInitDAC();
void vStartQAMTransfer();

#define QAM_4_SYM_00	0
#define QAM_4_SYM_01	8
#define QAM_4_SYM_10	16
#define QAM_4_SYM_11	24

uint8_t ucqamSymbols[8];
volatile uint8_t ucqamSymbolCount;




volatile uint8_t ucbuffer_a[2048];
volatile uint8_t ucbuffer_b[2048];

uint8_t ucnextLUTOffset;


#endif /* DMA_H_ */