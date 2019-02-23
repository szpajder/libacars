/*  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>			// calloc
#include <string.h>			// strchr(), strdup(), strtok_r(), strlen
#include "config.h"
#ifdef WITH_ZLIB
#include <zlib.h>			// z_stream, inflateInit2(), inflate(), inflateEnd()
#endif
#include <libacars/macros.h>		// la_assert(), LA_UNLIKELY()
#include <libacars/libacars.h>		// la_proto_node
#include <libacars/util.h>		// la_dict, la_dict_search(), XCALLOC(), la_hexdump()
#include <libacars/crc.h>		// la_crc16_arinc(), la_crc32_arinc665()
#include <libacars/miam-core.h>

/**********************
 * Forward declarations
 **********************/

la_proto_node *la_miam_core_v1_data_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);
la_proto_node *la_miam_core_v1_ack_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);
la_proto_node *la_miam_core_v2_data_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);
la_proto_node *la_miam_core_v2_ack_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);
la_proto_node *la_miam_core_v1v2_alo_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);
la_proto_node *la_miam_core_v1v2_alr_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen);

/*************************************************
 * MIAM CORE v1/v2 common definitions and routines
 *************************************************/

typedef la_proto_node* (la_miam_core_pdu_parse_f)(uint8_t const *hdrbuf, int hdrlen,
			uint8_t const *bodybuf, int bodylen);

static la_dict const la_miam_core_v1_pdu_parser_table[] = {
	{ .id = LA_MIAM_CORE_PDU_DATA,	.val = &la_miam_core_v1_data_parse },
	{ .id = LA_MIAM_CORE_PDU_ACK,	.val = &la_miam_core_v1_ack_parse },
	{ .id = LA_MIAM_CORE_PDU_ALO,	.val = &la_miam_core_v1v2_alo_parse },
	{ .id = LA_MIAM_CORE_PDU_ALR,	.val = &la_miam_core_v1v2_alr_parse },
	{ .id = LA_MIAM_CORE_PDU_UNKNOWN, .val = NULL }
};
static la_dict const la_miam_core_v2_pdu_parser_table[] = {
	{ .id = LA_MIAM_CORE_PDU_DATA,	.val = &la_miam_core_v2_data_parse },
	{ .id = LA_MIAM_CORE_PDU_ACK,	.val = &la_miam_core_v2_ack_parse },
	{ .id = LA_MIAM_CORE_PDU_ALO,	.val = &la_miam_core_v1v2_alo_parse },
	{ .id = LA_MIAM_CORE_PDU_ALR,	.val = &la_miam_core_v1v2_alr_parse },
	{ .id = LA_MIAM_CORE_PDU_UNKNOWN, .val = NULL }
};

static la_dict const la_miam_core_v1v2_alo_alr_compression_names[] = {
	{ .id = 0, .val = "deflate" },
	{ .id = 0, .val = NULL }
};

static la_dict const la_miam_core_v1v2_alo_alr_network_names[] = {
	{ .id = 0, .val = "ACARS" },
	{ .id = 1, .val = "IP Middleware" },
	{ .id = 2, .val = "TCP/IP" },
	{ .id = 3, .val = "Satcom Data 3" },
	{ .id = 4, .val = "UDP" },
	{ .id = 5, .val = NULL }
};

typedef struct {
	uint8_t *buf;
	int len;
} la_base85_decode_result;

#ifdef WITH_ZLIB
typedef struct {
	uint8_t *buf;
	size_t buflen;
	bool success;
} la_inflate_result;

#define MAX_INFLATED_LEN (1<<20)

la_inflate_result la_inflate(uint8_t const *buf, int const in_len) {
	la_assert(buf != NULL);
	la_assert(in_len > 0);

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	la_inflate_result result;
	memset(&result, 0, sizeof(result));

	int ret = inflateInit2(&stream, -15);	// raw deflate with max window size
	if(ret != Z_OK) {
		la_debug_print("inflateInit failed: %d\n", ret);
		goto end;
	}
	stream.avail_in = (uInt)in_len;
	stream.next_in = (uint8_t *)buf;
	int chunk_len = 4 * in_len;
	int out_len = chunk_len;			// rough initial approximation
	uint8_t *outbuf = LA_XCALLOC(out_len, sizeof(uint8_t));
	stream.next_out = outbuf;
	stream.avail_out = out_len;

	while((ret = inflate(&stream, Z_FINISH)) == Z_BUF_ERROR) {
		la_debug_print("Z_BUF_ERROR, avail_in=%u avail_out=%u\n", stream.avail_in, stream.avail_out);
		if(stream.avail_out == 0) {
// Not enough output space
			int new_len = out_len + chunk_len;
			la_debug_print("outbuf grow: %d -> %d\n", out_len, new_len);
			if(new_len > MAX_INFLATED_LEN) {
// Do not go overboard with memory usage
				la_debug_print("new_len too large: %d > %d\n", new_len, MAX_INFLATED_LEN);
				break;
			}
			outbuf = LA_XREALLOC(outbuf, new_len * sizeof(uint8_t));
			stream.next_out = outbuf + out_len;
			stream.avail_out = chunk_len;
			out_len = new_len;
		} else if(stream.avail_in == 0) {
// Input stream is truncated - error out
			break;
		}
	}
	la_debug_print("zlib ret=%d avail_out=%u total_out=%lu\n", ret, stream.avail_out, stream.total_out);
// Make sure the buffer is larger than the result.
// We need space to append NULL terminator later for printing it.
	if(stream.avail_out == 0) {
		outbuf = LA_XREALLOC(outbuf, (out_len + 1) * sizeof(uint8_t));
	}
	result.buf = outbuf;
	result.buflen = stream.total_out;
	result.success = (ret == Z_STREAM_END ? true : false);
end:
	(void)inflateEnd(&stream);
	return result;
}
#endif

