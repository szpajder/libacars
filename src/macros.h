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

#ifndef LA_MACROS_H
#define LA_MACROS_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __GNUC__
#define LA_LIKELY(x) (__builtin_expect(!!(x),1))
#define LA_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#define LA_LIKELY(x) (x)
#define LA_UNLIKELY(x) (x)
#endif

#ifdef __GNUC__
#define LA_PRETTY_FUNCTION __PRETTY_FUNCTION__
#else
#define LA_PRETTY_FUNCTION ""
#endif

#define la_assert_se(expr)			\
	do {					\
		if (LA_UNLIKELY(!(expr))) {	\
			fprintf(stderr, "Assertion '%s' failed at %s:%u, function %s(). Aborting.\n", \
				#expr , __FILE__, __LINE__, LA_PRETTY_FUNCTION); \
			abort();		\
		}				\
	} while (false)

#define la_nop() do {} while (false)

#ifdef NDEBUG
#define la_assert(expr) la_nop()
#else
#define la_assert(expr) la_assert_se(expr)
#endif

#if defined(__GNUC__)
#ifdef __MINGW32__
/* libintl overrides printf with a #define. As this breaks this attribute,
 * it has a workaround. However the workaround isn't enabled for MINGW
 * builds (only cygwin) */
#define LA_GCC_PRINTF_ATTR(a,b) __attribute__ ((format (__printf__, a, b)))
#else
#define LA_GCC_PRINTF_ATTR(a,b) __attribute__ ((format (printf, a, b)))
#endif
#else
#define LA_GCC_PRINTF_ATTR(a,b)
#endif

#define LA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define LA_MIN(a, b) ((a) < (b) ? (a) : (b))

#endif // LA_MACROS_H
