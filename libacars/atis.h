/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2025-2025 Yuuta Liang <yuuta@yuuta.moe>
 */

#ifndef LA_ATIS_H
#define LA_ATIS_H 1

#include <stdbool.h>
#include <stdint.h>
#include <libacars/libacars.h>              // la_type_descriptor, la_proto_node
#include <libacars/vstring.h>               // la_vstring

#ifdef __cplusplus
extern "C" {
#endif

#define ATIS_REQUEST_TYPE_ARRIVAL	'A'
#define ATIS_REQUEST_TYPE_DEPARTURE	'D'
#define ATIS_REQUEST_TYPE_ARRIVAL_AUTO	'C'
#define ATIS_REQUEST_TYPE_ENROUTE	'E'
#define ATIS_REQUEST_TYPE_TERMINATE	'T'

typedef struct {
	bool is_response; // 1 for uplink ATIS response; 0 for downlink ATIS request
	union {
		struct {
			// Max chars per line in response, in decimal digits, max 3 digits.
			char avionics_indicator[4];
			// Type as in ATIS_REQUEST_TYPE_*, max 1 char.
			char type[4];
			// Airport ICAO code, max 4 chars, but 3 chars also possible (e.g., YVR).
			char airport[8];
		} request;
		struct {
			// Airport ICAO code, max 4 chars.
			char airport[8];
			// ATIS type (e.g., ARR, DEP, ENR), max 3 chars.
			char type[4];
			// ATIS letter (e.g., A), max 1 char.
			char version[4];
			// UTC time (e.g., 1145), in hh:mm, 4 digits.
			char time[8];
			// ATIS content.
			char *content;
		} response;
	} data;

	bool err;
} la_atis_msg;

// atis.c
extern la_type_descriptor const la_DEF_atis_message;
la_proto_node *la_atis_parse(uint8_t const *buf, int len, la_msg_dir msg_dir);
void la_atis_format_text(la_vstring *vstr, void const *data, int indent);
void la_atis_format_json(la_vstring *vstr, void const *data);
void la_atis_destroy(void *data);
la_proto_node *la_proto_tree_find_atis(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_ATIS_H
