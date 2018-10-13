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

#ifndef LA_CPDLC_H
#define LA_CPDLC_H 1

#include <stdint.h>
#include "libacars.h"		// la_type_descriptor, la_proto_node
#include "vstring.h"		// la_vstring
#include "asn1/constr_TYPE.h"	// asn_TYPE_descriptor_t

typedef struct {
	asn_TYPE_descriptor_t *asn_type;
	void *data;
	int err;
} la_cpdlc_msg;

// cpdlc.c
extern int la_enable_asn1_dumps;
extern la_type_descriptor const la_DEF_cpdlc_message;

la_proto_node *la_cpdlc_parse(uint8_t *buf, int len, la_msg_dir const msg_dir);
void la_cpdlc_format_text(la_vstring * const vstr, void const * const data);
void la_cpdlc_destroy(void *data);

#endif // !LA_CPDLC_H
