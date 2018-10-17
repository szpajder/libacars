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

#include "macros.h"		// la_assert
#include "libacars.h"		// la_proto_node
#include "vstring.h"
#include "util.h"		// LA_XCALLOC, LA_XFREE

static void la_proto_node_format_text(la_vstring * const vstr, la_proto_node const * const node, int indent) {
	la_assert(indent >= 0);
	if(node->data != NULL) {
		la_assert(node->td);
		if(node->td->header != NULL) {
			LA_ISPRINTF(vstr, indent, "%s:\n", node->td->header);
		}
		node->td->format_text(vstr, node->data, indent);
	}
	if(node->next != NULL) {
		la_proto_node_format_text(vstr, node->next, indent+1);
	}
}

la_proto_node *la_proto_node_new() {
	la_proto_node *node = LA_XCALLOC(1, sizeof(la_proto_node));
	return node;
}

la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root) {
	la_assert(root);

	if(vstr == NULL) {
		vstr = la_vstring_new();
	}
	la_proto_node_format_text(vstr, root, 0);
	return vstr;
}

void la_proto_tree_destroy(la_proto_node *root) {
	if(root == NULL) {
		return;
	}
	if(root->next != NULL) {
		la_proto_tree_destroy(root->next);
	}
	if(root->td != NULL && root->td->destroy != NULL) {
		root->td->destroy(root->data);
	} else {
		LA_XFREE(root->data);
	}
	LA_XFREE(root);
}
