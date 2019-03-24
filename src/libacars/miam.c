/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>		// calloc()
#include <string.h>		// strchr(), strlen(), strncmp()
#include <libacars/macros.h>	// la_assert()
#include <libacars/libacars.h>	// la_proto_node, la_type_descriptor
#include <libacars/vstring.h>	// la_vstring
#include <libacars/json.h>	// la_json_append_*()
#include <libacars/util.h>	// la_dict, la_dict_search(),
				// la_strntouint16_t(), la_simple_strptime()
#include <libacars/miam-core.h> // la_miam_core_pdu_parse(), la_miam_core_format_*()
#include <libacars/miam.h>

typedef struct {
	char fid_char;
	la_miam_frame_id frame_id;
} la_miam_frame_id_map;

static la_miam_frame_id_map const frame_id_map[LA_MIAM_FRAME_ID_CNT] = {
	{ .fid_char= 'T',  .frame_id = LA_MIAM_FID_SINGLE_TRANSFER },
	{ .fid_char= 'F',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_REQ },
	{ .fid_char= 'K',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_ACCEPT },
	{ .fid_char= 'S',  .frame_id = LA_MIAM_FID_FILE_SEGMENT },
	{ .fid_char= 'A',  .frame_id = LA_MIAM_FID_FILE_TRANSFER_ABORT },
	{ .fid_char= 'Y',  .frame_id = LA_MIAM_FID_XOFF_IND },
	{ .fid_char= 'X',  .frame_id = LA_MIAM_FID_XON_IND },
	{ .fid_char= '\0', .frame_id = LA_MIAM_FID_UNKNOWN },
};

typedef la_proto_node* (la_miam_frame_parse_f)(char const *txt);
typedef struct {
	char *description;
	la_miam_frame_parse_f *parse;
} la_miam_frame_id_descriptor;

// Forward declarations

static la_proto_node *la_miam_single_transfer_parse(char const *txt);
static la_proto_node *la_miam_file_transfer_request_parse(char const *txt);
static la_proto_node *la_miam_file_transfer_accept_parse(char const *txt);
static la_proto_node *la_miam_file_segment_parse(char const *txt);
static la_proto_node *la_miam_file_transfer_abort_parse(char const *txt);
static la_proto_node *la_miam_xoff_ind_parse(char const *txt);
static la_proto_node *la_miam_xon_ind_parse(char const *txt);

