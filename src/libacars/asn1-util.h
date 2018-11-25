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

#ifndef LA_ASN1_UTIL_H
#define LA_ASN1_UTIL_H 1
#include <stdint.h>				// uint8_t
#include <libacars/asn1/asn_application.h>	// asn_TYPE_descriptor_t
#include <libacars/vstring.h>			// la_vstring

typedef struct {
	asn_TYPE_descriptor_t *type;
// FIXME: typedef?
	void (*format)(la_vstring *vstr, char const * const label, asn_TYPE_descriptor_t *, const void *, int);
	char const * const label;
} la_asn_formatter;

typedef void (*asn1_output_fun_t)(la_vstring *, asn_TYPE_descriptor_t *, const void *, int);

#define LA_ASN1_FORMATTER_PROTOTYPE(x) void x(la_vstring *vstr, char const * const label, asn_TYPE_descriptor_t *td, void const *sptr, int indent)

// asn1-util.c
int la_asn1_decode_as(asn_TYPE_descriptor_t *td, void **struct_ptr, uint8_t *buf, int size);
void la_asn1_output(la_vstring *vstr, la_asn_formatter const * const asn1_formatter_table,
	size_t asn1_formatter_table_len, asn_TYPE_descriptor_t *td, const void *sptr, int indent);

#endif // !LA_ASN1_UTIL_H
