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

#include <platform.h>

#include <em_cmu.h>

#include <protothreads.h>
#include <systimer.h>


PROCESS (MAIN_Process, "Main Process");
PROCESS_THREAD (MAIN_Process, ev, data)
{
    PROCESS_BEGIN ();

    CMU_ClockEnable (cmuClock_HFPER, true);
    CMU_ClockEnable (cmuClock_GPIO, true);

    PROCESS_END ();
}