la_base85_decode_result la_base85_decode(char const *str, char const *end) {
	static uint32_t const base85[] = {
		85 * 85 * 85 * 85,
		85 * 85 * 85,
		85 * 85,
		85,
		1
	};
	la_assert(str != NULL);
	la_assert(str < end);

// A rough approximation of the output buffer size needed.
// Five base85 digits encode a single 32-bit word
// Allow four extra words for potential 'z' occurrences
// (all-zero word encoded with a single character)
	size_t outsize = ((end - str) / 5 + 4) * 4;
	la_debug_print("approx outsize: %zu\n", outsize);
	uint8_t *out = LA_XCALLOC(outsize, sizeof(uint8_t));

	char const *ptr = str;
	size_t inpos = 0, outpos = 0;
	union {
		uint32_t val;
		struct {
#ifdef IS_BIG_ENDIAN
			uint8_t b3,b2,b1,b0;
#else
			uint8_t b0,b1,b2,b3;
#endif
		} b;
	} v;

	while(end - ptr >= 5 || *ptr == 'z') {
		v.val = 0;
		if(*ptr == 'z') {
			ptr++;
			inpos++;
		} else {
			for(int i = 0; i < 5; i++, inpos++, ptr++) {
// shall we reject 'z' inside the 5-digit group?
				v.val += (*ptr - 0x21) * base85[i];
			}
		}
		if(LA_UNLIKELY(outpos + 5 >= outsize)) {
// grow the buffer by 25 percent, but not less than 5 elements
			outsize += 5;
			outsize += outsize / 4;
			la_debug_print("outbuf too small; resizing to %zu elements\n", outsize);
			out = LA_XREALLOC(out, outsize * sizeof(uint8_t));
		}
		out[outpos++] = v.b.b3;
		out[outpos++] = v.b.b2;
		out[outpos++] = v.b.b1;
		out[outpos++] = v.b.b0;
	}
	if(ptr != end) {
		la_debug_print("Input truncated, %td bytes left\n", end - ptr);
	}
	return (la_base85_decode_result){
		.buf = out,
		.len = outpos
	};
}

void la_isprintf_multiline_text(la_vstring * const vstr, int const indent, char const *txt) {
	la_assert(vstr != NULL);
	la_assert(indent >= 0);
	if(txt == NULL) {
		return;
	}
// have to work on a copy, because strtok modifies its first argument
	char *line = strdup(txt);
	char *ptr = line;
	char *next_line = NULL;
	while((ptr = strtok_r(ptr, "\n", &next_line)) != NULL) {
		LA_ISPRINTF(vstr, indent, "%s\n", ptr);
		ptr = next_line;
	}
	LA_XFREE(line);
}

static bool is_printable(uint8_t const *buf, uint32_t data_len) {
	if(buf == NULL || data_len == 0) {
		return false;
	}
	for(uint32_t i = 0; i < data_len; i++) {
		if((buf[i] >= 7  && buf[i] <= 13) ||
		   (buf[i] >= 32 && buf[i] <= 126)) {
			// noop
		} else {
			la_debug_print("false due to character %u at position %u\n", buf[i], i);
			return false;
		}
	}
	la_debug_print("%s\n", "true");
	return true;
}

// MIAM CORE v1/v2 common parsers

static la_proto_node *v1v2_alo_alr_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf,
int bodylen, la_miam_core_pdu_type const pdu_type) {
// -Wunused-parameter
	(void)bodybuf;
	(void)bodylen;

	la_assert(hdrbuf != NULL);
	la_assert(pdu_type == LA_MIAM_CORE_PDU_ALO || pdu_type == LA_MIAM_CORE_PDU_ALR);

	la_miam_core_v1v2_alo_alr_pdu *pdu = LA_XCALLOC(1, sizeof(la_miam_core_v1v2_alo_alr_pdu));
	la_proto_node *node = la_proto_node_new();
	if(pdu_type == LA_MIAM_CORE_PDU_ALO) {
		node->td = &la_DEF_miam_core_v1v2_alo_pdu;
	} else if(pdu_type == LA_MIAM_CORE_PDU_ALR) {
		node->td = &la_DEF_miam_core_v1v2_alr_pdu;
	}
	node->data = pdu;
	node->next = NULL;

	if(hdrlen < 13) {	// should be 16, but let's not be overly pedantic on unused octets
		la_debug_print("Header too short: %d < 16\n", hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}

	pdu->pdu_len = (hdrbuf[1] << 16) | (hdrbuf[2] << 8) | hdrbuf[3];
	hdrbuf += 4; hdrlen -= 4;

	memcpy(&pdu->aircraft_id, hdrbuf, 7);
	pdu->aircraft_id[7] = '\0';
	la_debug_print("len: %u aircraft_id: %s\n", pdu->pdu_len, pdu->aircraft_id);
	hdrbuf += 7; hdrlen -= 7;

	pdu->compression = hdrbuf[0];
	pdu->networks = hdrbuf[1];
	la_debug_print("compression: 0x%02x networks: 0x%02x\n",
		pdu->compression, pdu->networks);
end:
	return node;
}

la_proto_node *la_miam_core_v1v2_alo_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	return v1v2_alo_alr_parse(hdrbuf, hdrlen, bodybuf, bodylen, LA_MIAM_CORE_PDU_ALO);
}

la_proto_node *la_miam_core_v1v2_alr_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	return v1v2_alo_alr_parse(hdrbuf, hdrlen, bodybuf, bodylen, LA_MIAM_CORE_PDU_ALR);
}