static la_dict const la_miam_frame_id_descriptor_table[] = {
	{
		.id = LA_MIAM_FID_SINGLE_TRANSFER,
		.val = &(la_miam_frame_id_descriptor){
			.description = "Single Transfer",
			.parse = &la_miam_single_transfer_parse
		}
	},
	{
		.id = LA_MIAM_FID_FILE_TRANSFER_REQ,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Transfer Request",
			.parse = &la_miam_file_transfer_request_parse
		}
	},
	{
		.id = LA_MIAM_FID_FILE_TRANSFER_ACCEPT,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Transfer Accept",
			.parse = &la_miam_file_transfer_accept_parse
		}
	},
	{
		.id = LA_MIAM_FID_FILE_SEGMENT,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Segment",
			.parse = &la_miam_file_segment_parse
		}
	},
	{
		.id = LA_MIAM_FID_FILE_TRANSFER_ABORT,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Transfer Abort",
			.parse = &la_miam_file_transfer_abort_parse
		}
	},
	{
		.id = LA_MIAM_FID_XOFF_IND,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Transfer Pause",
			.parse = &la_miam_xoff_ind_parse
		}
	},
	{
		.id = LA_MIAM_FID_XON_IND,
		.val = &(la_miam_frame_id_descriptor){
			.description = "File Transfer Resume",
			.parse = &la_miam_xon_ind_parse
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

static la_proto_node *la_miam_single_transfer_parse(char const *txt) {
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_single_transfer_message;
	node->data = NULL;
	node->next = la_miam_core_pdu_parse(txt);
	return node;
}

static la_proto_node *la_miam_file_transfer_request_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_file_transfer_request_msg *msg = NULL;
	if(chomped_strlen(txt) != 21) {
		goto hdr_error;
	}

	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_request_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 6)) < 0) {
		goto hdr_error;
	}
	msg->file_size = (size_t)i;
	txt += 6;

	la_debug_print("file_id: %u file_size: %zu\n", msg->file_id, msg->file_size);
	char const *ptr = la_simple_strptime(txt, &msg->validity_time);
	if(ptr == NULL) {
		goto hdr_error;
	}

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_request_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a file_transfer_request header");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_file_transfer_accept_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_file_transfer_accept_msg *msg = NULL;
	if(chomped_strlen(txt) != 10) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_accept_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if(txt[0] >= '0' && txt[0] <= '9') {
		msg->segment_size = txt[0] - '0';
	} else if(txt[0] >= 'A' && txt[0] <= 'F') {
		msg->segment_size = txt[0] - 'A' + 10;
	} else {
		goto hdr_error;
	}
	txt++;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->onground_segment_tempo = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->inflight_segment_tempo = (uint16_t)i;
	txt += 3;

	la_debug_print("file_id: %u seg_size: %u onground_tempo: %u inflight_tempo: %u\n",
		msg->file_id, msg->segment_size, msg->onground_segment_tempo, msg->inflight_segment_tempo);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_accept_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a file_transfer_accept header");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_file_segment_parse(char const *txt) {
	la_assert(txt != NULL);
	la_miam_file_segment_msg *msg = LA_XCALLOC(1, sizeof(la_miam_file_segment_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->segment_id = (uint16_t)i;
	txt += 3;

	la_debug_print("file_id: %u segment_id: %u\n", msg->file_id, msg->segment_id);

// MIAM File Segment headers have very simple structure and can easily be confused with
// various non-MIAM messages, especially when sent with H1 label. la_miam_core_pdu_parse()
// performs more thorough checks - if it fails to identify its input as a MIAM CORE PDU,
// then we declare that this message is not MIAM.
	void *next = la_miam_core_pdu_parse(txt);
	if(next == NULL) {
		goto hdr_error;
	}

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_segment_message;
	node->data = msg;
	node->next = next;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a file_segment header");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_file_transfer_abort_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_file_transfer_abort_msg *msg = NULL;
	if(chomped_strlen(txt) != 4) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_file_transfer_abort_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if(txt[0] >= '0' && txt[0] <= '9') {
		msg->reason = txt[0] - '0';
	} else {
		goto hdr_error;
	}
	txt++;

	la_debug_print("file_id: %u reason: %u\n", msg->file_id, msg->reason);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_file_transfer_abort_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a file_transfer_abort header");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_xoff_ind_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_xoff_ind_msg *msg = NULL;
	if(chomped_strlen(txt) != 3) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_xoff_ind_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		if(strncmp(txt, "FFF", 3) == 0) {
			i = 0xFFF;
		} else {
			goto hdr_error;
		}
	}
	msg->file_id = (uint16_t)i;
	txt += 3;
	la_debug_print("file_id: %u\n", msg->file_id);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_xoff_ind_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a xoff_ind header");
	LA_XFREE(msg);
	return NULL;
}

static la_proto_node *la_miam_xon_ind_parse(char const *txt) {
	la_assert(txt != NULL);

	la_miam_xon_ind_msg *msg = NULL;
	if(chomped_strlen(txt) != 9) {
		goto hdr_error;
	}
	msg = LA_XCALLOC(1, sizeof(la_miam_xon_ind_msg));
	int i;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		if(strncmp(txt, "FFF", 3) == 0) {
			i = 0xFFF;
		} else {
			goto hdr_error;
		}
	}
	msg->file_id = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->onground_segment_tempo = (uint16_t)i;
	txt += 3;

	if((i = la_strntouint16_t(txt, 3)) < 0) {
		goto hdr_error;
	}
	msg->inflight_segment_tempo = (uint16_t)i;
	txt += 3;

	la_debug_print("file_id: %u onground_tempo: %u inflight_tempo: %u\n",
		msg->file_id, msg->onground_segment_tempo, msg->inflight_segment_tempo);

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_xon_ind_message;
	node->data = msg;
	node->next = NULL;
	return node;
