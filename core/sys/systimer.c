/**
 * EFM32 protothreads
 * Copyright (C) 2014 Erki Aring
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdint.h>

#include <protothreads.h>

#include "lpm.h"
#include "systime.h"
#include "systimer.h"


/** Pointer to first timer. */
static SYSTIMER *FirstTimer = NULL;


static void SYSTIMER_TriggerHandler (void)
{
    process_poll (&SYSTIMER_Process);
    LPM_RegisterEvent ();
}


/**
 * @brief  Set up the RTC to trigger when the next timer is to be timed out.
 * @param  current_time Current time (used to avoid another call to clock_gettime ())
 * @retval None.
 */
static void SYSTIMER_SetTrigger ()
{
    if (FirstTimer != NULL) {
        SYSTIME_Trigger (&FirstTimer->target, SYSTIMER_TriggerHandler);
    }
}


// Add timer to the list
static void SYSTIMER_Add (SYSTIMER *timer)
{
    SYSTIMER *tmr = FirstTimer, *prv = NULL;
    while (1) {
        if (tmr == NULL || SYSTIME_CMP (&tmr->target, &timer->target) >= 0) {
            timer->next = tmr;
            timer->prev = prv;
            if (tmr != NULL) {
                tmr->prev = timer;
            }
            if (prv != NULL) {
                prv->next = timer;
            } else {
                FirstTimer = timer;
            }
            break;
        }
        prv = tmr;
        tmr = tmr->next;
    }
}


// Remove timer from the list
static void SYSTIMER_Remove (SYSTIMER *timer)
{
    if (timer->prev) {
        timer->prev->next = timer->next;
    } else {
        FirstTimer = timer->next;
    }
    if (timer->next) {
        timer->next->prev = timer->prev;
    }
    timer->prev = NULL;
    timer->next = NULL;
}


/**
 * @brief  Check the timed out timers, execute their callbacks and set up next RTC trigger.
 * @retval None.
 */
PROCESS (SYSTIMER_Process, "System Timer Process");
PROCESS_THREAD (SYSTIMER_Process, ev, data)
{
    PROCESS_BEGIN ();

    SYSTIMER *timer;
    SYSTIMER *readd, *temp;
    struct timespec current_time;

    while (1) {

        SYSTIMER_SetTrigger ();

        PROCESS_WAIT_EVENT_UNTIL (ev == PROCESS_EVENT_POLL);

        clock_gettime (CLOCK_REALTIME, &current_time);

        // In case we have many timers expiring at the same time
        readd = NULL;

        /* Handle timer events and reorder the list */
        timer = FirstTimer;
        while (timer) {
            temp = timer->next;
            if (SYSTIME_CMP (&timer->target, &current_time) <= 0) {
                if (timer->callback != NULL) {
                    timer->callback (timer->arg);
                }
                // Check if timer was stopped inside the callback
                if (timer->running) {
                    if (SYSTIME_ISSET (&timer->interval)) {
                        SYSTIMER_Remove (timer);
                        timer->next = readd;
                        readd = timer;
                        // SYSTIME_ADD (&timer->target, &timer->target, &timer->interval);
                        // SYSTIMER_Add (timer);
                    } else {
                        SYSTIMER_Stop (timer);
                    }
                }
            } else {
                break;
            }
            timer = temp;
        }

        while (readd) {
            temp = readd->next;
            SYSTIME_ADD (&readd->target, &readd->target, &readd->interval);
            SYSTIMER_Add (readd);
            readd = temp;
        }
    }

    PROCESS_END ();
}


/**
 * @brief   Initialize the timer.
 * @param   timer Pointer to timer structure.
 * @param   timeout Timeout value.
 * @param   reload Reload flag, 0 - no reload, else reload.
 * @param   callback Pointer to callback function.
 * @param   arg Argument to be passed to the callback function.
 * @retval  None.
 */
void SYSTIMER_Init_NoStart (SYSTIMER *timer, uint32_t timeout, uint32_t interval, int (*callback) (void*), void *arg)
{
    timer->target.tv_sec = 0;
    timer->target.tv_nsec = 0;
    timer->running = 0;
    timer->timeout.tv_sec = (timeout / 1000);
    timer->timeout.tv_nsec = (timeout % 1000) * 1000000;
    timer->interval.tv_sec = (interval / 1000);
    timer->interval.tv_nsec = (interval % 1000) * 1000000;
    timer->callback = callback;
    timer->arg = arg;
    timer->next = NULL;
    timer->prev = NULL;
}


/**
 * @brief   Initialize and start the timer.
 * @see     SYSTIMER_Init_NoStart()
 */
void SYSTIMER_Init (SYSTIMER *timer, uint32_t timeout, uint32_t interval, int (*callback) (void*), void *arg)
{
    SYSTIMER_Init_NoStart (timer, timeout, interval, callback, arg);
    SYSTIMER_Start (timer);
}


/**
 * @brief  Start the timer.
 * @param  timer Pointer to timer structure.
 * @retval None.
 */
void SYSTIMER_Start (SYSTIMER *timer)
{
    // Check that timer is not already running
    if (timer->running) {
        return;
    } else {
        timer->running = 1;
    }

    struct timespec current_time;
    clock_gettime (CLOCK_REALTIME, &current_time);

    // Update the target, if timer is stopped
    if (timer->target.tv_sec == 0 && timer->target.tv_nsec == 0) {
        SYSTIMER_Reset (timer);
    } else {
        SYSTIME_SUB (&timer->target, &timer->target, &timer->started);
        SYSTIME_ADD (&timer->target, &timer->target, &current_time);
        SYSTIME_SET (&timer->started, &current_time);
        timer->started.tv_nsec = (timer->started.tv_nsec / 1000000) * 1000000;
    }

    SYSTIMER_Add (timer);
    SYSTIMER_SetTrigger ();
}


/**
 * @brief  Pause the timer. To resume, call SYSTIMER_Start().
 * @param  timer Pointer to timer structure.
 * @retval None.
 */
void SYSTIMER_Pause (SYSTIMER *timer)
{
    // Check that timer is really running
    if (!timer->running) {
        return;
    } else {
        timer->running = 0;
    }

    SYSTIMER_Remove (timer);
}


/**
 * @brief  Stop the timer.
 * @param  timer Pointer to timer structure.
 * @retval None.
 */
void SYSTIMER_Stop (SYSTIMER *timer)
{
    SYSTIMER_Pause (timer);
    SYSTIME_RESET (&timer->target);
}


/**
 * @brief  Reset the timer to the timeout value.
 * @param  timer Pointer to timer structure.
 * @retval None.
 */
void SYSTIMER_Reset (SYSTIMER *timer)
{
    clock_gettime (CLOCK_REALTIME, &timer->started);
    timer->started.tv_nsec = (timer->started.tv_nsec / 1000000) * 1000000;
    SYSTIME_ADD (&timer->target, &timer->started, &timer->timeout);
}


/**
 * @brief  Check if the timer has timed out.
 * @param  timer Pointer to timer structure.
 * @retval 0 if not ready.
 */
int SYSTIMER_IsReady (SYSTIMER *timer)
{
    struct timespec current_time;
    clock_gettime (CLOCK_REALTIME, &current_time);

    if (SYSTIME_CMP (&timer->target, &current_time) <= 0) {
        return -1;
    } else {
        return 0;
    }
}


/**
 * @brief  Check if the timer is running.
 * @param  timer Pointer to timer structure.
 * @retval 0 if not running, else running.
 */
int SYSTIMER_IsRunning (SYSTIMER *timer)
{
  return timer->running;
}