la_proto_node *la_miam_core_pdu_parse(char const * const label, char const *txt, la_msg_dir const msg_dir) {
// -Wunused-parameter
	(void)label;
	(void)msg_dir;

	la_assert(txt != NULL);

// Determine if it's a MIAM CORE PDU - check body/header padding counts and look for header/body delimiter
	if(strlen(txt) < 3) {
		return NULL;
	}
	la_miam_core_pdu *pdu = NULL;

	char bpad = txt[0];	// valid values: 0, 1, 2, 3, -, .
	char hpad = txt[1];	// valid values: 0, 1, 2, 3
	txt += 2;
	if(!((bpad >= '0' && bpad <= '3') || bpad == '-' || bpad == '.')) {
		la_debug_print("Invalid body padding: %c\n", bpad);
		return NULL;
	}
	if(!(hpad >= '0' && hpad <= '3')) {
		la_debug_print("Invalid header padding: %c\n", hpad);
		return NULL;
	}
	hpad -= 0x30;	// get digit value: '1' -> 1
	char *delim = strchr(txt, '|');
	if(delim == NULL) {
		la_debug_print("%s", "Header/body delimiter not found\n");
		return NULL;
	}
	if(delim == txt) {
		la_debug_print("%s", "Empty header\n");
		return NULL;
	}
// Assume the initial part is the Header - try to decode it
	la_base85_decode_result header = la_base85_decode(txt, delim);
	if(header.buf == NULL || header.len < hpad) { // BASE85 decoder failed or result too short
		return NULL;
	}
	la_debug_print_buf_hex(header.buf, header.len, "%s:\n", "Decoded header");

// Decode message body, if exists and if it's encoded
	uint8_t *bodybuf = NULL;
	int bodylen = 0;
	la_base85_decode_result body = { .buf = NULL, .len = 0 };
	if(delim[1] != '\0') {
		if(bpad >= '0' && bpad <= '3') {
			char *end = strchr(delim, '\0');
			body = la_base85_decode(delim + 1, end);
			bpad -= 0x30;			// get digit value: '1' -> 1
			if(body.buf != NULL && body.len >= bpad) {
				body.len -= bpad;	// cut off padding bytes
			}
			bodybuf = body.buf;
			bodylen = body.len;
		} else if(bpad == '-') {
// Payload not encoded - just point at the start of it
			bodybuf = (uint8_t *)delim + 1;
			bodylen = strlen(delim + 1);
		}
	}

// From now on we assume that this is a MIAM frame and we return a non-NULL PDU with an
// error code if something goes wrong, so that a proper error message is printed to the output.
	uint8_t *b = header.buf;
	header.len -= hpad;				// cut off padding bytes

	uint8_t version = b[0] & 0xf;
	uint8_t pdu_type = (b[0] >> 4) & 0xf;
	la_debug_print("ver: %u pdu_type: %u\n", version, pdu_type);

	pdu = LA_XCALLOC(1, sizeof(la_miam_core_pdu));
	pdu->pdu_type = LA_MIAM_CORE_PDU_UNKNOWN;
	pdu->version = version;

	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_core_pdu;
	node->data = pdu;
	node->next = NULL;

	la_dict const *pdu_parse_dict = NULL;
	if(version == 1) {
		pdu_parse_dict = la_miam_core_v1_pdu_parser_table;
	} else if(version == 2) {
		pdu_parse_dict = la_miam_core_v2_pdu_parser_table;
	} else {
		la_debug_print("Unknown version %u\n", version);
		pdu->err |= LA_MIAM_ERR_HDR_PDU_VERSION_UNKNOWN;
		goto end;
	}

	la_miam_core_pdu_parse_f *pdu_parse = la_dict_search(pdu_parse_dict, pdu_type);
	if(pdu_parse == NULL) {
		la_debug_print("No parser for PDU type %u\n", pdu_type);
		pdu->err |= LA_MIAM_ERR_HDR_PDU_TYPE_UNKNOWN;
		goto end;
	}

	pdu->pdu_type = pdu_type;
	node->next = pdu_parse(header.buf, header.len, bodybuf, bodylen);

end:
	LA_XFREE(header.buf);
	LA_XFREE(body.buf);
	return node;
}

// MIAM CORE v1/v2 common formatters

void la_miam_errors_format_text(la_vstring * const vstr, uint32_t err, int indent) {
	static la_dict const la_miam_error_messages[] = {
		{ .id = LA_MIAM_ERR_SUCCESS,			.val = "No error" },
		{ .id = LA_MIAM_ERR_HDR_PDU_TYPE_UNKNOWN,	.val = "Unknown PDU type" },
		{ .id = LA_MIAM_ERR_HDR_PDU_VERSION_UNKNOWN,	.val = "Unsupported MIAM version" },
		{ .id = LA_MIAM_ERR_HDR_TRUNCATED,		.val = "Header truncated" },
		{ .id = LA_MIAM_ERR_HDR_APP_TYPE_UNKNOWN,	.val = "Unknown application type" },
		{ .id = LA_MIAM_ERR_BODY_TRUNCATED,		.val = "Message truncated" },
		{ .id = LA_MIAM_ERR_BODY_INFLATE_FAILED,	.val = "Decompression failed" },
		{ .id = LA_MIAM_ERR_BODY_COMPR_UNSUPPORTED,	.val = "Unsupported compression algorithm" },
		{ .id = LA_MIAM_ERR_BODY_CRC_FAILED,		.val = "CRC check failed" },
		{ .id = 0,					.val = NULL }
	};
	la_assert(vstr != NULL);
	la_assert(indent >= 0);
	for(uint32_t i = 0; i < 32; i++) {
		if((err & (1 << i)) != 0) {
			char *errmsg = la_dict_search(la_miam_error_messages, (int)(err & (1 << i)));
			if(errmsg != NULL) {
				LA_ISPRINTF(vstr, indent, "-- %s\n", errmsg);
			} else {
				LA_ISPRINTF(vstr, indent, "-- Unknown error (%u)\n", err);
			}
		}
	}
}

static void la_miam_bitmask_format_text(la_vstring * const vstr, uint8_t const bitmask,
la_dict const * const dict, int indent) {
	la_assert(vstr != NULL);
	la_assert(dict != NULL);
	la_assert(indent >= 0);

	for(int i = 0; i < 8; i++) {
		if((bitmask & (1 << i)) != 0) {
			char *name = la_dict_search(dict, i);
			if(name != NULL) {
				LA_ISPRINTF(vstr, indent, "%s\n", name);
			} else {
				LA_ISPRINTF(vstr, indent, "unknown (%u)\n", 1 << i);
			}
		}
	}
}

