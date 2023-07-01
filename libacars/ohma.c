/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2021 Tomasz Lemiech <szpajder@gmail.com>
 */

#include "config.h"                 // WITH_ZLIB
#include <string.h>                 // strlen
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>               // struct timeval
#endif
#include <libacars/libacars.h>      // la_proto_node, la_type_descriptor
#include <libacars/reassembly.h>
#include <libacars/util.h>          // la_base64_decode
#include <libacars/macros.h>        // la_debug_print()
#include <libacars/ohma.h>          // la_ohma_msg

#define OHMA_MSG_MIN_LEN 4          // one BASE64 block

la_proto_node *la_ohma_parse_and_reassemble(char const *reg, char const *txt,
		la_reasm_ctx *rtables, struct timeval rx_time) {
#ifdef WITH_ZLIB
	if(txt == NULL) {
		return NULL;
	}
	size_t len = strlen(txt);
	if(len < OHMA_MSG_MIN_LEN) {
		return NULL;
	}
	if(strncmp(txt, "OHM", 3) != 0) {       // Not an OHMA message
		return NULL;
	}

	la_octet_string *b64_decoded_msg = la_base64_decode(txt, len);
	if(b64_decoded_msg == NULL) {
		la_debug_print(D_INFO, "Message text is not BASE64-encoded\n");
		return NULL;
	}

	if(b64_decoded_msg->len < 4) {
		la_debug_print(D_INFO, "BASE64-decoded message too short");
		return NULL;
	}

	la_debug_print_buf_hex(D_VERBOSE, b64_decoded_msg->buf, b64_decoded_msg->len, "Message after BASE64 decoding:\n");
	// The first three octets come as a result of decoding the initial
	// "OHMA" string which acts only as a message type indicator. Skip them.
	// Next two octets are the zlib magic value. Skip them too.
	la_inflate_result inflated = la_inflate(b64_decoded_msg->buf + 5, b64_decoded_msg->len);
	// If it's text, it needs a NULL terminator.
	// If it's not text, it doesn't hurt either. The buffer is larger than len anyway.
	inflated.buf[inflated.buflen] = '\0';

	la_octet_string_destroy(b64_decoded_msg);

	LA_NEW(la_ohma_msg, msg);
	msg->payload = la_octet_string_new(inflated.buf, inflated.buflen);
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_ohma_msg;
	node->data = msg;
	node->next = NULL;
	return node;
#else   // !WITH_ZLIB
	LA_UNUSED(reg);
	LA_UNUSED(txt);
	LA_UNUSED(rtables);
	LA_UNUSED(tx_time);
	return NULL;
#endif  // WITH_ZLIB
}

static void la_ohma_msg_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	la_ohma_msg *msg = data;
	la_octet_string_destroy(msg->payload);
	LA_XFREE(msg);
}

void la_ohma_format_text(la_vstring *vstr, void const *data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	la_ohma_msg const *msg = data;
	if(msg->err != LA_OHMA_SUCCESS) {
		LA_ISPRINTF(vstr, indent, "-- Unparseable OHMA message\n");
		return;
	}
	LA_ISPRINTF(vstr, indent, "OHMA message:\n");
	if(is_printable(msg->payload->buf, msg->payload->len)) {
		la_isprintf_multiline_text(vstr, indent + 1, (char *)msg->payload->buf);
	} else {
		char *hexdump = la_hexdump((uint8_t *)msg->payload->buf, msg->payload->len);
		la_isprintf_multiline_text(vstr, indent + 1, hexdump);
		LA_XFREE(hexdump);
	}
}

void la_ohma_format_json(la_vstring *vstr, void const *data);
la_proto_node *la_proto_tree_find_ohma(la_proto_node *root);

la_type_descriptor const la_DEF_ohma_msg = {
	.format_text = la_ohma_format_text,
	.format_json = NULL,
	.json_key = NULL,
	.destroy = &la_ohma_msg_destroy
};
