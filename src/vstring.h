/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LA_VSTRING_H
#define LA_VSTRING_H

#include <stdbool.h>
#include "macros.h"		// LA_GCC_PRINTF_ATTR

typedef struct {
	char *str;		// string buffer pointer
	int len;		// current length of the string (excl. '\0')
	int allocated_size;	// current allocated buffer size (ie. max len = allocated_len - 1)
} la_vstring;

la_vstring *la_vstring_new();
void la_vstring_destroy(la_vstring *vstr, bool destroy_buffer);
void la_vstring_append_sprintf(la_vstring * const vstr, char const *fmt, ...) LA_GCC_PRINTF_ATTR(2, 3);

#endif // LA_VSTRING_H