static void v1v2_alo_alr_format_text(la_vstring * const vstr, void const * const data, int indent,
la_miam_core_pdu_type const pdu_type) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	if(pdu_type != LA_MIAM_CORE_PDU_ALO && pdu_type != LA_MIAM_CORE_PDU_ALR) {
		return;
	}

	LA_CAST_PTR(pdu, la_miam_core_v1v2_alo_alr_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	LA_ISPRINTF(vstr, indent, "PDU Length: %u\n", pdu->pdu_len);
	LA_ISPRINTF(vstr, indent, "Aircraft ID: %s\n", pdu->aircraft_id);
	LA_ISPRINTF(vstr, indent, "Compressions %s:\n",
		(pdu_type == LA_MIAM_CORE_PDU_ALO ? "supported" : "selected"));
	la_miam_bitmask_format_text(vstr, pdu->compression,
		la_miam_core_v1v2_alo_alr_compression_names, indent + 1);
	LA_ISPRINTF(vstr, indent, "%s", "Networks supported:\n");
	la_miam_bitmask_format_text(vstr, pdu->networks,
		la_miam_core_v1v2_alo_alr_network_names, indent + 1);
// Not checking for body errors here, as there is no body in ALO and ALR PDUs
}

void la_miam_core_v1v2_alo_format_text(la_vstring * const vstr, void const * const data, int indent) {
	v1v2_alo_alr_format_text(vstr, data, indent, LA_MIAM_CORE_PDU_ALO);
}

void la_miam_core_v1v2_alr_format_text(la_vstring * const vstr, void const * const data, int indent) {
	v1v2_alo_alr_format_text(vstr, data, indent, LA_MIAM_CORE_PDU_ALR);
}