hdr_error:
	la_debug_print("%s\n", "Not a xon_ind header");
	LA_XFREE(msg);
	return NULL;
}

la_proto_node *la_miam_parse(char const * const label, char const *txt, la_msg_dir const msg_dir) {
	if(txt == NULL) {
		return NULL;
	}
	la_assert(label != NULL);
	la_assert(strlen(label) >= 2);

	size_t len = strlen(txt);

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
	la_proto_node *next_node = fid_descriptor->parse(txt);
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

static void la_miam_single_transfer_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_miam_core_format_text(vstr, data, indent);
}

static void la_miam_single_transfer_format_json(la_vstring * const vstr, void const * const data) {
	la_miam_core_format_json(vstr, data);
}

static void la_miam_file_transfer_request_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_file_transfer_request_msg *, data);
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "File size: %zu bytes\n", msg->file_size);
	struct tm *t = &msg->validity_time;
	LA_ISPRINTF(vstr, indent, "Complete until: %d-%02d-%02d %02d:%02d:%02d\n",
		t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
		t->tm_hour, t->tm_min, t->tm_sec
	);
}

static void la_miam_file_transfer_request_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_file_transfer_request_msg *, data);
	la_json_append_long(vstr, "file_id", msg->file_id);
	la_json_append_long(vstr, "file_size", msg->file_size);
	struct tm *t = &msg->validity_time;
	la_json_object_start(vstr, "complete_until_datetime");
	la_json_object_start(vstr, "date");
	la_json_append_long(vstr, "year", t->tm_year + 1900);
	la_json_append_long(vstr, "month", t->tm_mon + 1);
	la_json_append_long(vstr, "day", t->tm_mday);
	la_json_object_end(vstr);
	la_json_object_start(vstr, "time");
	la_json_append_long(vstr, "hour", t->tm_hour);
	la_json_append_long(vstr, "minute", t->tm_min);
	la_json_append_long(vstr, "second", t->tm_sec);
	la_json_object_end(vstr);
	la_json_object_end(vstr);
}

static void la_miam_file_transfer_accept_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_file_transfer_accept_msg *, data);
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "Segment size: %u\n", msg->segment_size);
	LA_ISPRINTF(vstr, indent, "On-ground segment temporization: %u sec\n", msg->onground_segment_tempo);
	LA_ISPRINTF(vstr, indent, "In-flight segment temporization: %u sec\n", msg->inflight_segment_tempo);
}

static void la_miam_file_transfer_accept_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_file_transfer_accept_msg *, data);
	la_json_append_long(vstr, "file_id", msg->file_id);
	la_json_append_long(vstr, "segment_size", msg->segment_size);
	la_json_append_long(vstr, "on_ground_seg_temp_secs", msg->onground_segment_tempo);
	la_json_append_long(vstr, "in_flight_seg_temp_secs", msg->inflight_segment_tempo);
}

static void la_miam_file_segment_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_file_segment_msg *, data);
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	LA_ISPRINTF(vstr, indent, "Segment ID: %u\n", msg->segment_id);
}

static void la_miam_file_segment_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_file_segment_msg *, data);
	la_json_append_long(vstr, "file_id", msg->file_id);
	la_json_append_long(vstr, "segment_id", msg->segment_id);
}

static void la_miam_file_transfer_abort_format_text(la_vstring * const vstr, void const * const data, int indent) {
	static la_dict const abort_reasons[] = {
		{ .id = 0, .val = "File transfer request refused by receiver" },
		{ .id = 1, .val = "File segment out of context" },
		{ .id = 2, .val = "File transfer stopped by sender" },
		{ .id = 3, .val = "File transfer stopped by receiver" },
		{ .id = 4, .val = "File segment transmission failed" },
		{ .id = 0, .val = NULL }
	};
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_file_transfer_abort_msg *, data);
	indent++;
	LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	char *descr = la_dict_search(abort_reasons, msg->reason);
	LA_ISPRINTF(vstr, indent, "Reason: %u (%s)\n", msg->reason,
		(descr != NULL ? descr : "unknown"));
}

