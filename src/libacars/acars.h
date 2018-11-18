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

#ifndef LA_ACARS_H
#define LA_ACARS_H 1
#include <stdint.h>
#include <stdbool.h>
#include <libacars/libacars.h>			// la_proto_node, la_type_descriptor
#include <libacars/vstring.h>			// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool crc_ok;
	bool err;
	char mode;
	char reg[8];
	char ack;
	char label[3];
	char block_id;
	char bs;
	char no[5];
	char flight_id[7];
	char *txt;
} la_acars_msg;

// acars.c
extern la_type_descriptor const la_DEF_acars_message;
la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir const msg_dir);
void la_acars_format_text(la_vstring *vstr, void const * const data, int indent);
la_proto_node *la_proto_tree_find_acars(la_proto_node *root);
#ifdef __cplusplus
}
#endif
#endif // !LA_ACARS_H
