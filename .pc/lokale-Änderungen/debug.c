/*
 * debug.c
 *
 * Copyright (C) 2013 - Unknown
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *logfilename=NULL;
static FILE *logfile=NULL;

void debugLogFile(const char *file)
{
	if (logfilename) free(logfilename);
	if (logfile) fclose(logfile);

	logfilename = strdup(file);
	logfile = fopen(logfilename, "a");	
}

void debugLog(const char *fmsg, ...)
{
	va_list args;
	if (logfile == NULL) return;

	va_start(args, fmsg);
	vfprintf(logfile, fmsg, args);
	fflush(logfile);
	va_end(args);
}

