/*! \file osmo-gsmtap-sink.c
 * Receive GSMTAP without causing ICMP reject messages */
/*
 * (C) 2018 by sysmocom - s.f.m.c. GmbH <info@sysmocom.de>
 *
 * All Rights Reserved
 *
 * Author: Neels Hofmeyr <nhofmeyr@sysmocom.de>
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdbool.h>

#include <osmocom/core/socket.h>
#include <osmocom/core/gsmtap.h>
#include <osmocom/core/select.h>

static void help(const char *progname)
{
	printf("Usage: %s [-hv] [<local-ip> [<local-port>]]\n"
	       "\n"
	       "Bind to UDP local-ip:local-port, read and discard everything received.\n"
	       "Default is to listen on 'any' interface at the GSMTAP port %u\n"
	       "(hence invoking this is a no-brainer compared to e.g. a netcat).\n"
	       "\n"
	       "Rationale: sending GSMTAP has the sole purpose of being picked up by a\n"
	       "network trace like tcpdump. However, sending GSMTAP to an unbound port\n"
	       "will trigger ICMP packets to indicate rejection, and this may even\n"
	       "cause GSMTAP packets to go missing from the trace. Simply launch this\n"
	       "program, a black hole to receive GSMTAP, to resolve these problems.\n"
	       "\n"
	       "  -h  show help\n"
	       "  -v  verbose: print a dot for each received packet\n",
	       progname, GSMTAP_UDP_PORT);
}

bool verbose = false;

int receive_cb(struct osmo_fd *ofd, unsigned int what)
{
	static uint8_t buf[4096];

	if (!(what & BSC_FD_READ))
		return 0;

	recv(ofd->fd, buf, sizeof(buf), 0);
	if (verbose) {
		printf(".");
		fflush(stdout);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int opt;
	char *local_ip = NULL;
	uint16_t local_port = GSMTAP_UDP_PORT;
	struct osmo_fd ofd;

	while ((opt = getopt(argc, argv, "hv")) != -1) {
		switch (opt) {
		case 'v':
			verbose = true;
			break;
		case 'h':
			help(argv[0]);
			exit(0);
			break;
		default:
			break;
		}
	}

	if (argc - optind > 0) {
		local_ip = argv[optind++];
	}

	if (argc - optind > 0) {
		local_port = atoi(argv[optind++]);
	}

	if (argc - optind > 0) {
		help(argv[0]);
		exit(0);
	}

	ofd.cb = receive_cb;
	osmo_sock_init_ofd(&ofd, AF_UNSPEC, SOCK_DGRAM, IPPROTO_UDP,
			   local_ip, local_port, OSMO_SOCK_F_BIND);

	while (1) {
		int rc;
		rc = osmo_select_main(0);
		if (rc < 0)
			exit(1);
	}
	exit(0);
}
