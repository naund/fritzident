/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * main.c
 * Copyright (C) 2013 Andre Larbiere <andre@larbiere.eu>
 * Copyright (C) 2015 Nils Naumann <nau@gmx.net>
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
#include <arpa/inet.h>
//#include <resolv.h>
#include <errno.h>
#include <systemd/sd-daemon.h>


#include "netinfo.h"
#include "userinfo.h"
#include "debug.h"

#define SOCKET 14013 /* Fritzident Socket */

void SocketServer();
void usage(const char *cmdname);

int main(int argc, char *argv[])
{
    int c;

    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"verbose",	no_argument, NULL, 'v'},
            {"domain",   required_argument, NULL, 'd'},
            {"help",	no_argument, NULL, '?'},
            {0,		0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "vd:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'v':  /* FIXME implementieren */
            break;
        case 'd':
            set_default_domain(optarg);
            break;
        case '?':
            usage(argv[0]);
            return 0;
        default:
            fprintf(stderr, "Unknown option\n");
            fprintf(stderr, "Usage: fritzident [-v] [-d domain]\n");
            return 1;
        }
    }

    add_uid_range (1000, 65533);
    add_uid_range (65537,(uid_t) -1);

    SocketServer();
    return 0;
}

void sendResponse(int client_fd, const char *response, ...)
{
    char msg[256];
    va_list args;
    va_start(args, response);
    int n = vsnprintf(msg, sizeof(msg), response, args);
    va_end(args);

    ssize_t nSent = send(client_fd, msg, n+1, 0);
    if(nSent < n+1) debugLog("Sending Response failed: %s", msg);
}

void execUSERS(int client_fd)
{
    struct passwd *userinfo = getpwent();
    while (userinfo) {
        if (included_uid(userinfo->pw_uid)) {
	    debugLog("USERS: %s\n", add_default_domain(userinfo->pw_name));
            sendResponse(client_fd, "%s\r\n", add_default_domain(userinfo->pw_name));
        }
        userinfo = getpwent();
    }
    endpwent();
}

void execTCP(int client_fd, const char *ipv4, const char *port)
{
    unsigned int portNumber;
    uid_t uid;
    sscanf(port, "%u", &portNumber);
    uid = ipv4_tcp_port_uid(ipv4, portNumber);
    if (uid != UID_NOT_FOUND) {
        if (included_uid(uid)) {
            struct passwd *user;
            user = getpwuid(uid);
            debugLog("TCP %s: %s\n", port, user->pw_name);
            sendResponse(client_fd, "USER %s\r\n", add_default_domain(user->pw_name));
        }
        else
            sendResponse(client_fd, "ERROR SYSTEM_USER\r\n");
    }
    else {
        sendResponse(client_fd, "ERROR NOT_FOUND\r\n");
    }
}

void execUDP(int client_fd, const char *ipv4, const char *port)
{
    unsigned int portNumber;
    uid_t uid;
    sscanf(port, "%u", &portNumber);
    uid = ipv4_udp_port_uid(ipv4, portNumber);
    if (uid != UID_NOT_FOUND) {
        if (included_uid(uid)) {
            struct passwd *user;
            user = getpwuid(uid);
            debugLog("UDP %s: %s\n", port, user->pw_name);
            sendResponse(client_fd, "USER %s\r\n", add_default_domain(user->pw_name));
        }
        else
            sendResponse(client_fd, "ERROR SYSTEM_USER\r\n");
    }
    else {
        sendResponse(client_fd, "ERROR NOT_FOUND\r\n");
    }
}


void SocketServer()
{
    int socket_fd;
    struct sockaddr_in self;
    char cmd[256];
    int n;

    n = sd_listen_fds(0); /* number of file descriptors passed by systemd */
      
    /* debugLog("Got %i file descriptors from systemd", n); */
    if (n > 1) {
	    debugLog("Too many file descriptors received.\n");
	    exit(1);
    } else if(n == 1){
	    /* debugLog("Socket passed by systemd"); */
	    socket_fd = SD_LISTEN_FDS_START + 0;
    }
    else {
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
	    debugLog(strerror(errno));
	    exit(errno);
	}

	bzero(&self, sizeof(self));

	self.sin_family = AF_INET;
	self.sin_port = htons(SOCKET);
	self.sin_addr.s_addr = INADDR_ANY;

	/* Bind port to socket */
	if(bind(socket_fd, (struct sockaddr*) &self, sizeof(self)) != 0 )
	{
	    debugLog(strerror(errno));
	    exit(errno);
	}

	/* Make it a "listening socket" */
	if (listen(socket_fd, 20) != 0)
	{
	    debugLog(strerror(errno));
	    exit(errno);
	}
    }

    /* Infinite loop */
    while (1)
    {
        int client_fd;
        ssize_t nSent;
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);

        /* Await a connection on socket_fd*/
        client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addrlen);
        /*debugLog("%s:%d connected\n", inet_ntoa(client_addr.sin_addr),
		 ntohs(client_addr.sin_port)); */

        nSent = send(client_fd, "AVM IDENT\r\n", sizeof("AVM IDENT\r\n"), 0);
        if(nSent < 0) {
            debugLog("Sending \"AVM IDENT\" failed!");
            exit(-1);
        }

        /* wait for data */
        recv(client_fd, cmd, 256, 0);

        /* the first line is containing the command which might be
         * USERS, TCP, or UDP */
        char *cmdVerb = strtok(cmd, "\r\n ");

        /* debugLog("Received command: \"%s\"\n", cmdVerb); */
        if (strcmp(cmdVerb, "USERS") == 0) {
            execUSERS(client_fd);
        }
        else if (strcmp(cmdVerb, "TCP") == 0) {
            char *localIp = strtok(NULL, ":");
            char *localPort = strtok(NULL, "\r\n");
            /* debugLog("Searching for \"%s:%s\"\n", localIp, localPort); */
            execTCP(client_fd, localIp, localPort);
        }
        else if (strcmp(cmdVerb, "UDP") == 0) {
            char *localIp = strtok(NULL, ":");
            char *localPort = strtok(NULL, "\r\n");
            /* debugLog("Searching for \"%s:%s\"\n", localIp, localPort); */
            execUDP(client_fd, localIp, localPort);
        }
        else {
            debugLog("Unrecognized command \"%s\"\r\n", cmdVerb);
            sendResponse(client_fd, "ERROR UNSPECIFIED\r\n");
        }

        /* Close data connection */
        close(client_fd);
    }

    /* Finally some housekeeping */
    close(socket_fd);
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