void la_miam_core_format_text(la_vstring * const vstr, void const * const data, int indent) {
	static char const * const la_miam_core_pdu_type_names[] = {
		[LA_MIAM_CORE_PDU_DATA] = "Data",
		[LA_MIAM_CORE_PDU_ACK] = "Ack",
		[LA_MIAM_CORE_PDU_ALO] = "Aloha",
		[LA_MIAM_CORE_PDU_ALR] = "Aloha Reply",
		[LA_MIAM_CORE_PDU_UNKNOWN] = "unknown PDU"
	};

	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(pdu, la_miam_core_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	la_assert(pdu->pdu_type <= LA_MIAM_CORE_PDU_TYPE_MAX);
	LA_ISPRINTF(vstr, indent, "MIAM CORE %s, version %u:\n",
		la_miam_core_pdu_type_names[pdu->pdu_type], pdu->version);
	indent++;
}

// MIAM CORE v1/v2 common type descriptors

la_type_descriptor const la_DEF_miam_core_pdu = {
	.format_text = la_miam_core_format_text,
	.destroy = NULL
};
la_type_descriptor const la_DEF_miam_core_v1v2_alo_pdu = {
	.format_text = la_miam_core_v1v2_alo_format_text,
	.destroy = NULL
};
la_type_descriptor const la_DEF_miam_core_v1v2_alr_pdu = {
	.format_text = la_miam_core_v1v2_alr_format_text,
	.destroy = NULL
};

/**********************************************
 * MIAM CORE version 1 definitions and routines
 **********************************************/

#define LA_MIAM_CORE_V1_CRC_LEN 4

#define LA_MIAM_CORE_V1_COMP_NONE 0x0
#define LA_MIAM_CORE_V1_COMP_DEFLATE 0x1

#define LA_MIAM_CORE_V1_ENC_ISO5 0x0
#define LA_MIAM_CORE_V1_ENC_BINARY 0x1

#define LA_MIAM_CORE_V1_APP_ACARS_2CHAR 0x0
#define LA_MIAM_CORE_V1_APP_ACARS_4CHAR 0x1
#define LA_MIAM_CORE_V1_APP_ACARS_6CHAR 0x2
#define LA_MIAM_CORE_V1_APP_NONACARS_6CHAR 0x3

// MIAM Core v1-specific parsers

la_proto_node *la_miam_core_v1_data_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	la_assert(hdrbuf != NULL);

	la_miam_core_v1_data_pdu *pdu = LA_XCALLOC(1, sizeof(la_miam_core_v1_data_pdu));
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_core_v1_data_pdu;
	node->data = pdu;
	node->next = NULL;

	if(hdrlen < 20) {
		la_debug_print("Header too short: %d < 20\n", hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}

	pdu->pdu_len = (hdrbuf[1] << 16) | (hdrbuf[2] << 8) | hdrbuf[3];
	if(pdu->pdu_len > (uint32_t)(hdrlen + bodylen)) {
		la_debug_print("PDU truncated: length from header: %d > pdu_len %d (%d+%d)\n",
			pdu->pdu_len, hdrlen + bodylen, hdrlen, bodylen);
		pdu->err |= LA_MIAM_ERR_BODY_TRUNCATED;
	}
	hdrbuf += 4; hdrlen -= 4;

	memcpy(&pdu->aircraft_id, hdrbuf, 7);
	pdu->aircraft_id[7] = '\0';
	la_debug_print("len: %u aircraft_id: %s\n", pdu->pdu_len, pdu->aircraft_id);
	hdrbuf += 7; hdrlen -= 7;

	uint8_t bytes_needed = 0;
	pdu->msg_num = (hdrbuf[0] >> 1) & 0x7f;
	pdu->ack_option = hdrbuf[0] & 0x1;
	la_debug_print("msg_num: %u ack_option: %u\n", pdu->msg_num, pdu->ack_option);
	hdrbuf++; hdrlen--;

	pdu->compression = ((hdrbuf[0] << 2) | ((hdrbuf[1] >> 6) & 0x3)) & 0x7;
	pdu->encoding = (hdrbuf[1] >> 4) & 0x3;
	pdu->app_type = hdrbuf[1] & 0xf;
	la_debug_print("compression: 0x%x encoding: 0x%x app_type: 0x%x\n",
		pdu->compression, pdu->encoding, pdu->app_type);
	hdrbuf += 2; hdrlen -= 2;

	uint8_t app_id_len = 0;
	if(pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_2CHAR) {
		app_id_len = 2;
	} else if(pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_4CHAR) {
		app_id_len = 4;
	} else if(pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_6CHAR ||
		  pdu->app_type == LA_MIAM_CORE_V1_APP_NONACARS_6CHAR) {
		app_id_len = 6;
	} else {
		la_debug_print("Unknown app type 0x%u\n", pdu->app_type);
		pdu->err |= LA_MIAM_ERR_HDR_APP_TYPE_UNKNOWN;
		goto end;
	}
	bytes_needed = app_id_len + LA_MIAM_CORE_V1_CRC_LEN;
	if(hdrlen < bytes_needed) {
		la_debug_print("Header too short for app_type 0x%x: "
			"need %u more bytes but only %u available\n",
			pdu->app_type, bytes_needed, hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}
	memcpy(pdu->app_id, hdrbuf, app_id_len);
	pdu->app_id[6] = '\0';
	la_debug_print("app_id: '%s'\n", pdu->app_id);
	hdrbuf += app_id_len; hdrlen -= app_id_len;

	pdu->crc = (hdrbuf[0] << 24) | (hdrbuf[1] << 16) | (hdrbuf[2] << 8) | hdrbuf[3];
	hdrbuf += LA_MIAM_CORE_V1_CRC_LEN; hdrlen -= LA_MIAM_CORE_V1_CRC_LEN;
	if(hdrlen > 0) {
		la_debug_print("Warning: %u bytes left after MIAM header\n", hdrlen);
	}
	if(bodybuf != NULL && bodylen > 0) {
#ifdef WITH_ZLIB
		if(pdu->compression == LA_MIAM_CORE_V1_COMP_DEFLATE) {
			la_inflate_result inflated = la_inflate(bodybuf, bodylen);
			la_debug_print_buf_hex(inflated.buf, (int)inflated.buflen, "%s", "Decompressed content:\n");
// If it's text, it needs a NULL terminator.
// If it's not text, it doesn't hurt either. The buffer is larger than len anyway.
			inflated.buf[inflated.buflen] = '\0';
			pdu->data = inflated.buf;
			pdu->data_len = inflated.buflen;
			if(inflated.success == false) {
				pdu->err |= LA_MIAM_ERR_BODY_INFLATE_FAILED;
			}
		} else
#endif
		if(pdu->compression == LA_MIAM_CORE_V1_COMP_NONE) {
			uint8_t *pdu_data = LA_XCALLOC(bodylen + 1, sizeof(uint8_t));
			memcpy(pdu_data, bodybuf, bodylen);
			pdu_data[bodylen] = '\0';
			pdu->data = pdu_data;
			pdu->data_len = bodylen;
		} else {
			pdu->err |= LA_MIAM_ERR_BODY_COMPR_UNSUPPORTED;
		}
		uint32_t crc_check = la_crc32_arinc665(pdu->data, pdu->data_len, 0xFFFFFFFFu);
		crc_check = ~crc_check;
		la_debug_print("crc: %08x crc_check: %08x\n", pdu->crc, crc_check);
		if(crc_check != pdu->crc) {
			pdu->err |= LA_MIAM_ERR_BODY_CRC_FAILED;
		}
	}
end:
	return node;
}

la_proto_node *la_miam_core_v1_ack_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	la_assert(hdrbuf != NULL);
// -Wunused-parameter - no body present in ack PDU
	(void)bodybuf;
	(void)bodylen;

	la_miam_core_v1_ack_pdu *pdu = LA_XCALLOC(1, sizeof(la_miam_core_v1_ack_pdu));
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_core_v1_ack_pdu;
	node->data = pdu;
	node->next = NULL;

	if(hdrlen < 20) {
		la_debug_print("Header too short: %d < 20\n", hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}

	pdu->pdu_len = (hdrbuf[1] << 16) | (hdrbuf[2] << 8) | hdrbuf[3];
	hdrbuf += 4; hdrlen -=4;

	memcpy(&pdu->aircraft_id, hdrbuf, 7);
	pdu->aircraft_id[7] = '\0';
	la_debug_print("len: %u aircraft_id: %s\n", pdu->pdu_len, pdu->aircraft_id);
	hdrbuf += 7; hdrlen -= 7;

	pdu->msg_ack_num = (hdrbuf[0] >> 1) & 0x7f;
	hdrbuf++; hdrlen--;

	pdu->ack_xfer_result = (hdrbuf[0] >> 4) & 0xf;
	la_debug_print("msg_ack_num: %u ack_xfer_result: 0x%x\n", pdu->msg_ack_num, pdu->ack_xfer_result);
	hdrbuf += 4; hdrlen -= 4;

	memcpy(pdu->crc, hdrbuf, LA_MIAM_CORE_V1_CRC_LEN);
	hdrbuf += LA_MIAM_CORE_V1_CRC_LEN; hdrlen -= LA_MIAM_CORE_V1_CRC_LEN;
	if(hdrlen > 0) {
		la_debug_print("Warning: %u bytes left after MIAM header\n", hdrlen);
	}
end:
	return node;
}

// MIAM Core v1-specific formatters

void la_miam_core_v1_data_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	static la_dict const v1_compression_names[] = {
		{ .id = LA_MIAM_CORE_V1_COMP_NONE,	.val = "none" },
		{ .id = LA_MIAM_CORE_V1_COMP_DEFLATE,	.val = "deflate" },
		{ .id = 0x0,				.val = NULL }
	};

	static la_dict const v1_encoding_names[] = {
		{ .id = LA_MIAM_CORE_V1_ENC_ISO5,	.val = "ISO #5" },
		{ .id = LA_MIAM_CORE_V1_ENC_BINARY,	.val = "binary" },
		{ .id = 0x0,				.val = NULL }
	};

	LA_CAST_PTR(pdu, la_miam_core_v1_data_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	LA_ISPRINTF(vstr, indent, "PDU Length: %u\n", pdu->pdu_len);
	LA_ISPRINTF(vstr, indent, "Aircraft ID: %s\n", pdu->aircraft_id);
	LA_ISPRINTF(vstr, indent, "Msg num: %u\n", pdu->msg_num);
	LA_ISPRINTF(vstr, indent, "ACK: %srequired\n",
		(pdu->ack_option == 1 ? "" : "not "));

	char *name = la_dict_search(v1_compression_names, pdu->compression);
	if(name != NULL) {
		LA_ISPRINTF(vstr, indent, "Compression: %s\n", name);
	} else {
		LA_ISPRINTF(vstr, indent, "Compression: unknown (%u)\n", pdu->compression);
	}

	name = la_dict_search(v1_encoding_names, pdu->encoding);
	if(name != NULL) {
		LA_ISPRINTF(vstr, indent, "Encoding: %s\n", name);
	} else {
		LA_ISPRINTF(vstr, indent, "Encoding: unknown (%u)\n", pdu->encoding);
	}

	switch(pdu->app_type) {
	case LA_MIAM_CORE_V1_APP_ACARS_2CHAR:
	case LA_MIAM_CORE_V1_APP_ACARS_4CHAR:
	case LA_MIAM_CORE_V1_APP_ACARS_6CHAR:
		LA_ISPRINTF(vstr, indent, "%s", "ACARS:\n");
		indent++;
		LA_ISPRINTF(vstr, indent, "Label: %c%c",
			pdu->app_id[0], pdu->app_id[1]);

		if(pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_4CHAR ||
			pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_6CHAR) {
			la_vstring_append_sprintf(vstr, " Sublabel: %c%c",
				pdu->app_id[2], pdu->app_id[3]);
		}

		if(pdu->app_type == LA_MIAM_CORE_V1_APP_ACARS_6CHAR) {
			la_vstring_append_sprintf(vstr, " MFI: %c%c",
				pdu->app_id[4], pdu->app_id[5]);
		}

		la_vstring_append_sprintf(vstr, "%s", "\n");

		break;
	case LA_MIAM_CORE_V1_APP_NONACARS_6CHAR:
		LA_ISPRINTF(vstr, indent, "%s", "Non-ACARS payload:\n");
		indent++;
		LA_ISPRINTF(vstr, indent, "Application ID: %s\n", pdu->app_id);
		break;
	default:
		break;
	}

	LA_ISPRINTF(vstr, indent, "%s", "Message:\n");
	indent++;
	if(pdu->data != NULL) {
// Don't trust pdu->encoding - if the payload is printable, then print it as text.
// Otherwise print a hexdump.
		if(is_printable(pdu->data, pdu->data_len)) {
// Parser has appended '\0' at the end, so it's safe to print it directly
			la_isprintf_multiline_text(vstr, indent, (char *)pdu->data);
		} else {
			char *hexdump = la_hexdump((uint8_t *)pdu->data, pdu->data_len);
			la_isprintf_multiline_text(vstr, indent, hexdump);
			LA_XFREE(hexdump);
		}
	}

	if(pdu->err & LA_MIAM_ERR_BODY) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_BODY, indent);
		return;
	}
}

