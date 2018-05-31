/*
 * (C) 2018 by sysmocom - s.f.m.c. GmbH <info@sysmocom.de>
 * All Rights Reserved
 *
 * Authors: Neels Hofmeyr <neels@hofmeyr.de>
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

#include <stdio.h>
#include <osmocom/core/utils.h>
#include <osmocom/core/timer.h>

struct timer_remaining_testcase {
	struct timespec now;
	struct timespec timeout;
	int expect_rc;
	struct timespec expect_diff;
};

struct timer_remaining_testcase timer_remaining_data[] = {
	{
		.now = { .tv_sec = 1000000, .tv_nsec = 0 },
		.timeout = { .tv_sec = 1000123, .tv_nsec = 1 },
		.expect_rc = 0,
		.expect_diff = { .tv_sec = 123, .tv_nsec = 1 },
	},
	{
		.now = { .tv_sec = 1000000, .tv_nsec = 0 },
		.timeout = { .tv_sec = 1000000, .tv_nsec = 1 },
		.expect_rc = 0,
		.expect_diff = { .tv_sec = 0, .tv_nsec = 1 },
	},
	{
		.now = { .tv_sec = 1000000, .tv_nsec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_nsec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = 0 - 1, .tv_nsec = 999999999 },
	},
	{
		.now = { .tv_sec = 1000001, .tv_nsec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_nsec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = -1 - 1, .tv_nsec = 999999999 },
	},
	{
		.now = { .tv_sec = 1000123, .tv_nsec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_nsec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = -123 - 1, .tv_nsec = 999999999 },
	},
};

void test_timer_remaining()
{
	int i;
	bool all_ok = true;
	struct timespec *now;
	printf("\n--- start: %s\n", __func__);
	osmo_clock_override_enable(CLOCK_MONOTONIC, true);
	now = osmo_clock_override_gettimespec(CLOCK_MONOTONIC);

	for (i = 0; i < ARRAY_SIZE(timer_remaining_data); i++) {
		struct timer_remaining_testcase *tc = &timer_remaining_data[i];
		struct osmo_timer_list t = {
			.timeout_at = tc->timeout,
			.active = 1,
		};
		struct timespec diff;
		int rc;
		bool ok = true;

		*now = tc->now;
		rc = osmo_timer_remaining2(&t, &diff);

		printf("timeout:%ld.%06ld - now:%ld.%06ld = diff:%ld.%06ld; rc=%d\n",
		       t.timeout_at.tv_sec, t.timeout_at.tv_nsec,
		       tc->now.tv_sec, tc->now.tv_nsec,
		       diff.tv_sec, diff.tv_nsec,
		       rc);

		if (rc != tc->expect_rc) {
			printf("  ERROR: expected rc = %d\n", tc->expect_rc);
			ok = false;
		}

		if (diff.tv_sec != tc->expect_diff.tv_sec || diff.tv_nsec != tc->expect_diff.tv_nsec) {
			printf("  ERROR: expected diff = %ld.%06ld\n",
			       tc->expect_diff.tv_sec, tc->expect_diff.tv_nsec);
			ok = false;
		}

		if (!ok)
			all_ok = false;
	}

	if (!all_ok) {
		printf("--- FAILURE: %s\n", __func__);
		exit(EXIT_FAILURE);
	}

	printf("--- done: %s\n", __func__);
}

int main(int argc, char *argv[])
{
	test_timer_remaining();
}
