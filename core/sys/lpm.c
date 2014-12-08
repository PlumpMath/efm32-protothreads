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

#include "em_emu.h"
#include "em_int.h"

#include "lpm.h"

static volatile int EventRegistered = 0;


void LPM_RegisterEvent (void)
{
  EventRegistered = 1;
}


void LPM_WaitForEvent (void)
{
    while (!EventRegistered) {
        INT_Disable ();
        EMU_EnterEM2 (true);
        INT_Enable ();
    }

    EventRegistered = 0;
}