void la_miam_core_v1_ack_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	static la_dict const v1_ack_xfer_result_names[] = {
		{ .id = 0x0, .val = "ack" },
		{ .id = 0x1, .val = "nack" },
		{ .id = 0x2, .val = "time_expiry" },
		{ .id = 0x3, .val = "peer_abort" },
		{ .id = 0x4, .val = "local_abort" },
		{ .id = 0x0, .val = NULL }
	};

	LA_CAST_PTR(pdu, la_miam_core_v1_ack_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	LA_ISPRINTF(vstr, indent, "PDU Length: %u\n", pdu->pdu_len);
	LA_ISPRINTF(vstr, indent, "Aircraft ID: %s\n", pdu->aircraft_id);
	LA_ISPRINTF(vstr, indent, "Msg ACK num: %u\n", pdu->msg_ack_num);
	char *xfer_result_name = la_dict_search(v1_ack_xfer_result_names, pdu->ack_xfer_result);
	if(xfer_result_name != NULL) {
		LA_ISPRINTF(vstr, indent, "Transfer result: %s\n", xfer_result_name);
	} else {
		LA_ISPRINTF(vstr, indent, "Transfer result: unknown (%u)\n", pdu->ack_xfer_result);
	}
// Not checking for body errors here, as there is no body in an ack PDU
}

// MIAM Core v1-specific destructors

void la_miam_core_v1_data_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	LA_CAST_PTR(pdu, la_miam_core_v1_data_pdu *, data);
	LA_XFREE(pdu->data);
	LA_XFREE(pdu);
}

// MIAM Core v1-specific type descriptors

la_type_descriptor const la_DEF_miam_core_v1_data_pdu = {
	.format_text = la_miam_core_v1_data_format_text,
	.destroy = la_miam_core_v1_data_destroy
};
la_type_descriptor const la_DEF_miam_core_v1_ack_pdu = {
	.format_text = la_miam_core_v1_ack_format_text,
	.destroy = NULL
};

/**********************************************
 * MIAM CORE version 2 definitions and routines
 **********************************************/

#define LA_MIAM_CORE_V2_CRC_LEN 2

#define LA_MIAM_CORE_V2_COMP_NONE 0x0
#define LA_MIAM_CORE_V2_COMP_DEFLATE 0x1

#define LA_MIAM_CORE_V2_ENC_ISO5 0x0
#define LA_MIAM_CORE_V2_ENC_BINARY 0x1

#define LA_MIAM_CORE_V2_APP_ACARS_2CHAR 0x0
#define LA_MIAM_CORE_V2_APP_ACARS_4CHAR 0x1
#define LA_MIAM_CORE_V2_APP_ACARS_6CHAR 0x2
#define LA_MIAM_CORE_V2_APP_NONACARS_6CHAR 0x3
// No need to have these macros for other non-ACARS V2-specific app types,
// as the app ID length can be derived from the value of the app type field.

// MIAM CORE v2-specific parsers

