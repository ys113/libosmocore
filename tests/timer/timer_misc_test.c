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
	struct timeval now;
	struct timeval timeout;
	int expect_rc;
	struct timeval expect_diff;
};

struct timer_remaining_testcase timer_remaining_data[] = {
	{
		.now = { .tv_sec = 1000000, .tv_usec = 0 },
		.timeout = { .tv_sec = 1000123, .tv_usec = 1 },
		.expect_rc = 0,
		.expect_diff = { .tv_sec = 123, .tv_usec = 1 },
	},
	{
		.now = { .tv_sec = 1000000, .tv_usec = 0 },
		.timeout = { .tv_sec = 1000000, .tv_usec = 1 },
		.expect_rc = 0,
		.expect_diff = { .tv_sec = 0, .tv_usec = 1 },
	},
	{
		.now = { .tv_sec = 1000000, .tv_usec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_usec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = 0 - 1, .tv_usec = 999999 },
	},
	{
		.now = { .tv_sec = 1000001, .tv_usec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_usec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = -1 - 1, .tv_usec = 999999 },
	},
	{
		.now = { .tv_sec = 1000123, .tv_usec = 1 },
		.timeout = { .tv_sec = 1000000, .tv_usec = 0 },
		.expect_rc = -1,
		.expect_diff = { .tv_sec = -123 - 1, .tv_usec = 999999 },
	},
};

void test_timer_remaining()
{
	int i;
	bool all_ok = true;
	printf("\n--- start: %s\n", __func__);

	for (i = 0; i < ARRAY_SIZE(timer_remaining_data); i++) {
		struct timer_remaining_testcase *tc = &timer_remaining_data[i];
		struct osmo_timer_list t = {
			.timeout = tc->timeout,
			.active = 1,
		};
		struct timeval diff;
		int rc;
		bool ok = true;

		rc = osmo_timer_remaining(&t, &tc->now, &diff);

		printf("timeout:%ld.%06ld - now:%ld.%06ld = diff:%ld.%06ld; rc=%d\n",
		       t.timeout.tv_sec, t.timeout.tv_usec,
		       tc->now.tv_sec, tc->now.tv_usec,
		       diff.tv_sec, diff.tv_usec,
		       rc);

		if (rc != tc->expect_rc) {
			printf("  ERROR: expected rc = %d\n", tc->expect_rc);
			ok = false;
		}

		if (diff.tv_sec != tc->expect_diff.tv_sec || diff.tv_usec != tc->expect_diff.tv_usec) {
			printf("  ERROR: expected diff = %ld.%06ld\n",
			       tc->expect_diff.tv_sec, tc->expect_diff.tv_usec);
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
