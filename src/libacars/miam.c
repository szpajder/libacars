/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>		// calloc
#include <stdio.h>		// printf
#include <string.h>		// strchr
#include <zlib.h>
#include <libacars/macros.h>	// la_assert
#include <libacars/libacars.h>	// la_proto_node
#include <libacars/util.h>	// la_dict, la_dict_search()
#include <libacars/miam.h>
#include <libacars/miam-core.h>

typedef struct {
	char fid_char;
	la_miam_frame_id frame_id;
} la_miam_frame_id_map;

static la_miam_frame_id_map const frame_id_map[LA_MIAM_FRAME_ID_CNT] = {
	{ .fid_char= 'T',  .frame_id = LA_MIAM_FID_SINGLE_TRANSFER },
	{ .fid_char= 'F',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_REQ },
	{ .fid_char= 'K',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_ACCEPT },
	{ .fid_char= 'S',  .frame_id = LA_MIAM_FID_FILE_SEGMENT },
	{ .fid_char= 'A',  .frame_id = LA_MIAM_FID_TRANSFER_ABORT },
	{ .fid_char= 'Y',  .frame_id = LA_MIAM_FID_XOFF_IND },
	{ .fid_char= 'X',  .frame_id = LA_MIAM_FID_XON_IND },
	{ .fid_char= '\0', .frame_id = LA_MIAM_FID_UNKNOWN },
};

typedef la_proto_node* (la_miam_frame_parse_f)(char const * const label, char const *txt, la_msg_dir const msg_dir);
typedef struct {
	char *description;
	la_miam_frame_parse_f *parse;
} la_miam_frame_id_descriptor;

/*
static char const * const miam_frame_id_descriptor_table[LA_MIAM_FRAME_ID_CNT] = {
	[LA_MIAM_FID_UNKNOWN]			= "Unknown MIAM frame type",
	[LA_MIAM_FID_SINGLE_TRANSFER]		= "MIAM Single Transfer",
	[LA_MIAM_FID_FILE_TRANSFER_REQ]		= "MIAM File Transfer Request",
	[LA_MIAM_FID_FILE_TRANSFER_ACCEPT]	= "MIAM File Transfer Accept",
	[LA_MIAM_FID_FILE_SEGMENT]		= "MIAM File Segment",
	[LA_MIAM_FID_TRANSFER_ABORT]		= "MIAM Transfer Abort",
	[LA_MIAM_FID_XOFF_IND]			= "MIAM XOFF Indication",
	[LA_MIAM_FID_XON_IND]			= "MIAM XON Indication"
}; */

static la_dict const la_miam_frame_id_descriptor_table[] = {
	{
		.id = LA_MIAM_FID_SINGLE_TRANSFER,
		.val = &(la_miam_frame_id_descriptor){
			.description = "MIAM Single Transfer",
			.parse = &la_miam_single_transfer_parse
		}
	},
// TODO: add all types
	{
		.id = 0,
		.val = NULL
	}
};

la_proto_node *la_miam_single_transfer_parse(char const * const label, char const *txt, la_msg_dir const msg_dir) {
	return la_miam_core_pdu_parse(label, txt, msg_dir);
}

la_proto_node *la_miam_parse(char const * const label, char const *txt, la_msg_dir const msg_dir) {
	if(txt == NULL) {
		return NULL;
	}
	la_assert(label != NULL);
	la_assert(strlen(label) >= 2);

	size_t len = strlen(txt);
// FIXME
	la_debug_print("%s\n", txt);

// Handle messages to ACARS peripherals (label H1):
// - uplinks with OAT prefix "- #<2-char-sublabel>",
// - downlinks with OAT prefix "#<2-char-sublabel>B"
// Strip the OAT prefix and sublabel field, then continue as usual.
	if(label[0] == 'H' && label[1] == '1') {
		if(msg_dir == LA_MSG_DIR_GND2AIR) {
			if(len >= 5 && strncmp(txt, "- #", 3) == 0) {
				txt += 5; len -= 5;
			}
		} else if(msg_dir == LA_MSG_DIR_AIR2GND) {
			if(len >= 4 && txt[0] == '#' && txt[3] == 'B') {
				txt += 4; len -= 4;
			}
		}
	}
// First character identifies the ACARS CF frame
	if(len == 0) {
		return NULL;
	}
	la_miam_frame_id fid = LA_MIAM_FID_UNKNOWN; 		// safe default
	for(int i = 0; i < LA_MIAM_FRAME_ID_CNT; i++) {
		if(txt[0] == frame_id_map[i].fid_char) {
			fid = frame_id_map[i].frame_id;
			la_debug_print("txt[0]: %c frame_id: %d\n", txt[0], fid);
			break;
		}
	}
	if(fid == LA_MIAM_FID_UNKNOWN) {
		la_debug_print("%s", "not a MIAM message (unknown ACARS CF frame)\n");
		return NULL;
	}
	la_miam_frame_id_descriptor *fid_descriptor = la_dict_search(la_miam_frame_id_descriptor_table, fid);
	if(fid_descriptor == NULL) {
		la_debug_print("Warning: no type descriptor defined for ACARS CF frame '%c' (%d)\n", txt[0], fid);
		return NULL;
	}
	txt++; len--;
	la_proto_node *next_node = fid_descriptor->parse(label, txt, msg_dir);
	if(next_node == NULL) {
		return NULL;
	}
	la_miam_msg *msg = LA_XCALLOC(1, sizeof(la_miam_msg));
	msg->frame_id = fid;
	la_proto_node *node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_miam_message;
	node->next = next_node;
	return node;
}

void la_miam_single_transfer_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_miam_core_format_text(vstr, data, indent);
}

void la_miam_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_msg *, data);
	la_miam_frame_id_descriptor *fid_descriptor = la_dict_search(la_miam_frame_id_descriptor_table, msg->frame_id);
	la_assert(fid_descriptor != NULL);
	LA_ISPRINTF(vstr, indent, "%s:\n", fid_descriptor->description);
}

la_type_descriptor const la_DEF_miam_message = {
	.format_text = la_miam_format_text,
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_single_transfer_message = {
	.format_text = la_miam_single_transfer_format_text,
	.destroy = NULL
};

la_proto_node *la_proto_tree_find_miam(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_miam_message);
}
