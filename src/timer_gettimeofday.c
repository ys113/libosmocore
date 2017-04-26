/*
 * (C) 2016 by sysmocom s.f.m.c. GmbH <info@sysmocom.de>
 * All Rights Reserved
 *
 * Authors: Neels Hofmeyr <nhofmeyr@sysmocom.de>
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

/*! \file timer_gettimeofday.c
 */

#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>


bool osmo_gettimeofday_override = false;

struct timeval osmo_gettimeofday_override_time = { 23, 424242 };

static unsigned int osmo_gettimeofday_factor = 0;

/*! \brief shim around gettimeofday to be able to set the time manually.
 * To override, set osmo_gettimeofday_override == true and set the desired
 * current time in osmo_gettimeofday_override_time. */
int osmo_gettimeofday(struct timeval *tv, struct timezone *tz)
{
	struct timeval now, diff1, diff2;

	if (!osmo_gettimeofday_override)
		return gettimeofday(tv, tz);

	if (!osmo_gettimeofday_factor) {
		*tv = osmo_gettimeofday_override_time;
	} else {
		gettimeofday(&now, NULL);
		timersub(&now, &osmo_gettimeofday_override_time, &diff1);
		diff2.tv_usec = (diff1.tv_usec * osmo_gettimeofday_factor) % 1000000;
		diff2.tv_sec = diff1.tv_sec * osmo_gettimeofday_factor +
				(diff1.tv_usec * osmo_gettimeofday_factor) / 1000000;
		timeradd(&osmo_gettimeofday_override_time, &diff2, tv);
	}

	return 0;
}

/*! \brief convenience function to advance the fake time.
 * Add the given values to osmo_gettimeofday_override_time. */
void osmo_gettimeofday_override_add(time_t secs, suseconds_t usecs)
{
	struct timeval val = { secs, usecs };
	timeradd(&osmo_gettimeofday_override_time, &val,
		 &osmo_gettimeofday_override_time);
}

/*! \brief convenience function to make time advance faster, at a x<factor> rate.
 * Setting factor to 0 disables the feature. */
void osmo_gettimeofday_override_accel_factor(unsigned int factor)
{
	gettimeofday(&osmo_gettimeofday_override_time, NULL);
	osmo_gettimeofday_factor = factor;
}

/*! @} */
