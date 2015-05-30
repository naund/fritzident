/*
 * netinfo.c
 *
 * Copyright (C) 2013 - Andre Larbiere <andre@larbiere.eu>
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
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "netinfo.h"

#include "debug.h"

#define IPV4_TCP_PORTS  "/proc/net/tcp"
#define IPV4_UDP_PORTS  "/proc/net/udp"
#define IPV6_TCP_PORTS  "/proc/net/tcp6"
#define IPV6_UDP_PORTS  "/proc/net/udp6"

// convert an IPv4 address & port number to a hex string in the format
//  B0B1B2B3:PORT
// with Bx representing the IP address in host byte order
// the string is returned in a static buffer, which is overwritten at every call
const char *ipv4_bindstring(const char *ipv4, unsigned int port)
{
	static char buffer[32];
	union {
		uint8_t b[4];
		uint32_t bin;
	} ip_s;

	sscanf(ipv4, "%hhu.%hhu.%hhu.%hhu",
	       &(ip_s.b[0]), &(ip_s.b[1]), &(ip_s.b[2]), &(ip_s.b[3]));

	sprintf(buffer, "%08X:%04X", ip_s.bin, port);

	debugLog(LOG_DEBUG, "Bindstring \"%s\"\n", buffer);
	return buffer;
}

		
// find the UID associated with a specific local ipv4 TCP port
uid_t ipv4_tcp_port_uid(const char *ipv4, unsigned int port)
{
	char buffer[1024];
	const char *bindstring = ipv4_bindstring(ipv4, port);

	FILE *portlist=fopen(IPV4_TCP_PORTS, "r");
	while (fgets(buffer, 1024, portlist)) {
	    char *uid;
	    char *field=strtok(buffer, " ");        // INDEX (ignored)
	    char *local=field=strtok(NULL, " ");    // LOCAL ADDRESS
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    uid=strtok(NULL, " ");                  // UID
	    if (strcmp(local, bindstring)==0) {
            unsigned long id;
            sscanf(uid, "%lu", &id);
	    debugLog(LOG_DEBUG, "Found UID=%lu\n", id);
	    fclose(portlist);
            return (uid_t)id;
	    }
	}
	fclose(portlist);
	debugLog(LOG_NOTICE, "UID fpr UDP Port %i Not found\n", port);
	return UID_NOT_FOUND;
}

// find the UID associated with a specific local ipv4 UDP port
uid_t ipv4_udp_port_uid(const char *ipv4, unsigned int port)
{
	char buffer[1024];
	const char *bindstring = ipv4_bindstring(ipv4, port);

	FILE *portlist=fopen(IPV4_UDP_PORTS, "r");
	while (fgets(buffer, 1024, portlist)) {
	    char *uid;
	    char *field=strtok(buffer, " ");        // INDEX (ignored)
	    char *local=field=strtok(NULL, " ");    // LOCAL ADDRESS
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    field=strtok(NULL, " ");                // (ignored)
	    uid=strtok(NULL, " ");                  // UID
	    if (strcmp(local, bindstring)==0) {
            unsigned long id;
            sscanf(uid, "%lu", &id);
	    debugLog(LOG_DEBUG, "Found UID=%lu\n", id);
	    fclose(portlist);
            return (uid_t)id;
	    }
	}
	fclose(portlist);
	debugLog(LOG_NOTICE, "UID for TCP port %i Not found\n", port);
	return UID_NOT_FOUND;
}

	