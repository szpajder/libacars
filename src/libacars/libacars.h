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

#ifndef LA_LIBACARS_H
#define LA_LIBACARS_H 1
#include <stdbool.h>
#include <libacars/vstring.h>		// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LA_MSG_DIR_UNKNOWN,
	LA_MSG_DIR_GND2AIR,
	LA_MSG_DIR_AIR2GND
} la_msg_dir;

typedef void (la_print_type_f)(la_vstring * const vstr, void const * const data, int indent);
typedef void (la_destroy_type_f)(void *data);

typedef struct {
	bool dump_asn1;
} la_config_struct;

typedef struct {
	char const * const header;
	la_print_type_f *format_text;
	la_destroy_type_f *destroy;
} la_type_descriptor;

typedef struct la_proto_node la_proto_node;

struct la_proto_node {
	la_type_descriptor const *td;
	void *data;
	la_proto_node *next;
};

// libacars.c
extern la_config_struct la_config;
la_proto_node *la_proto_node_new();
la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root);
void la_proto_tree_destroy(la_proto_node *root);
la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const * const td);

#ifdef __cplusplus
}
#endif

#endif // !LA_LIBACARS_H
