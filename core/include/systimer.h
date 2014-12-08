/*
 * systimer.h
 *
 *  Created on: 27.11.2014
 *      Author: erkia
 */

#ifndef SYSTIMER_H_
#define SYSTIMER_H_

#include <protothreads.h>

#include "systime.h"

/** Timer Structure. */
typedef struct SYSTIMER_S SYSTIMER;
struct SYSTIMER_S
{
  int               running;
  struct timespec   started;
  struct timespec   target;
  struct timespec   timeout;
  struct timespec   interval;
  int               (*callback) (void *);
  void              *arg;
  SYSTIMER          *next;
  SYSTIMER          *prev;
};

PROCESS_NAME (SYSTIMER_Process);

void SYSTIMER_Init_NoStart (SYSTIMER *timer, uint32_t timeout, uint32_t interval, int (*callback) (void*), void *arg);
void SYSTIMER_Init (SYSTIMER *timer, uint32_t timeout, uint32_t interval, int (*callback) (void*), void *arg);
void SYSTIMER_Set_Timeout (SYSTIMER *timer, uint32_t timeout);
void SYSTIMER_Start (SYSTIMER *timer);
void SYSTIMER_Pause (SYSTIMER *timer);
void SYSTIMER_Stop (SYSTIMER *timer);
void SYSTIMER_Reset (SYSTIMER *timer);
int SYSTIMER_IsReady (SYSTIMER *timer);
int SYSTIMER_IsRunning (SYSTIMER *timer);

#endif /* SYSTIMER_H_ */
