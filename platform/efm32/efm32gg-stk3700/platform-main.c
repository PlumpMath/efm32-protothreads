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

#include "platform-conf.h"

#include <em_device.h>
#include <em_chip.h>
#include <em_cmu.h>

#include <lpm.h>
#include <protothreads.h>
#include <systime.h>
#include <systimer.h>

int main (void)
{
    // Chip errata
    CHIP_Init ();

    // Set up clocks
    CMU_ClockSelectSet (cmuClock_HF, cmuSelect_HFRCO);
    CMU_ClockSelectSet (cmuClock_LFA, cmuSelect_LFXO);

    // Initialize the system time
    SYSTIME_Init (SysTimeRtc);

    // Initialize protothreads...
    process_init ();

    // Activate the system timer
    process_start (&SYSTIMER_Process, NULL);

    process_start (&MAIN_Process, NULL);

    while (1) {

        // Run processes...
        while (process_run () > 0);

        // Sleep until the next event...
        LPM_WaitForEvent ();

    }

    return 0;
}

