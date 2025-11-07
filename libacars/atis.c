/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2025-2025 Yuuta Liang <yuuta@yuuta.moe>
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libacars/macros.h>                        // la_assert
#include <libacars/atis.h>                          // la_atis_msg
#include <libacars/libacars.h>                      // la_proto_node, la_config_get_bool, la_proto_tree_find_protocol
#include <libacars/macros.h>                        // la_debug_print
#include <libacars/util.h>                          // LA_XFREE
#include <libacars/vstring.h>                       // la_vstring, la_vstring_append_sprintf()
#include <libacars/json.h>                          // la_json_append_bool()

la_proto_node *la_atis_parse(uint8_t const *buf, int len, la_msg_dir msg_dir) {
	if(buf == NULL)
		return NULL;

	la_proto_node *node = la_proto_node_new();
	LA_NEW(la_atis_msg, msg);
	node->data = msg;
	node->td = &la_DEF_atis_message;

	memset(msg, 0, sizeof(la_atis_msg));
	if(msg_dir == LA_MSG_DIR_GND2AIR) {
		msg->is_response = 1;
		if(len < 23) {
			msg->err = true;
			return node;
		}
		la_debug_print(D_INFO, "Decoding ATIS response, len: %d\n", len);
		memcpy(msg->data.response.airport, buf + 0, 4);
		memcpy(msg->data.response.type, buf + 5, 3);
		memcpy(msg->data.response.version, buf + 14, 1);
		memcpy(msg->data.response.time, buf + 17, 4);
		msg->data.response.content = LA_XCALLOC(len + 1, sizeof(char));
		memcpy(msg->data.response.content, buf, len);
		msg->data.response.content[len] = 0;
	} else if(msg_dir == LA_MSG_DIR_AIR2GND) {
		msg->is_response = 0;
		la_debug_print(D_INFO, "Decoding ATIS request, len: %d\n", len);
		if(len < 8) {
			msg->err = true;
			return node;
		}
		memcpy(msg->data.request.avionics_indicator, buf + 0, 3);
		memcpy(msg->data.request.airport, buf + 3, 4);
		memcpy(msg->data.request.type, buf + 7, 1);
	}
	return node;
}

void la_atis_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	la_atis_msg const *msg = data;
	char *request_type_str;
	if(msg->err == true) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable ATIS message\n");
		return;
	}
	if(msg->is_response) {
		LA_ISPRINTF(vstr, indent, "ATIS response:\n");
		LA_ISPRINTF(vstr, indent + 1, "Airport: %s\n",
				msg->data.response.airport);
		LA_ISPRINTF(vstr, indent + 1, "Type: %s\n",
				msg->data.response.type);
		LA_ISPRINTF(vstr, indent + 1, "Version: %s\n",
				msg->data.response.version);
		LA_ISPRINTF(vstr, indent + 1, "Time: %c%c:%c%cZ\n",
				msg->data.response.time[0],
				msg->data.response.time[1],
				msg->data.response.time[2],
				msg->data.response.time[3]);
		LA_ISPRINTF(vstr, indent + 1, "Content:\n");
		la_isprintf_multiline_text(vstr, indent + 2,
				msg->data.response.content);
	} else {
		switch(msg->data.request.type[0]) {
			case ATIS_REQUEST_TYPE_ARRIVAL:
				request_type_str = "Arrival";
				break;
			case ATIS_REQUEST_TYPE_DEPARTURE:
				request_type_str = "Departure";
				break;
			case ATIS_REQUEST_TYPE_ARRIVAL_AUTO:
				request_type_str = "Automatic arrival updates";
				break;
			case ATIS_REQUEST_TYPE_ENROUTE:
				request_type_str = "Enroute / VOLMET";
				break;
			case ATIS_REQUEST_TYPE_TERMINATE:
				request_type_str = "Terminate automatic updates";
				break;
			default:
				request_type_str = "Unknown";
				break;
		}
		LA_ISPRINTF(vstr, indent, "ATIS request:\n");
		LA_ISPRINTF(vstr, indent + 1, "Line break: %s chars max\n",
				msg->data.request.avionics_indicator);
		LA_ISPRINTF(vstr, indent + 1, "Airport: %s\n",
				msg->data.request.airport);
		LA_ISPRINTF(vstr, indent + 1, "Type: %s (%s)\n",
				msg->data.request.type,
				request_type_str);
	}
}

void la_atis_format_json(la_vstring *vstr, void const *data) {
	la_assert(vstr);
	la_assert(data);

	la_atis_msg const *msg = data;
	la_json_append_bool(vstr, "err", msg->err);
	if(msg->err == true) {
		return;
	}
	if(msg->is_response) {
		la_json_object_start(vstr, "response");
		la_json_append_string(vstr, "airport", msg->data.response.airport);
		la_json_append_string(vstr, "type", msg->data.response.type);
		la_json_append_string(vstr, "version", msg->data.response.version);
		la_json_append_string(vstr, "time", msg->data.response.time);
		la_json_append_string(vstr, "content", msg->data.response.content);
		la_json_object_end(vstr);
	} else {
		la_json_object_start(vstr, "request");
		la_json_append_string(vstr, "avionics_indicator",
				msg->data.request.avionics_indicator);
		la_json_append_string(vstr, "airport", msg->data.request.airport);
		la_json_append_string(vstr, "type", msg->data.request.type);
		la_json_object_end(vstr);
	}
}

void la_atis_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_atis_msg *msg = data;
	if (msg->is_response && msg->data.response.content) {
		LA_XFREE(msg->data.response.content);
	}
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_atis_message = {
	.format_text = la_atis_format_text,
	.format_json = la_atis_format_json,
	.json_key = "atis",
	.destroy = la_atis_destroy
};

la_proto_node *la_proto_tree_find_atis(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_atis_message);
}
