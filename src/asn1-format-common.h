/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2017-2018 Tomasz Lemiech <szpajder@gmail.com>
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

#ifndef LA_ASN1_FORMAT_COMMON_H
#define LA_ASN1_FORMAT_COMMON_H 1
#include "asn1/asn_application.h"	// asn_TYPE_descriptor_t
#include "asn1-util.h"			// LA_ASN1_FORMATTER_PROTOTYPE
#include "util.h"			// la_dict
#include "vstring.h"			// la_vstring

char const *la_value2enum(asn_TYPE_descriptor_t *td, long const value);
void la_format_INTEGER_with_unit(la_vstring *vstr, char const * const label, asn_TYPE_descriptor_t *td,
	void const *sptr, int indent, char const * const unit, double multiplier, int decimal_places);
void la_format_CHOICE(la_vstring *vstr, char const * const label, la_dict const * const choice_labels,
	asn1_output_fun_t cb, asn_TYPE_descriptor_t *td, void const *sptr, int indent);
void la_format_SEQUENCE(la_vstring *vstr, char const * const label, asn1_output_fun_t cb,
	asn_TYPE_descriptor_t *td, void const *sptr, int indent);
void la_format_SEQUENCE_OF(la_vstring *vstr, char const * const label, asn1_output_fun_t cb,
	asn_TYPE_descriptor_t *td, void const *sptr, int indent);
LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_any);
LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_IA5String);
LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_NULL);
LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_ENUM);
LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_Deg);

#endif // !LA_ASN1_FORMAT_COMMON_H
