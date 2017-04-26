/*
 * (C) 2016 by sysmocom s.f.m.c. GmbH <info@sysmocom.de>
 * All Rights Reserved
 *
 * Authors: Pau Espin Pedrol <pespin@sysmocom.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*! \addtogroup timer
 *  @{
 */

/*! \file timer_clockgettime.c
 */

#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include <osmocom/core/timer_compat.h>


struct fakeclock {
	bool override;
	struct timespec time;
	unsigned int factor;
};

static struct fakeclock mono;

static struct fakeclock* clkid_to_fakeclock(clockid_t clk_id)
{
	switch(clk_id) {
	case CLOCK_MONOTONIC:
		return &mono;
	default:
		return NULL;
	}
}

/*! \brief shim around clock_gettime to be able to set the time manually.
 * To override, set clock_gettime == true and set the desired
 * current time in clock_gettime. */
int osmo_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	struct timespec now, diff1, diff2;
	struct fakeclock* c = clkid_to_fakeclock(clk_id);
	if (!c || !c->override)
		return clock_gettime(clk_id, tp);

	if (!c->factor) {
		*tp = c->time;
	} else {
		clock_gettime(clk_id, &now);
		timespecsub(&now, &c->time, &diff1);
		diff2.tv_nsec = (diff1.tv_nsec * c->factor) % 1000000000;
		diff2.tv_sec = diff1.tv_sec * c->factor +
				(diff1.tv_nsec * c->factor) / 1000000000;
		timespecadd(&c->time, &diff2, tp);
	}
	return 0;
}

/*! \brief convenience function to enable or disable a specific clock fake time.
 */
void osmo_clock_override_enable(clockid_t clk_id, bool enable)
{
	struct fakeclock* c = clkid_to_fakeclock(clk_id);
	if (c)
		c->override = enable;
}

/*! \brief convenience function to return a pointer to the timespec handling the
 * fake time for clock clk_id. */
struct timespec *osmo_clock_gettimespec(clockid_t clk_id)
{
	struct fakeclock* c = clkid_to_fakeclock(clk_id);
	if (c)
		return &c->time;
	return NULL;
}

/*! \brief convenience function to advance the fake time.
 * Add the given values to the clock time. */
void osmo_clock_override_add(clockid_t clk_id, time_t secs, suseconds_t usecs)
{
	struct timespec val = { secs, usecs*1000 };
	struct fakeclock* c = clkid_to_fakeclock(clk_id);
	if (c)
		timespecadd(&c->time, &val, &c->time);
}

/*! \brief convenience function to make time advance faster, at a x<factor> rate.
 * Setting factor to 0 disables the feature. */
void osmo_clock_override_accel_factor(clockid_t clk_id, unsigned int factor)
{
	struct fakeclock* c = clkid_to_fakeclock(clk_id);
	if (c) {
		clock_gettime(clk_id, &c->time);
		c->factor = factor;
	}
}

/*! @} */