la_proto_node *la_miam_core_v2_data_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	la_assert(hdrbuf != NULL);

	la_miam_core_v2_data_pdu *pdu = LA_XCALLOC(1, sizeof(la_miam_core_v2_data_pdu));
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_core_v2_data_pdu;
	node->data = pdu;
	node->next = NULL;

	if(hdrlen < 7) {
		la_debug_print("Header too short: %d < 7\n", hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}
	hdrbuf++; hdrlen--;

	uint8_t bytes_needed = 0;
	pdu->msg_num = (hdrbuf[0] >> 1) & 0x7f;
	pdu->ack_option = hdrbuf[0] & 0x1;
	la_debug_print("msg_num: %u ack_option: %u\n", pdu->msg_num, pdu->ack_option);
	hdrbuf++; hdrlen--;

	pdu->compression = ((hdrbuf[0] << 2) | ((hdrbuf[1] >> 6) & 0x3)) & 0x7;
	pdu->encoding = (hdrbuf[1] >> 4) & 0x3;
	pdu->app_type = hdrbuf[1] & 0xf;
	la_debug_print("compression: 0x%x encoding: 0x%x app_type: 0x%x\n",
		pdu->compression, pdu->encoding, pdu->app_type);
	hdrbuf += 2; hdrlen -= 2;

	uint8_t app_id_len = 0;
	if(pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_2CHAR) {
		app_id_len = 2;
	} else if(pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_4CHAR) {
		app_id_len = 4;
	} else if(pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_6CHAR ||
		  pdu->app_type == LA_MIAM_CORE_V2_APP_NONACARS_6CHAR) {
		app_id_len = 6;
	} else if((pdu->app_type & 0x8) != 0 && pdu->app_type != 0xd /* reserved value */) {
		app_id_len = (pdu->app_type & 0x7) + 1;
	} else {
		la_debug_print("Unknown app type 0x%u\n", pdu->app_type);
		pdu->err |= LA_MIAM_ERR_HDR_APP_TYPE_UNKNOWN;
		goto end;
	}
	bytes_needed = app_id_len + LA_MIAM_CORE_V2_CRC_LEN;
	if(hdrlen < bytes_needed) {
		la_debug_print("Header too short for app_type 0x%x: "
			"need %u more bytes but only %u available\n",
			pdu->app_type, bytes_needed, hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}
	memcpy(pdu->app_id, hdrbuf, app_id_len);
	pdu->app_id[6] = '\0';
	la_debug_print("app_id: '%s'\n", pdu->app_id);
	hdrbuf += app_id_len; hdrlen -= app_id_len;

	pdu->crc = (hdrbuf[0] << 8) | hdrbuf[1];
	hdrbuf += LA_MIAM_CORE_V2_CRC_LEN; hdrlen -= LA_MIAM_CORE_V2_CRC_LEN;
	if(hdrlen > 0) {
		la_debug_print("Warning: %u bytes left after MIAM header\n", hdrlen);
	}
	if(bodybuf != NULL && bodylen > 0) {
#ifdef WITH_ZLIB
		if(pdu->compression == LA_MIAM_CORE_V2_COMP_DEFLATE) {
			la_inflate_result inflated = la_inflate(bodybuf, bodylen);
			la_debug_print_buf_hex(inflated.buf, (int)inflated.buflen, "%s", "Decompressed content:\n");
// If it's text, it needs a NULL terminator.
// If it's not text, it doesn't hurt either. The buffer is larger than len anyway.
			inflated.buf[inflated.buflen] = '\0';
			pdu->data = inflated.buf;
			pdu->data_len = inflated.buflen;
			if(inflated.success == false) {
				pdu->err |= LA_MIAM_ERR_BODY_INFLATE_FAILED;
			}
		} else
#endif
		if(pdu->compression == LA_MIAM_CORE_V2_COMP_NONE) {
			uint8_t *pdu_data = LA_XCALLOC(bodylen + 1, sizeof(uint8_t));
			memcpy(pdu_data, bodybuf, bodylen);
			pdu_data[bodylen] = '\0';
			pdu->data = pdu_data;
			pdu->data_len = bodylen;
		} else {
			pdu->err |= LA_MIAM_ERR_BODY_COMPR_UNSUPPORTED;
		}
		uint16_t crc_check = la_crc16_arinc(pdu->data, pdu->data_len, 0xFFFFu);
		la_debug_print("crc: %04x crc_check: %04x\n", pdu->crc, crc_check);
		if(crc_check != pdu->crc) {
			pdu->err |= LA_MIAM_ERR_BODY_CRC_FAILED;
		}
	}
end:
	return node;
}


la_proto_node *la_miam_core_v2_ack_parse(uint8_t const *hdrbuf, int hdrlen, uint8_t const *bodybuf, int bodylen) {
	la_assert(hdrbuf != NULL);
// -Wunused-parameter
	(void)bodybuf;
	(void)bodylen;

	la_miam_core_v2_ack_pdu *pdu = LA_XCALLOC(1, sizeof(la_miam_core_v2_ack_pdu));
	la_proto_node *node = la_proto_node_new();
	node->td = &la_DEF_miam_core_v2_ack_pdu;
	node->data = pdu;
	node->next = NULL;

	if(hdrlen < 8) {
		la_debug_print("Header too short: %d < 8\n", hdrlen);
		pdu->err |= LA_MIAM_ERR_HDR_TRUNCATED;
		goto end;
	}

	hdrbuf++; hdrlen--;

	pdu->msg_ack_num = (hdrbuf[0] >> 1) & 0x7f;
	pdu->ack_xfer_result = ((hdrbuf[0] << 3) | (hdrbuf[1] >> 5)) & 0xf;
	la_debug_print("msg_ack_num: %u ack_xfer_result: 0x%x\n", pdu->msg_ack_num, pdu->ack_xfer_result);
	hdrbuf += 3; hdrlen -= 3;

	memcpy(pdu->crc, hdrbuf, LA_MIAM_CORE_V2_CRC_LEN);
	hdrbuf += LA_MIAM_CORE_V2_CRC_LEN; hdrlen -= LA_MIAM_CORE_V2_CRC_LEN;

	hdrbuf += 2; hdrlen -= 2;	// jump over 2 spare octets
	if(hdrlen > 0) {
		la_debug_print("Warning: %u bytes left after MIAM header\n", hdrlen);
	}
end:
	return node;
}

// MIAM CORE v2-specific formatters

void la_miam_core_v2_data_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	static la_dict const v2_compression_names[] = {
		{ .id = LA_MIAM_CORE_V2_COMP_NONE,	.val = "none" },
		{ .id = LA_MIAM_CORE_V2_COMP_DEFLATE,	.val = "deflate" },
		{ .id = 0x0,				.val = NULL }
	};

	static la_dict const v2_encoding_names[] = {
		{ .id = LA_MIAM_CORE_V2_ENC_ISO5,	.val = "ISO #5" },
		{ .id = LA_MIAM_CORE_V2_ENC_BINARY,	.val = "binary" },
		{ .id = 0x0,				.val = NULL }
	};

	LA_CAST_PTR(pdu, la_miam_core_v2_data_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	LA_ISPRINTF(vstr, indent, "Msg num: %u\n", pdu->msg_num);
	LA_ISPRINTF(vstr, indent, "ACK: %srequired\n",
		(pdu->ack_option == 1 ? "" : "not "));

	char *name = la_dict_search(v2_compression_names, pdu->compression);
	if(name != NULL) {
		LA_ISPRINTF(vstr, indent, "Compression: %s\n", name);
	} else {
		LA_ISPRINTF(vstr, indent, "Compression: unknown (%u)\n", pdu->compression);
	}

	name = la_dict_search(v2_encoding_names, pdu->encoding);
	if(name != NULL) {
		LA_ISPRINTF(vstr, indent, "Encoding: %s\n", name);
	} else {
		LA_ISPRINTF(vstr, indent, "Encoding: unknown (%u)\n", pdu->encoding);
	}

	switch(pdu->app_type) {
	case LA_MIAM_CORE_V2_APP_ACARS_2CHAR:
	case LA_MIAM_CORE_V2_APP_ACARS_4CHAR:
	case LA_MIAM_CORE_V2_APP_ACARS_6CHAR:
		LA_ISPRINTF(vstr, indent, "%s", "ACARS:\n");
		indent++;
		LA_ISPRINTF(vstr, indent, "Label: %c%c",
			pdu->app_id[0], pdu->app_id[1]);

		if(pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_4CHAR ||
			pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_6CHAR) {
			la_vstring_append_sprintf(vstr, " Sublabel: %c%c",
				pdu->app_id[2], pdu->app_id[3]);
		}

		if(pdu->app_type == LA_MIAM_CORE_V2_APP_ACARS_6CHAR) {
			la_vstring_append_sprintf(vstr, " MFI: %c%c",
				pdu->app_id[4], pdu->app_id[5]);
		}

		la_vstring_append_sprintf(vstr, "%s", "\n");
		break;
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
	case 0xd:
// reserved for future use
		break;
	case LA_MIAM_CORE_V2_APP_NONACARS_6CHAR:
	default:	// including 0x8-0x15
		LA_ISPRINTF(vstr, indent, "%s", "Non-ACARS payload:\n");
		indent++;
		LA_ISPRINTF(vstr, indent, "Application ID: %s\n", pdu->app_id);
		break;
	}

	LA_ISPRINTF(vstr, indent, "%s", "Message:\n");
	indent++;
	if(pdu->data != NULL) {
// Don't trust pdu->encoding - if the payload is printable, then print it as text.
// Otherwise print a hexdump.
		if(is_printable(pdu->data, pdu->data_len)) {
// Parser has appended '\0' at the end, so it's safe to print it directly
			la_isprintf_multiline_text(vstr, indent, (char *)pdu->data);
		} else {
			char *hexdump = la_hexdump((uint8_t *)pdu->data, pdu->data_len);
			la_isprintf_multiline_text(vstr, indent, hexdump);
			LA_XFREE(hexdump);
		}
	}

	if(pdu->err & LA_MIAM_ERR_BODY) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_BODY, indent);
		return;
	}
}

