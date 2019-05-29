/*
 * qamSendByte.h
 *
 *  @version 1.0
 *  @author Claudio Hediger, Michael Meier
 */ 


#ifndef TASKS_H_
#define TASKS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"


void vTask_DMAHandler(void *pvParameters);

#define DMA_EVT_GRP_QAM_FINISHED  ( 1 << 0 )

EventGroupHandle_t xDMAProcessEventGroup;


#endif /* TASKS_H_ */