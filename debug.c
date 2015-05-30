/*
 * debug.c
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


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>


void debugLog(int pri, const char *fmsg, ...)
{
	va_list args;
	
	va_start(args, fmsg);
	vsyslog(pri, fmsg, args);
	va_end(args);
}


/**
 * @brief Increase verbositx by one, max value = 7
 * 
 * @return void
 */
void raiseVerbosity()
{
  setlogmask((setlogmask(0) << 1) +1);
}

void initLogging(){
  setlogmask(LOG_UPTO(4));
}