void la_miam_core_v2_ack_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr != NULL);
	la_assert(data != NULL);
	la_assert(indent >= 0);

	static la_dict const v2_ack_xfer_result_names[] = {
		{ .id = 0x0, .val = "ack" },
		{ .id = 0x1, .val = "nack" },
		{ .id = 0x2, .val = "time_expiry" },
		{ .id = 0x3, .val = "peer_abort" },
		{ .id = 0x4, .val = "local_abort" },
		{ .id = 0x5, .val = "miam_version_not_supported" },
		{ .id = 0x0, .val = NULL }
	};

	LA_CAST_PTR(pdu, la_miam_core_v2_ack_pdu *, data);
	if(pdu->err & LA_MIAM_ERR_HDR) {
		la_miam_errors_format_text(vstr, pdu->err & LA_MIAM_ERR_HDR, indent);
		return;
	}
	LA_ISPRINTF(vstr, indent, "Msg ACK num: %u\n", pdu->msg_ack_num);
	char *xfer_result_name = la_dict_search(v2_ack_xfer_result_names, pdu->ack_xfer_result);
	if(xfer_result_name != NULL) {
		LA_ISPRINTF(vstr, indent, "Transfer result: %s\n", xfer_result_name);
	} else {
		LA_ISPRINTF(vstr, indent, "Transfer result: unknown (%u)\n", pdu->ack_xfer_result);
	}
// Not checking for body errors here, as there is no body in an ack PDU
}

// MIAM CORE v2-specific destructors

void la_miam_core_v2_data_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	LA_CAST_PTR(pdu, la_miam_core_v2_data_pdu *, data);
	LA_XFREE(pdu->data);
	LA_XFREE(pdu);
}

// MIAM CORE v2-specific type descriptors

la_type_descriptor const la_DEF_miam_core_v2_data_pdu = {
	.format_text = la_miam_core_v2_data_format_text,
	.destroy = &la_miam_core_v2_data_destroy
};
la_type_descriptor const la_DEF_miam_core_v2_ack_pdu = {
	.format_text = la_miam_core_v2_ack_format_text,
	.destroy = NULL
};
