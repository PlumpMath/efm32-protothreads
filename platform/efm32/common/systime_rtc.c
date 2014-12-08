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

#define RTCDRV_CLKDIV           cmuClkDiv_1
#define RTCDRV_FREQ             32768U
#define RTCDRV_CNT_MASK         (_RTC_CNT_CNT_MASK >> _RTC_CNT_CNT_SHIFT)
#define RTCDRV_CNT_MAX          (RTCDRV_CNT_MASK + 1)
#define RTCDRV_SEC_PER_OF       (RTCDRV_CNT_MAX / RTCDRV_FREQ)
#define RTCDRC_COMP_SET_MIN     1

#include <stddef.h>
#include <math.h>

#include <em_device.h>
#include <em_assert.h>
#include <em_cmu.h>
#include <em_rtc.h>

#include "systime_rtc.h"

typedef struct {
    volatile uint32_t overflow_counter;
    void              (*compare_callback)(void);
} RTCControl;


static RTCControl RTCCtrl;


/***************************************************************************//**
 * @brief RTC Interrupt Handler, invoke callback function if defined.
 ******************************************************************************/
void RTC_IRQHandler (void)
{
    if (RTC_IntGet () & RTC_IF_COMP0) {

        /* Clear interrupt source */
        RTC_IntClear (RTC_IFC_COMP0);

        /* Trigger callback if defined */
        if (RTCCtrl.compare_callback) {
            void (*temp)(void) = RTCCtrl.compare_callback;
            RTCCtrl.compare_callback = NULL;
            temp ();
        }

    }

    if (RTC_IntGet () & RTC_IF_OF) {

        /* Clear interrupt source */
        RTC_IntClear (RTC_IFC_OF);

        /* Increase the overflow counter */
        RTCCtrl.overflow_counter++;

    }
}


/***************************************************************************//**
 * @brief
 *  Setup RTC with selected clock source and prescaler.
 *
 * @param rtcPrescale
 *  RTC prescaler
 ******************************************************************************/
static int RTCDRV_Init (void)
{
    RTCCtrl.overflow_counter = 0;
    RTCCtrl.compare_callback = NULL;

    /* Ensure LE modules are accessible */
    CMU_ClockEnable (cmuClock_CORELE, true);
    CMU_ClockEnable (cmuClock_RTC, true);
    CMU_ClockDivSet (cmuClock_RTC, RTCDRV_CLKDIV);

    RTC_Init_TypeDef init = RTC_INIT_DEFAULT;

    init.enable = false;
    init.debugRun = false;
    init.comp0Top = false;

    RTC_Init (&init);

    EFM_ASSERT (CMU_ClockFreqGet (cmuClock_RTC) == RTCDRV_FREQ);

    /* Enable RTC counter overflow interrupt */
    RTC_IntEnable (RTC_IEN_OF | RTC_IEN_COMP0);

    /* Enable interrupts */
    NVIC_ClearPendingIRQ (RTC_IRQn);
    NVIC_EnableIRQ (RTC_IRQn);

    /* Enable RTC */
    RTC_Enable (true);

    return 0;
}


static uint32_t RTCDRV_GetSystemTime (void)
{
    uint32_t cnt, ofc;

    // Make sure the overflow counter does not get incremented
    // while reading the RTC counter value
    do {
        ofc = RTCCtrl.overflow_counter;
        cnt = RTC_CounterGet ();
    } while (ofc != RTCCtrl.overflow_counter);

    return (ofc * RTCDRV_SEC_PER_OF) + (cnt / RTCDRV_FREQ);
}


static void RTCDRV_GetSystemNanoTime (SysTime *tm)
{
    uint32_t cnt, ofc;

    // Make sure the overflow counter does not get incremented
    // while reading the RTC counter value
    do {
        ofc = RTCCtrl.overflow_counter;
        cnt = RTC_CounterGet ();
    } while (ofc != RTCCtrl.overflow_counter);

    tm->tv_sec  = (ofc * RTCDRV_SEC_PER_OF) + (cnt / RTCDRV_FREQ);
    tm->tv_nsec = ((cnt % RTCDRV_FREQ) * 125000 / RTCDRV_FREQ) * 8000;
}


static uint32_t RTCDRV_GetFrequency ()
{
    return RTCDRV_FREQ;
}


/***************************************************************************//**
 * @brief RTC trigger enable
 * @param msec Enable trigger in msec
 * @param callback Callback invoked when @p msec elapsed
 ******************************************************************************/
static void RTCDRV_Trigger (SysTime *trigger_time, void (*callback) (void))
{
    uint32_t ticks;

    RTCCtrl.compare_callback = NULL;

    if (trigger_time->tv_sec < RTCDRV_SEC_PER_OF) {
        ticks = (trigger_time->tv_sec * RTCDRV_FREQ) + (((trigger_time->tv_nsec / 8000) * RTCDRV_FREQ) / 125000);
    } else {
        ticks = 0;
    }

    /* Register callback */
    RTCCtrl.compare_callback = callback;

    // Set some safe threshold, 1 tick should be enough...
    if (ticks < RTCDRC_COMP_SET_MIN) {
        ticks = RTCDRC_COMP_SET_MIN;
    }

    /* Set new compare value */
    RTC_CompareSet (0, RTC_CounterGet () + ticks);
}


SysTimeBackend _SysTimeRtc = {
    .init               = RTCDRV_Init,
    .getFrequency       = RTCDRV_GetFrequency,
    .getSystemTime      = RTCDRV_GetSystemTime,
    .getSystemNanoTime  = RTCDRV_GetSystemNanoTime,
    .trigger            = RTCDRV_Trigger
};

SysTimeBackend *SysTimeRtc = &_SysTimeRtc;
