/*
 * outputManagement.h
 *
 *  @version 1.0
 *  @author Claudio Hediger, Michael Meier
 */ 


#include "semphr.h"
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

uint8_t ucDataReadyA;
uint8_t ucDataReadyB;

volatile uint8_t ucQamBlockTransfer;

volatile uint8_t ucQamSymbolCount;


uint8_t ucNextLUTOffset;

SemaphoreHandle_t xByteSent;

#endif /* DMA_H_ */