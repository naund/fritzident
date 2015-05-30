/*
 * netinfo.h
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
#include <pwd.h>
#define UID_SYSTEM	  0	/* returned for ports that are owned by a system user */
#define UID_NOT_FOUND ((uid_t)-1)   /* returned if port is not found */

uid_t ipv4_tcp_port_uid(const char *ipv4, unsigned int port);
uid_t ipv4_udp_port_uid(const char *ipv4, unsigned int port);
