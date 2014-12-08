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

#include <errno.h>
#include <string.h>

#include "systime.h"

typedef struct {
    SysTimeBackend  *backend;
    struct timespec offset;
} SysTimeControl;

static SysTimeControl SysTimeCtrl;


int SYSTIME_Init (SysTimeBackend *backend)
{
    SysTimeCtrl.offset.tv_sec = 0;
    SysTimeCtrl.offset.tv_nsec = 0;
    SysTimeCtrl.backend = backend;

    return SysTimeCtrl.backend->init ();
}


void SYSTIME_Trigger (struct timespec *trigger_time, void (*callback) (void))
{
    struct timespec current_time;

    clock_gettime (CLOCK_REALTIME, &current_time);

    if (SYSTIME_CMP (trigger_time, &current_time) > 0) {

        uint32_t d_sec, d_nsec;

        d_sec = trigger_time->tv_sec - current_time.tv_sec;

        if (trigger_time->tv_nsec >= current_time.tv_nsec) {
            d_nsec = trigger_time->tv_nsec - current_time.tv_nsec;
        } else {
            d_sec--;
            d_nsec = 1000000000 + trigger_time->tv_nsec - current_time.tv_nsec;
        }

        SysTime tm = {
            .tv_sec = d_sec,
            .tv_nsec = d_nsec
        };

        SysTimeCtrl.backend->trigger (&tm, callback);

    } else {

        // Requested time has already passed, trigger callback immediately
        callback ();

    }
}


#if defined(_POSIX_TIMERS)

int clock_settime (clockid_t clock_id, const struct timespec *tp)
{
    SysTime tm;

    if (tp != NULL) {

        if (clock_id == CLOCK_REALTIME) {

            if (tp->tv_nsec >= 1000000000) {
                errno = EINVAL;
                return -1;
            }

            SysTimeCtrl.backend->getSystemNanoTime (&tm);
            SYSTIME_SUB (&SysTimeCtrl.offset, tp, &tm);

            return 0;

        } else {
            errno = EINVAL;
            return -1;
        }

    } else {
        errno = EFAULT;
        return -1;
    }
}


int clock_gettime (clockid_t clock_id, struct timespec *tp)
{
    SysTime tm;

    if (tp != NULL) {
        SysTimeCtrl.backend->getSystemNanoTime (&tm);
        switch (clock_id) {
            case CLOCK_REALTIME:
                SYSTIME_ADD (tp, &tm, &SysTimeCtrl.offset);
                return 0;
#ifdef _POSIX_MONOTONIC_CLOCK
            case CLOCK_MONOTONIC:
                SYSTIME_SET (tp, &tm);
                return 0;
#endif
            default:
                errno = EINVAL;
                return -1;
        }
    } else {
        errno = EFAULT;
        return -1;
    }
}


int clock_getres (clockid_t clock_id, struct timespec *res)
{
    uint32_t freq = SysTimeCtrl.backend->getFrequency ();

    if (res != NULL) {
        switch (clock_id) {
            case CLOCK_REALTIME:
#ifdef _POSIX_MONOTONIC_CLOCK
            case CLOCK_MONOTONIC:
#endif
                res->tv_sec = 0;
                res->tv_nsec = 1000000000 / freq;
                if (res->tv_nsec >= 1000000000) {
                    res->tv_sec++;
                    res->tv_nsec -= 1000000000;
                }
                return 0;
            default:
                errno = EINVAL;
                return -1;
        }
    } else {
        errno = EFAULT;
        return -1;
    }
}

#endif /* _POSIX_TIMERS */


int settimeofday (const struct timeval *tv, const struct timezone *tz)
{
#if defined(_POSIX_TIMERS)
    struct timespec tp = {
        .tv_sec = tv->tv_sec,
        .tv_nsec = tv->tv_usec * 1000
    };

    return clock_settime (CLOCK_REALTIME, &tp);
#else
    SysTime tm;

    if (tv != NULL) {

        if (tv->tv_usec >= 1000000) {
            errno = EINVAL;
            return -1;
        }

        SysTimeCtrl.backend->getSystemNanoTime (&tm);

        struct timespec tp = {
            .tv_sec = tv->tv_sec,
            .tv_nsec = tv->tv_usec * 1000
        };

        if (tp.tv_nsec >= tm.tv_nsec) {
            SysTimeCtrl.offset.tv_sec = tp.tv_sec - tm.tv_sec;
            SysTimeCtrl.offset.tv_nsec = tp.tv_nsec - tm.tv_nsec;
        } else {
            SysTimeCtrl.offset.tv_sec = tp.tv_sec - tm.tv_sec - 1;
            SysTimeCtrl.offset.tv_nsec = 1000000000 + tp.tv_nsec - tm.tv_nsec;
        }

        return 0;

    } else {
        errno = EFAULT;
        return -1;
    }
#endif
}


int gettimeofday (struct timeval *tv, void *tz)
{
#if defined(_POSIX_TIMERS)
    int res;
    struct timespec tp;

    res = clock_gettime (CLOCK_REALTIME, &tp);

    if (res == 0) {
        tv->tv_sec = tp.tv_sec;
        tv->tv_usec = tp.tv_nsec / 1000;
        return 0;
    } else {
        return res;
    }
#else
    SysTime tm;

    if (tv != NULL) {
        SysTimeCtrl.backend->getSystemNanoTime (&tm);
        tv->tv_sec = tm.tv_sec + SysTimeCtrl.offset.tv_sec;
        tv->tv_usec = (tm.tv_nsec + SysTimeCtrl.offset.tv_nsec) / 1000;
        if (tv->tv_usec >= 1000000) {
            tv->tv_sec++;
            tv->tv_usec -= 1000000;
        }
    } else {
        errno = EFAULT;
        return -1;
    }

    return 0;

#endif
}


time_t time (time_t *timer)
{
  time_t t = (SysTimeCtrl.backend->getSystemTime () + SysTimeCtrl.offset.tv_sec);

  /* Copy system time to timer if not NULL*/
  if (timer != NULL) {
      *timer = t;
  }

  return t;
}
