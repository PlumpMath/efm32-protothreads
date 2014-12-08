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

#ifndef SYSTIME_H_
#define SYSTIME_H_

#include "features.h"

#include <stdint.h>
#include <time.h>
#include <sys/time.h>


typedef struct {
    uint32_t tv_sec;
    uint32_t tv_nsec;
} SysTime;


#define SYSTIME_SET(tm_dest, tm_src)        \
do {                                        \
    (tm_dest)->tv_sec = (time_t)(tm_src)->tv_sec;   \
    (tm_dest)->tv_nsec = (long)(tm_src)->tv_nsec;   \
} while (0)

#define SYSTIME_RESET(tm)                   \
do {                                        \
    (tm)->tv_sec = 0;                       \
    (tm)->tv_nsec = 0;                      \
} while (0)

#define SYSTIME_ISSET(tm) ((tm)->tv_sec != 0 || (tm)->tv_nsec != 0)

#define SYSTIME_ADD(res, tm1, tm2)          \
do {                                        \
    (res)->tv_sec = (time_t)(tm1)->tv_sec + (time_t)(tm2)->tv_sec;  \
    (res)->tv_nsec = (long)(tm1)->tv_nsec + (long)(tm2)->tv_nsec;   \
    if ((res)->tv_nsec >= 1000000000) {     \
        (res)->tv_sec++;                    \
        (res)->tv_nsec -= 1000000000;       \
    }                                       \
} while (0)

#define SYSTIME_SUB(res, tm1, tm2)          \
do {                                        \
    if ((long)(tm1)->tv_nsec >= (long)(tm2)->tv_nsec) {                             \
        (res)->tv_sec = (time_t)(tm1)->tv_sec - (time_t)(tm2)->tv_sec;              \
        (res)->tv_nsec = (long)(tm1)->tv_nsec - (long)(tm2)->tv_nsec;               \
    } else {                                                                        \
        (res)->tv_sec = (time_t)(tm1)->tv_sec - (time_t)(tm2)->tv_sec - 1;          \
        (res)->tv_nsec = 1000000000L + (long)(tm1)->tv_nsec - (long)(tm2)->tv_nsec; \
    }                                                                               \
} while (0)

#define SYSTIME_CMP(tm1, tm2) ((tm1)->tv_sec == (tm2)->tv_sec ? ((tm1)->tv_nsec ==(tm2)->tv_nsec ? 0 : ((tm1)->tv_nsec > (tm2)->tv_nsec ? 1 : -1)) : ((tm1)->tv_sec > (tm2)->tv_sec ? 1 : -1))
#define SYSTIME_CMP_L(tm1, tm2) ((tm1).tv_sec == (tm2).tv_sec ? ((tm1).tv_nsec ==(tm2).tv_nsec ? 0 : ((tm1).tv_nsec > (tm2).tv_nsec ? 1 : -1)) : ((tm1).tv_sec > (tm2).tv_sec ? 1 : -1))

typedef struct {
    int         (*init) (void);
    uint32_t    (*getFrequency) (void);
    uint32_t    (*getSystemTime) (void);
    void        (*getSystemNanoTime) (SysTime *tm);
    void        (*trigger) (SysTime *tm, void (*callback) (void));
} SysTimeBackend;


int SYSTIME_Init (SysTimeBackend *backend);
void SYSTIME_Trigger (struct timespec *trigger_time, void (*callback) (void));

#endif /* SYSTIME_H_ */
