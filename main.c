/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2013 Andre Larbiere <andre@larbiere.eu>
 * 
 * fritzident is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * fritzident is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <pwd.h>
#include <sys/socket.h>

#include "netinfo.h"
#include "userinfo.h"
#include "debug.h"

void handleCmd();
void usage(const char *cmdname);

int main(int argc, char *argv[])
{
	int c;

	while (1) {
	   int option_index = 0;
	   static struct option long_options[] = {
		   {"log",		required_argument, NULL, 'l'},
		   {"domain",   required_argument, NULL, 'd'},
		   {"help",   required_argument, NULL, '?'},
		   {0,         0,                 0,  0 }
	   };

       c = getopt_long(argc, argv, "l:d:",
                long_options, &option_index);
       if (c == -1)
           break;

       switch (c) {
		   case 'l':
			   debugLogFile(optarg);
	           break;
		   case 'd':
			   set_default_domain(optarg);
			   break;
		   case '?':
			   usage(argv[0]);
			   return 0;
		   default:
			   fprintf(stderr, "Unknown option\n");
			   fprintf(stderr, "Usage: avmident [-l logfile] [-d domain]\n");
			   return 1;
	   }
	}

	add_uid_range (1000, 65533);
	add_uid_range (65537,(uid_t) -1);
	handleCmd();
	return 0;
}

void sendResponse(const char *response, ...)
{
	va_list args;
	va_start(args, response);
	vfprintf(stdout, response, args);
	fflush(stdout);
	va_end(args);
}

void execUSERS()
{
	struct passwd *userinfo = getpwent();
	while (userinfo) {
	    if (included_uid(userinfo->pw_uid))
            sendResponse("%s\r\n", add_default_domain(userinfo->pw_name));
	    userinfo = getpwent();
	}
	endpwent();
}

void execTCP(const char *ipv4, const char *port)
{
	unsigned int portNumber;
	uid_t uid;
	sscanf(port, "%u", &portNumber);
	uid = ipv4_tcp_port_uid(ipv4, portNumber);
	if (uid != UID_NOT_FOUND) {
		if (included_uid(uid)) {
			struct passwd *user;
			user = getpwuid(uid);
			debugLog("USER %s\n", user->pw_name);
			sendResponse("USER %s\r\n", add_default_domain(user->pw_name));
		}
		else
			sendResponse("ERROR SYSTEM_USER\r\n");
	}
	else {
		sendResponse("ERROR NOT_FOUND\r\n");
	}
}

void execUDP(const char *ipv4, const char *port)
{
	unsigned int portNumber;
	uid_t uid;
	sscanf(port, "%u", &portNumber);
	uid = ipv4_udp_port_uid(ipv4, portNumber);
	if (uid != UID_NOT_FOUND) {
		if (included_uid(uid)) {
			struct passwd *user;
			user = getpwuid(uid);
			debugLog("USER %s\n", user->pw_name);
			sendResponse("USER %s\r\n", add_default_domain(user->pw_name));
		}
		else
			sendResponse("ERROR SYSTEM_USER\r\n");
	}
	else {
		sendResponse("ERROR NOT_FOUND\r\n");
	}
}

void handleCmd()
{
	char cmd[256];


	debugLog("Sending prompt\n");
	sendResponse("AVM IDENT\r\n");
	while (fgets(cmd, 256, stdin)) {
		char *cmdVerb = strtok(cmd, "\r\n ");
		debugLog("Received command \"%s\"\n", cmdVerb);
		if (strcmp(cmdVerb, "USERS") == 0) {
			execUSERS();
		}
		else if (strcmp(cmdVerb, "TCP") == 0) {
			char *localIp = strtok(NULL, ":");
			char *localPort = strtok(NULL, "\r\n");
			debugLog("Searching for \"%s:%s\"\n", localIp, localPort);
			execTCP(localIp, localPort);
		}
		else if (strcmp(cmdVerb, "UDP") == 0) {
			char *localIp = strtok(NULL, ":");
			char *localPort = strtok(NULL, "\r\n");
			debugLog("Searching for \"%s:%s\"\n", localIp, localPort);
			execUDP(localIp, localPort);
		}
		else {
			debugLog("Unrecognized command \"%s\"\r\n", cmdVerb);
			sendResponse("ERROR UNSPECIFIED\r\n");
		}
	}
}

void usage(const char *cmdname)
{
	printf("Usage: %s [-l logfile] [-d domain]\n\n", cmdname);
	printf("Answer Fritz!Box user identification requests.\n\n");
	printf("%s mimics the standard AVM Windows application that allows the Fritz!Box to recognize individual users connecting to the Internet.\n", cmdname);
	printf("Options:\n");
	printf("\t-l logfile ..... write debug messages to the specified log file\n");
	printf("\t-d domain ...... fake a Windows domain\n");
	printf("\nLICENSE:\n");
	printf("This utility is provided under the GNU GENERAL PUBLIC LICENSE v3.0\n(see http://www.gnu.org/licenses/gpl-3.0.txt)\n");
}
