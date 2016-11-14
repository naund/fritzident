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
#include <unistd.h> 
#include <pwd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <systemd/sd-daemon.h>


#include "netinfo.h"
#include "userinfo.h"
#include "debug.h"

#define PORT 14013 /* Fritzident port */ 
#define BUFFER 256

void SocketServer();
void usage(const char *cmdname);

int main(int argc, char *argv[])
{
    int c;
    int Port = PORT;  /* initializing port with default fritzident port */
   
   initLogging();
   
    while (1) {
        int option_index = 0;
        static struct option long_options[] = {
            {"verbose",	no_argument, NULL, 'v'},
            {"domain",   required_argument, NULL, 'd'},
            {"port",   required_argument, NULL, 'p'},
            {"help",	no_argument, NULL, '?'},
            {0,		0,                 0,  0 }
        };

        c = getopt_long(argc, argv, "vd:p:",
                        long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
        case 'v':
	  raiseVerbosity();
	  break;
        case 'd':
            set_default_domain(optarg);
            break;
	case 'p':
	    Port = atoi(optarg);
	    break;
        case '?':
            usage(argv[0]);
            return 0;
        default:
            fprintf(stderr, "Unknown option\n");
            fprintf(stderr, "Usage: fritzident [-v] [-p Port] [-d domain]\n");
            return 1;
        }
    }

    add_uid_range (1000, 65533);
    add_uid_range (65537,(uid_t) -1);

    SocketServer(Port);
  
    return 0;
}

void sendResponse(int client_fd, const char *response, ...)
{
    char msg[BUFFER];
    int n;
    ssize_t nSent;
    
    va_list args;
    va_start(args, response);
    n = vsnprintf(msg, sizeof(msg), response, args);
    va_end(args);

    nSent = send(client_fd, msg, n, 0);
    if(nSent < n){
      debugLog(LOG_ERR, "Sending Response failed: %s", msg);
      debugLog(LOG_ERR, "send: %s\n", strerror(errno));
      exit(errno);
    }
}

void execUSERS(int client_fd)
{
    struct passwd *userinfo = getpwent();
    while (userinfo) {
        if (included_uid(userinfo->pw_uid)) {
	    debugLog(LOG_DEBUG, "USERS: %s\n", add_default_domain(userinfo->pw_name));
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
            debugLog(LOG_DEBUG, "TCP %s: %s\n", port, user->pw_name);
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
            debugLog(LOG_DEBUG, "UDP %s: %s\n", port, user->pw_name);
            sendResponse(client_fd, "USER %s\r\n", add_default_domain(user->pw_name));
        }
        else
            sendResponse(client_fd, "ERROR SYSTEM_USER\r\n");
    }
    else {
        sendResponse(client_fd, "ERROR NOT_FOUND\r\n");
    }
}


void SocketServer(int Port)
{
    int socket_fd;
    int client_fd;
    struct sockaddr_in self;
    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    ssize_t bytes;
    char cmd[BUFFER];
    int n;

    n = sd_listen_fds(0); /* number of file descriptors passed by systemd */

    /* debugLog("Got %i file descriptors from systemd", n); */
    if (n > 1) {
	    debugLog(LOG_ERR, "Too many file descriptors received.\n");
	    exit(1);
    } else if(n == 1){
	    debugLog(LOG_DEBUG, "Socket passed by systemd");
	    socket_fd = SD_LISTEN_FDS_START + 0;
    }
    else {
      debugLog(LOG_INFO, "Creating socket\n");
      if((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	debugLog(LOG_ERR, "socket: %s\n", strerror(errno));
	exit(errno);
      }

      bzero(&self, sizeof(self));
      self.sin_family = AF_INET;
      self.sin_port = htons(Port);
      self.sin_addr.s_addr = INADDR_ANY;

      debugLog(LOG_INFO, "Binding port to socket\n");
      if(bind(socket_fd, (struct sockaddr*) &self, sizeof(self)) != 0 ) {
	debugLog(LOG_ERR, "bind: %s\n", strerror(errno));
	exit(errno);
      }

      /* Make it a "listening socket" */
      if (listen(socket_fd, 20) != 0) {
	debugLog(LOG_ERR, "listen: %s\n", strerror(errno));
	exit(errno);
      }
    }

    debugLog(LOG_INFO, "fritzident daemon started om port %i\n", Port);

    /* Infinite loop */
    while (1) {
      /* Bind port to socket */
      /* Await a connection on socket_fd*/

      /* debugLog("Waiting for connection from fritz!box...\n"); */
      client_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &addrlen);
      if(client_fd < 0){
	debugLog(LOG_ERR, "accept %s:%d connection failed: %s\n",
		 inet_ntoa(client_addr.sin_addr),
		 ntohs(client_addr.sin_port),
		 strerror(errno));
	exit(errno);
      }
      
      bytes = send(client_fd, "AVM IDENT\r\n", strlen("AVM IDENT\r\n"), 0);
      if(bytes < 0) {
	debugLog(LOG_ERR, "Sending \"AVM IDENT\" failed!");
	exit(-1);
      }

      /* wait for data */
      bzero(&cmd, sizeof(cmd));
      bytes = recv(client_fd, cmd, sizeof(cmd)-1, 0);

      if(bytes == 0) continue;
      else if(bytes < 0){
	debugLog(LOG_ERR, "recv: %s\n", strerror(errno));
	exit(errno);
      }

      cmd[bytes] = '\0';
      /* the first token is containing the command which might be
       * USERS, TCP, or UDP and is usually seperated by spaces */
      char *cmdVerb = strtok(cmd, "\r\n ");

      /* debugLog("Received command: \"%s\"\n", cmdVerb); */
      if (strcmp(cmdVerb, "USERS") == 0) {
	execUSERS(client_fd);
      }
      else if (strcmp(cmdVerb, "TCP") == 0) {
	char *localIp = strtok(NULL, ":");
	char *localPort = strtok(NULL, "\r\n");
	debugLog(LOG_DEBUG, "Searching for \"%s:%s\"\n", localIp, localPort);
	if(localIp != NULL && localPort != NULL)
	  execTCP(client_fd, localIp, localPort);
	else sendResponse(client_fd, "ERROR UNSPECIFIED\r\n");
      }
      else if (strcmp(cmdVerb, "UDP") == 0) {
	char *localIp = strtok(NULL, ":");
	char *localPort = strtok(NULL, "\r\n");
	debugLog(LOG_DEBUG, "Searching for \"%s:%s\"\n", localIp, localPort);
	if(localIp != NULL && localPort != NULL)
	  execUDP(client_fd, localIp, localPort);
	else sendResponse(client_fd, "ERROR UNSPECIFIED\r\n");
      } else {
	debugLog(LOG_NOTICE, "Unrecognized command \"%s\"\n", cmd);
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
    printf("\t-v increase verbosity (may be assed multiple times.\n");
    printf("\t-p Port to listen on if not 14013 (for debugging only)\n");
    printf("\t-d domain ...... fake a Windows domain\n");
    printf("\nLICENSE:\n");
    printf("This utility is provided under the GNU GENERAL PUBLIC LICENSE v3.0\n(see http://www.gnu.org/licenses/gpl-3.0.txt)\n");
}
