/*
 * userinfo.h
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

#include <pwd.h>

struct uid_range {
	uid_t min;
	uid_t max;
	struct uid_range *link;
};

struct uid_range *add_uid_range(uid_t min, uid_t max);
int included_uid(uid_t id);

void set_default_domain(const char *domain);
char *add_default_domain(char *username);