static void la_miam_file_transfer_abort_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_file_transfer_abort_msg *, data);
	la_json_append_long(vstr, "file_id", msg->file_id);
	la_json_append_long(vstr, "reason", msg->reason);
}

static void la_miam_xoff_ind_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_xoff_ind_msg *, data);
	indent++;
	if(msg->file_id == 0xFFF) {
		LA_ISPRINTF(vstr, indent, "%s\n", "File ID: 0xFFF (all)");
	} else {
		LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	}
}

static void la_miam_xoff_ind_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_xoff_ind_msg *, data);
	la_json_append_bool(vstr, "all_files", msg->file_id == 0xFFF);
	if(msg->file_id != 0xFFF) {
		la_json_append_long(vstr, "file_id", msg->file_id);
	}
}

static void la_miam_xon_ind_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_xon_ind_msg *, data);
	indent++;
	if(msg->file_id == 0xFFF) {
		LA_ISPRINTF(vstr, indent, "%s\n", "File ID: 0xFFF (all)");
	} else {
		LA_ISPRINTF(vstr, indent, "File ID: %u\n", msg->file_id);
	}
	LA_ISPRINTF(vstr, indent, "On-ground segment temporization: %u sec\n", msg->onground_segment_tempo);
	LA_ISPRINTF(vstr, indent, "In-flight segment temporization: %u sec\n", msg->inflight_segment_tempo);
}

static void la_miam_xon_ind_format_json(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, la_miam_xon_ind_msg *, data);
	la_json_append_bool(vstr, "all_files", msg->file_id == 0xFFF);
	if(msg->file_id != 0xFFF) {
		la_json_append_long(vstr, "file_id", msg->file_id);
	}
	la_json_append_long(vstr, "on_ground_seg_temp_secs", msg->onground_segment_tempo);
	la_json_append_long(vstr, "in_flight_seg_temp_secs", msg->inflight_segment_tempo);
}

void la_miam_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_miam_msg *, data);
	la_miam_frame_id_descriptor *fid_descriptor = la_dict_search(la_miam_frame_id_descriptor_table, msg->frame_id);
	la_assert(fid_descriptor != NULL);
	LA_ISPRINTF(vstr, indent, "%s\n", "MIAM:");
	LA_ISPRINTF(vstr, indent+1, "%s:\n", fid_descriptor->description);
}

void la_miam_format_json(la_vstring * const vstr, void const * const data) {
// -Wunused-parameter
	(void)vstr;
	(void)data;
	// NOOP
}

la_type_descriptor const la_DEF_miam_message = {
	.format_text = la_miam_format_text,
	.format_json = la_miam_format_json,
	.json_key = "miam",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_single_transfer_message = {
	.format_text = la_miam_single_transfer_format_text,
	.format_json = la_miam_single_transfer_format_json,
	.json_key = "single_transfer",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_transfer_request_message = {
	.format_text = la_miam_file_transfer_request_format_text,
	.format_json = la_miam_file_transfer_request_format_json,
	.json_key = "file_transfer_request",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_transfer_accept_message = {
	.format_text = la_miam_file_transfer_accept_format_text,
	.format_json = la_miam_file_transfer_accept_format_json,
	.json_key = "file_transfer_accept",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_segment_message = {
	.format_text = la_miam_file_segment_format_text,
	.format_json = la_miam_file_segment_format_json,
	.json_key = "file_segment",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_file_transfer_abort_message = {
	.format_text = la_miam_file_transfer_abort_format_text,
	.format_json = la_miam_file_transfer_abort_format_json,
	.json_key = "file_transfer_abort",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_xoff_ind_message = {
	.format_text = la_miam_xoff_ind_format_text,
	.format_json = la_miam_xoff_ind_format_json,
	.json_key = "file_xoff_ind",
	.destroy = NULL
};

la_type_descriptor const la_DEF_miam_xon_ind_message = {
	.format_text = la_miam_xon_ind_format_text,
	.format_json = la_miam_xon_ind_format_json,
	.json_key = "file_xon_ind",
	.destroy = NULL
};

la_proto_node *la_proto_tree_find_miam(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_miam_message);
}
