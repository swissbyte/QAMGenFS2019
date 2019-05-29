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

uint8_t ucQamSymbolsbufferA[8];
uint8_t ucQamSymbolsbufferB[8];
uint8_t ucActivebuffer;
volatile uint8_t ucQamBlockTransfer;

volatile uint8_t ucQamSymbolCount;




volatile uint8_t ucBuffer_a[2048];
volatile uint8_t ucBuffer_b[2048];

uint8_t ucNextLUTOffset;


#endif /* DMA_H_ */