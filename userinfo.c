/*
 * userinfo.c
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
#include <stdlib.h>
#include <string.h>
#include "userinfo.h"

static char *default_domain=NULL;
static struct uid_range *included_uids=NULL;

struct uid_range *add_uid_range(uid_t min, uid_t max)
{
	// walk through the list
	struct uid_range **hereptr = &included_uids;
	struct uid_range *here = *hereptr;
	while (here) {
		// check if the new range will touch the current range
		// there are 3 possibilities
		//  1) the new range does not touch and comes before
		//  2) the new range does not touch and goes behind
		//  3) the new range does overlap or touch
		if (max < here->min) { // non-touching, before
			// insert the new range before the current one
			struct uid_range *new = (struct uid_range *)malloc(sizeof(struct uid_range));
			new->min = min;
			new->max = max;
			new->link = here;
			*hereptr = new;
			return new;
		}
		if (min < here->min && max >= here->min) {  // overlapping at front
			// update the current range to start earlier
			here->min = min;
			// check if there's more overlap behind
			if (max > here->max) {
				min = here->max+1;
				// let the next iterations check where to fit this
			}
			else {  // otherwise, we're done
				return here;
			}
		}
		hereptr = &(here->link);
		here = *hereptr;
	}
	// reached the end of the list without finding a place to insert
	// append a new range
	*hereptr = (struct uid_range *)malloc(sizeof(struct uid_range));
	(*hereptr)->min = min;
	(*hereptr)->max = max;
	(*hereptr)->link = NULL;
	return (*hereptr);
}

// check if uid is included in our list of ranges
int included_uid(uid_t id)
{
	struct uid_range *here = included_uids;
	while (here) {
		if (here->min <= id && id <= here->max)
			return 1;
		here = here->link;
	}
	return 0;
}

void set_default_domain(const char *domain)
{
	if (default_domain) free(default_domain);
	default_domain = strdup(domain);
}

char *add_default_domain(char *username)
{
	static char *buffer=NULL;
    char *p = strchr(username, '\\');
    if (p == NULL) {
		if (default_domain) {
			if (buffer) free(buffer);
			buffer=(char *)malloc(strlen(default_domain)+strlen(username)+2);
			strcpy(buffer, default_domain);
			strcat(buffer, "\\");
			strcat(buffer, username);
			return buffer;
		}
		else
	        return username;
	}
    else
        return username;
}