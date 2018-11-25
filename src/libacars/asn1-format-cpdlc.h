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

#ifndef LA_ASN1_FORMAT_CPDLC_H
#define LA_ASN1_FORMAT_CPDLC_H 1
#include <libacars/asn1/asn_application.h>	// asn_TYPE_descriptor_t
#include <libacars/vstring.h>			// la_vstring

// asn1-format-cpdlc.c
void la_asn1_output_cpdlc_as_text(la_vstring *vstr, asn_TYPE_descriptor_t *td, const void *sptr, int indent);

#endif // !LA_ASN1_FORMAT_CPDLC_H
