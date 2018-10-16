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

#define _GNU_SOURCE
#include <stdint.h>
#include <string.h>		// memcpy(), strlen(), strcat(), strdup()
#include <math.h>
#include "macros.h"		// la_debug_print, LA_CAST_PTR
#include "bitstream.h"
#include "libacars.h"		// la_msg_dir
#include "arinc.h"		// la_arinc_imi
#include "list.h"		// la_list_*
#include "util.h"		// la_dict, la_dict_search(), LA_XCALLOC, LA_XFREE
#include "vstring.h"		// la_vstring, la_vstring_append_sprintf()
#include "adsc.h"

// FIXME: replace with la_vstring_append_sprintf
#include <stdio.h>
#include <stdarg.h>
int xasprintf(const char *file, const int line, const char *func, char **strp, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	int r = vasprintf(strp, fmt, ap);
	va_end(ap);
	if(r == -1) {
		fprintf(stderr, "%s:%d: %s(): vasprintf() failed (out of memory?)\n",
			file, line, func);
	}
	return r;
}
#define XASPRINTF(failcode, strp, fmt, ...) \
	do { \
		if(xasprintf(__FILE__, __LINE__, __func__, (strp), (fmt), __VA_ARGS__) == -1) { \
			return (failcode); \
		} \
	} while(0);

static double adsc_parse_coordinate(uint32_t c) {
// extend the 21-bit signed field to 32-bit signed int
	struct { signed int coord:21; } s;
	int r = s.coord = (int)c;
	la_debug_print("r=%d\n", r);
// Field range is -180 to 180 degrees.
// MSB weight is defined to have a weight of 90 degrees.
// LSB weight is therefore 90/(2^19).
// First, calculate maximum value of the field:
	double result = (180.0-90.0/pow(2, 19));
// Then multiply it by r/max_r
	result *= (double)r;
	result /= (double)0xfffff;
	la_debug_print("result: %f\n", result);
	return result;
}

static int adsc_parse_altitude(uint32_t a) {
	struct { signed int alt:16; } s;
	int result = s.alt = (int)a;
	result *= 4;
	la_debug_print("result: %d\n", result);
	return result;
}

static double adsc_parse_timestamp(uint32_t t) {
	double result = (double)t * 0.125;
	la_debug_print("result: %f\n", result);
	return result;
}

static double adsc_parse_speed(uint32_t s) {
	double result = (double)s / 2.0;
	la_debug_print("result: %f\n", result);
	return result;
}

static int adsc_parse_vert_speed(uint32_t vs) {
	struct { signed int vs:12; } s;
	int result = s.vs = (int)vs;
	result *= 16;
	la_debug_print("result: %d\n", result);
	return result;
}

static double adsc_parse_distance(uint32_t d) {
	double result = (double)d / 8.0;
	la_debug_print("result: %f\n", result);
	return result;
}

double adsc_parse_heading(uint32_t h) {
// Heading/track format is the same as latitude/longitude
// except that:
// - the field is 12-bit long (including sign bit)
// - LSB weight is 90/(2^10).
// FIXME: reduce this to a common function
	struct { signed int hdg:12; } s;
	int r = s.hdg = (int)h;
	la_debug_print("r=%d\n", r);
	double result = (180.0-90.0/pow(2, 10));
	result *= (double)r;
	result /= (double)0x7ff;
	if(result < 0.0)
		result += 360.0;
	la_debug_print("result: %f\n", result);
	return result;
}

double adsc_parse_wind_dir(uint32_t w) {
// Wind direction format is the same as latitude/longitude
// except that:
// - the field is 9-bit long (including sign bit)
// - LSB weight is 90/(2^7).
	struct { signed int dir:9; } s;
	int r = s.dir = (int)w;
	la_debug_print("r=%d\n", r);
	double result = (180.0-90.0/pow(2, 7));
	result *= (double)r;
	result /= (double)0xff;
	if(result < 0.0)
		result += 360.0;
	la_debug_print("result: %f\n", result);
	return result;
}

double adsc_parse_temperature(uint32_t t) {
	struct { signed int temp:12; } s;
	int r = s.temp = (int)t;
	la_debug_print("r=%d\n", r);
	double result = (512.0-256.0/pow(2, 10));
	result *= (double)r;
	result /= (double)0x7ff;
	la_debug_print("result: %f\n", result);
	return result;
}

#define ADSC_CHECK_LEN(t, l, m) if((l) < (m)) { \
	la_debug_print("Truncated tag %u: len: %u < %u\n", (t), (l), (m)); \
		return -1; \
	}
#define ADSC_PARSER_PROTOTYPE(x) static int x(void *dest, uint8_t *buf, uint32_t len)
#define ADSC_FORMATTER_PROTOTYPE(x) static char *x(char const * const label, void const * const data)

static int adsc_parse_tag(adsc_tag_t *t, la_dict const *tag_descriptor_table, uint8_t *buf, uint32_t len);

/***************************************************
 * Prototypes of functions used in descriptor tables
 ***************************************************/

ADSC_PARSER_PROTOTYPE(adsc_parse_uint8_t);
ADSC_PARSER_PROTOTYPE(adsc_parse_contract_request);
ADSC_PARSER_PROTOTYPE(adsc_parse_reporting_interval);
ADSC_PARSER_PROTOTYPE(adsc_parse_lat_dev_change);
ADSC_PARSER_PROTOTYPE(adsc_parse_vspd_change);
ADSC_PARSER_PROTOTYPE(adsc_parse_alt_range);
ADSC_PARSER_PROTOTYPE(adsc_parse_acft_intent_group);
ADSC_PARSER_PROTOTYPE(adsc_parse_nack);
ADSC_PARSER_PROTOTYPE(adsc_parse_noncomp_notify);
ADSC_PARSER_PROTOTYPE(adsc_parse_basic_report);
ADSC_PARSER_PROTOTYPE(adsc_parse_flight_id);
ADSC_PARSER_PROTOTYPE(adsc_parse_predicted_route);
ADSC_PARSER_PROTOTYPE(adsc_parse_earth_air_ref);
ADSC_PARSER_PROTOTYPE(adsc_parse_intermediate_projection);
ADSC_PARSER_PROTOTYPE(adsc_parse_fixed_projection);
ADSC_PARSER_PROTOTYPE(adsc_parse_meteo);
ADSC_PARSER_PROTOTYPE(adsc_parse_airframe_id);

ADSC_FORMATTER_PROTOTYPE(adsc_format_empty_tag);
ADSC_FORMATTER_PROTOTYPE(adsc_format_tag_with_contract_number);
ADSC_FORMATTER_PROTOTYPE(adsc_format_contract_request);
ADSC_FORMATTER_PROTOTYPE(adsc_format_reporting_interval);
ADSC_FORMATTER_PROTOTYPE(adsc_format_lat_dev_change);
ADSC_FORMATTER_PROTOTYPE(adsc_format_vspd_change);
ADSC_FORMATTER_PROTOTYPE(adsc_format_alt_range);
ADSC_FORMATTER_PROTOTYPE(adsc_format_acft_intent_group);
ADSC_FORMATTER_PROTOTYPE(adsc_format_modulus);
ADSC_FORMATTER_PROTOTYPE(adsc_format_nack);
ADSC_FORMATTER_PROTOTYPE(adsc_format_dis_reason_code);
ADSC_FORMATTER_PROTOTYPE(adsc_format_noncomp_notify);
ADSC_FORMATTER_PROTOTYPE(adsc_format_basic_report);
ADSC_FORMATTER_PROTOTYPE(adsc_format_flight_id);
ADSC_FORMATTER_PROTOTYPE(adsc_format_predicted_route);
ADSC_FORMATTER_PROTOTYPE(adsc_format_earth_ref);
ADSC_FORMATTER_PROTOTYPE(adsc_format_air_ref);
ADSC_FORMATTER_PROTOTYPE(adsc_format_intermediate_projection);
ADSC_FORMATTER_PROTOTYPE(adsc_format_fixed_projection);
ADSC_FORMATTER_PROTOTYPE(adsc_format_meteo);
ADSC_FORMATTER_PROTOTYPE(adsc_format_airframe_id);

static void adsc_destroy_contract_request(void *data);
static void adsc_destroy_noncomp_notify(void *data);

/*****************
 * Downlink tags
 *****************/

static la_dict const adsc_downlink_tag_descriptor_table[] = {
	{
		.id = 3,
		.val = &(adsc_type_descriptor_t){
			.label = "Acknowledgement",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_tag_with_contract_number,
			.destroy = NULL
		}
	},
	{
		.id = 4,
		.val = &(adsc_type_descriptor_t){
			.label = "Negative acknowledgement",
			.parse = &adsc_parse_nack,
			.format = &adsc_format_nack,
			.destroy = NULL
		}
	},
	{
		.id = 5,
		.val = &(adsc_type_descriptor_t){
			.label = "Noncompliance notification",
			.parse = &adsc_parse_noncomp_notify,
			.format = &adsc_format_noncomp_notify,
			.destroy = &adsc_destroy_noncomp_notify
		}
	},
	{
		.id = 6,
		.val = &(adsc_type_descriptor_t){
			.label = "Cancel emergency mode",
			.parse = NULL,
			.format = &adsc_format_empty_tag,
			.destroy = NULL
		}
	},
	{
		.id = 7,
		.val = &(adsc_type_descriptor_t){
			.label = "Basic report",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 9,
		.val = &(adsc_type_descriptor_t){
			.label = "Emergency basic report",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 10,
		.val = &(adsc_type_descriptor_t){
			.label = "Lateral deviation change event",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 12,
		.val = &(adsc_type_descriptor_t){
			.label = "Flight ID data",
			.parse = &adsc_parse_flight_id,
			.format = &adsc_format_flight_id,
			.destroy = NULL
		}
	},
	{
		.id = 13,
		.val = &(adsc_type_descriptor_t){
			.label = "Predicted route",
			.parse = &adsc_parse_predicted_route,
			.format = &adsc_format_predicted_route,
			.destroy = NULL
		}
	},
	{
		.id = 14,
		.val = &(adsc_type_descriptor_t){
			.label = "Earth reference data",
			.parse = &adsc_parse_earth_air_ref,
			.format = &adsc_format_earth_ref,
			.destroy = NULL
		}
	},
	{
		.id = 15,
		.val = &(adsc_type_descriptor_t){
			.label = "Air reference data",
			.parse = &adsc_parse_earth_air_ref,
			.format = &adsc_format_air_ref,
			.destroy = NULL
		}
	},
	{
		.id = 16,
		.val = &(adsc_type_descriptor_t){
			.label = "Meteo data",
			.parse = &adsc_parse_meteo,
			.format = &adsc_format_meteo,
			.destroy = NULL
		}
	},
	{
		.id = 17,
		.val = &(adsc_type_descriptor_t){
			.label = "Airframe ID",
			.parse = &adsc_parse_airframe_id,
			.format = &adsc_format_airframe_id,
			.destroy = NULL
		}
	},
	{
		.id = 18,
		.val = &(adsc_type_descriptor_t){
			.label = "Vertical rate change event",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 19,
		.val = &(adsc_type_descriptor_t){
			.label = "Altitude range event",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 20,
		.val = &(adsc_type_descriptor_t){
			.label = "Waypoint change event",
			.parse = &adsc_parse_basic_report,
			.format = &adsc_format_basic_report,
			.destroy = NULL
		}
	},
	{
		.id = 22,
		.val = &(adsc_type_descriptor_t){
			.label = "Intermediate projection",
			.parse = &adsc_parse_intermediate_projection,
			.format = &adsc_format_intermediate_projection,
			.destroy = NULL
		}
	},
	{
		.id = 23,
		.val = &(adsc_type_descriptor_t){
			.label = "Fixed projection",
			.parse = &adsc_parse_fixed_projection,
			.format = &adsc_format_fixed_projection,
			.destroy = NULL
		}
	},
	{
		.id = 255,	// Fake tag for reason code in DIS message
		.val = &(adsc_type_descriptor_t){
			.label = "Reason",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_dis_reason_code,
			.destroy = NULL
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

/**************************
 * Downlink tag destructors
 **************************/

static void adsc_destroy_noncomp_notify(void *data) {
	if(data == NULL)
		return;
	LA_CAST_PTR(n, adsc_noncomp_notify_t *, data);
	LA_XFREE(n->groups);
	LA_XFREE(data);
	return;
}

/**********************
 * Downlink tag parsers
 **********************/

#define BS_READ_OR_RETURN(bs, dest, len, ret) \
	if(bitstream_read_word_msbfirst(bs, dest, len) < 0) { return ret; }

ADSC_PARSER_PROTOTYPE(adsc_parse_nack) {
	uint32_t tag_len = 2;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_nack_t *n = LA_XCALLOC(1, sizeof(adsc_basic_report_t));

	n->contract_req_num = buf[0];
	if(buf[1] > ADSC_NACK_MAX_REASON_CODE) {
		la_debug_print("Invalid reason code: %u\n", buf[1]);
		goto fail;
	}
	n->reason = buf[1];
	la_debug_print("reason: %u\n", n->reason);

// these reason codes have extended data byte
	if(buf[1] == 1 || buf[1] == 2 || buf[1] == 7) {
		tag_len++;
		if(len < tag_len) {
			la_debug_print("Truncated tag %u: len: %u < %u\n", t->tag, len, tag_len);
			goto fail;
		}
		n->ext_data = buf[2];
		la_debug_print("ext_data: %u\n", n->ext_data);
	}
	t->data = n;
	return tag_len;
fail:
	LA_XFREE(n);
	return -1;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_noncomp_group) {
	uint32_t tag_len = 2;
	LA_CAST_PTR(g, adsc_noncomp_group_t *, dest);
	if(len < tag_len) {
		la_debug_print("too short: %u < %u\n", len, tag_len);
		return -1;
	}

	g->noncomp_tag = buf[0];
	g->is_unrecognized = (buf[1] & 0x80) ? 1 : 0;
	g->is_whole_group_unavail = (buf[1] & 0x40) ? 1 : 0;
	la_debug_print("tag: %u unrecognized: %u whole_group: %u\n",
		g->noncomp_tag, g->is_unrecognized, g->is_whole_group_unavail);

	if(g->is_unrecognized || g->is_whole_group_unavail) {
		return tag_len;
	}
	g->param_cnt = buf[1] & 0xf;
	la_debug_print("param_cnt: %u\n", g->param_cnt);
	if(g->param_cnt == 0) {
		return tag_len;
	}

// following octets contain 4-bit numbers of non-compliant parameters (up to 15)
	tag_len += g->param_cnt / 2 + g->param_cnt % 2;
	la_debug_print("new tag_len: %u\n", tag_len);
	if(len < tag_len) {
		la_debug_print("too short: %u < %u\n", len, tag_len);
		return -1;
	}
	buf += 2; len -= 2;
	for(int i = 0; i < g->param_cnt; i++) {
// store nibbles separately
		g->params[i] = (*buf >> (((i + 1) % 2) * 4)) & 0xf;
		buf += i % 2; len -= i % 2;
	}
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_noncomp_notify) {
	uint32_t tag_len = 2;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_noncomp_notify_t *n = LA_XCALLOC(1, sizeof(adsc_noncomp_notify_t));
	t->data = n;

	n->contract_req_num = buf[0];
	n->group_cnt = buf[1];
	if(n->group_cnt == 0) {
		return tag_len;
	}
	la_debug_print("group_cnt: %u\n", n->group_cnt);
	n->groups = LA_XCALLOC(n->group_cnt, sizeof(adsc_noncomp_group_t));
	buf += 2; len -= 2;
	int consumed_bytes = 0;
	for(uint8_t i = 0; i < n->group_cnt; i++) {
		la_debug_print("Remaining length: %u\n", len);
		if((consumed_bytes = adsc_parse_noncomp_group(n->groups + i, buf, len)) < 0) {
			return -1;
		}
		buf += consumed_bytes; len -= consumed_bytes;
		tag_len += consumed_bytes;
		if(len == 0) {
			if(i < n->group_cnt - 1) {
				la_debug_print("truncated: read %u/%u groups\n", i + 1, n->group_cnt);
				return -1;
			} else {
				break;	// parsing completed
			}
		}
	}
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_basic_report) {
	const uint32_t tag_len = 10;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_basic_report_t *r = LA_XCALLOC(1, sizeof(adsc_basic_report_t));
	t->data = r;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt = adsc_parse_altitude(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 15, -1);
	r->timestamp = adsc_parse_timestamp(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 7, -1);
	r->redundancy = (uint8_t)(tmp & 1);
	r->accuracy = (uint8_t)((tmp >> 1) & 0x7);
	r->tcas_health = (uint8_t)((tmp >> 4) & 1);
	la_debug_print("redundancy: %u accuracy: %u TCAS: %u\n",
		r->redundancy, r->accuracy, r->tcas_health);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_flight_id) {
	const uint32_t tag_len = 6;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_flight_id_t *f = LA_XCALLOC(1, sizeof(adsc_flight_id_t));
	t->data = f;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}
	uint32_t tmp = 0;
	for(int i = 0; i < sizeof(f->id) - 1; i++) {
// ISO5 alphabet on 6 bits, valid characters: A-Z, 0-9, space
// (00) 10 0000 - space
// (01) 0x xxxx - A-Z
// (00) 11 xxxx - 0-9
		BS_READ_OR_RETURN(bs, &tmp, 6, -1);
		if((tmp & 0x20) == 0)
			tmp += 0x40;
		f->id[i] = (uint8_t)tmp;
	}
	f->id[sizeof(f->id) - 1] = '\0';
	la_debug_print("%s\n", f->id);
	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_predicted_route) {
	const uint32_t tag_len = 17;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_predicted_route_t *r = LA_XCALLOC(1, sizeof(adsc_predicted_route_t));
	t->data = r;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat_next = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon_next = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt_next = adsc_parse_altitude(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	r->eta_next = tmp;
	la_debug_print("eta: %d\n", r->eta_next);
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lat_next_next = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	r->lon_next_next = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	r->alt_next_next = adsc_parse_altitude(tmp);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_earth_air_ref) {
	const uint32_t tag_len = 5;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_earth_air_ref_t *r = LA_XCALLOC(1, sizeof(adsc_earth_air_ref_t));
	t->data = r;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	r->heading_invalid = tmp;
	BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	r->heading = adsc_parse_heading(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 13, -1);
	r->speed = adsc_parse_speed(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	r->vert_speed = adsc_parse_vert_speed(tmp);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_intermediate_projection) {
	const uint32_t tag_len = 8;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_intermediate_projection_t *p = LA_XCALLOC(1, sizeof(adsc_intermediate_projection_t));
	t->data = p;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->distance = adsc_parse_distance(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	p->track_invalid = tmp;
	BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	p->track = adsc_parse_heading(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->alt = adsc_parse_altitude(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	p->eta = tmp;
	la_debug_print("eta: %d\n", p->eta);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_fixed_projection) {
	const uint32_t tag_len = 9;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_fixed_projection_t *p = LA_XCALLOC(1, sizeof(adsc_fixed_projection_t));
	t->data = p;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	p->lat = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 21, -1);
	p->lon = adsc_parse_coordinate(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 16, -1);
	p->alt = adsc_parse_altitude(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 14, -1);
	p->eta = tmp;
	la_debug_print("eta: %d\n", p->eta);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_meteo) {
	const uint32_t tag_len = 4;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_meteo_t *m = LA_XCALLOC(1, sizeof(adsc_meteo_t));
	t->data = m;

	bitstream_t *bs = bitstream_init(tag_len * 8);
	if(bitstream_append_msbfirst(bs, buf, tag_len, 8) < 0) {
		return -1;
	}

	uint32_t tmp;
	BS_READ_OR_RETURN(bs, &tmp, 9, -1);
	m->wind_speed = adsc_parse_speed(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 1, -1);
	m->wind_dir_invalid = tmp;
	BS_READ_OR_RETURN(bs, &tmp, 9, -1);
	m->wind_dir = adsc_parse_wind_dir(tmp);
	BS_READ_OR_RETURN(bs, &tmp, 12, -1);
	m->temp = adsc_parse_temperature(tmp);

	bitstream_destroy(bs);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_airframe_id) {
	const uint32_t tag_len = 3;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_airframe_id_t *a = LA_XCALLOC(1, sizeof(adsc_airframe_id_t));
	t->data = a;

	memcpy(a->icao_hex, buf, tag_len);
	return tag_len;
}

/*************************
 * Downlink tag formatters
 *************************/

ADSC_FORMATTER_PROTOTYPE(adsc_format_nack) {
	static const char *reason_code_table[ADSC_NACK_MAX_REASON_CODE+1] = {
		[0] = NULL,
		[1] = "Duplicate group tag",
		[2] = "Duplicate reporting interval tag",
		[3] = "Event contract request with no data",
		[4] = "Improper operational mode tag",
		[5] = "Cancel request of a contract which does not exist",
		[6] = "Requested contract already exists",
		[7] = "Undefined contract request tag",
		[8] = "Undefined error",
		[9] = "Not enough data in request",
		[10] = "Invalid altitude range: low limit >= high limit",
		[11] = "Vertical speed threshold is 0",
		[12] = "Aircraft intent projection time is 0",
		[13] = "Lateral deviation threshold is 0"
	};
	LA_CAST_PTR(n, adsc_nack_t *, data);
	char *str = NULL;
	char *tmp = NULL;
	if(n->reason == 1 || n->reason == 2 || n->reason == 7) {
		XASPRINTF(NULL, &tmp,
			"\n  Erroneous octet number: %u",
			n->ext_data
		);
	}
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Contract request number: %u\n"
		"  Reason: %u (%s)"
		"%s",
		label,
		n->contract_req_num,
		n->reason,
		reason_code_table[n->reason],
		tmp ? tmp : ""
	);
	LA_XFREE(tmp);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_dis_reason_code) {
	static la_dict const dis_reason_code_table[] = {
		{ 0, "reason not specified" },
		{ 1, "congestion" },
		{ 2, "application not available" },
		{ 8, "normal disconnect" },
		{ 0, NULL }
	};
	LA_CAST_PTR(rc, uint8_t *, data);
	uint8_t reason = *rc >> 4;
	char *descr = la_dict_search(dis_reason_code_table, reason);
	char *str = NULL;
	if(descr) {
		XASPRINTF(NULL, &str, "%s: %s", label, descr);
	} else {
		XASPRINTF(NULL, &str, "%s: unknown (%u)", label, reason);
	}
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_noncomp_group) {
	LA_CAST_PTR(g, adsc_noncomp_group_t *, data);
	char *str = NULL;
	char *grp_header;
	char param_numbers[64];
	param_numbers[0] = '\0';

	XASPRINTF(NULL, &grp_header,
		"Tag %u:\n"
		"   %s",
		g->noncomp_tag,
		g->is_unrecognized ? "Unrecognized group" :
			(g->is_whole_group_unavail ? "Unavailable group" : "Unavailable parameters: ")
	);
	size_t total_len = strlen(grp_header);
	if(!g->is_unrecognized && !g->is_whole_group_unavail && g->param_cnt > 0) {
		char tmp[5];
		for(int i = 0; i < g->param_cnt; i++) {
			snprintf(tmp, sizeof(tmp), "%d ", g->params[i]);
			strcat(param_numbers, tmp);
		}
		total_len += strlen(param_numbers);
	}
	str = LA_XCALLOC(total_len + 1, sizeof(char));
	strcat(str, grp_header);
	LA_XFREE(grp_header);
	if(strlen(param_numbers) > 0) {
		strcat(str, param_numbers);
	}
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_noncomp_notify) {
	LA_CAST_PTR(n, adsc_noncomp_notify_t *, data);
	char *str = NULL;
	char *header = NULL;
	XASPRINTF(NULL, &header,
		"%s:\n"
		"  Contract number: %u",
		label,
		n->contract_req_num
	);
	char **tmp = NULL;
	size_t total_len = strlen(header);
	if(n->group_cnt > 0) {
		tmp = LA_XCALLOC(n->group_cnt, sizeof(char *));
		for(int i = 0; i < n->group_cnt; i++) {
			tmp[i] = adsc_format_noncomp_group(NULL, n->groups + i);
			if(tmp[i] != NULL) {
				total_len += strlen(tmp[i]) + 3;	// add room for newline and two spaces
			}
		}
	}
	str = LA_XCALLOC(total_len + 1, sizeof(char));	// add room for '\0'
	strcat(str, header);
	LA_XFREE(header);
	if(n->group_cnt > 0) {
		for(int i = 0; i < n->group_cnt; i++) {
			strcat(str, "\n  ");
			strcat(str, tmp[i]);
			LA_XFREE(tmp[i]);
		}
		LA_XFREE(tmp);
	}
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_basic_report) {
	static const char *accuracy_table[] = {
		[0] = "none (NAV capability lost)",
		[1] = "<30 nm",
		[2] = "<15 nm",
		[3] = "<8 nm",
		[4] = "<4 nm",
		[5] = "<1 nm",
		[6] = "<0.25 nm",
		[7] = "<0.05 nm"
	};
	static const char *redundancy_state_table[] = {
		[0] = "lost",
		[1] = "OK"
	};
	static const char *tcas_state_table[] = {
		[0] = "not available to ADS",
		[1] = "OK"
	};
	LA_CAST_PTR(r, adsc_basic_report_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Lat: %.7f\n"
		"  Lon: %.7f\n"
		"  Alt: %d ft\n"
		"  Time: %.3f sec past hour (:%02.0f:%06.3f)\n"
		"  Position accuracy: %s\n"
		"  NAV unit redundancy: %s\n"
		"  TCAS: %s",
		label,
		r->lat,
		r->lon,
		r->alt,
		r->timestamp,
		trunc(r->timestamp / 60.0),
		r->timestamp - 60.0 * trunc(r->timestamp / 60.0),
		accuracy_table[r->accuracy],
		redundancy_state_table[r->redundancy],
		tcas_state_table[r->tcas_health]
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_flight_id) {
	LA_CAST_PTR(f, adsc_flight_id_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Flight ID: %s",
		label,
		f->id
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_predicted_route) {
	LA_CAST_PTR(r, adsc_predicted_route_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Next waypoint:\n"
		"   Lat: %.7f\n"
		"   Lon: %.7f\n"
		"   Alt: %d ft\n"
		"   ETA: %d sec\n"
		"  Next+1 waypoint:\n"
		"   Lat: %.7f\n"
		"   Lon: %.7f\n"
		"   Alt: %d ft",
		label,
		r->lat_next,
		r->lon_next,
		r->alt_next,
		r->eta_next,
		r->lat_next_next,
		r->lon_next_next,
		r->alt_next_next
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_earth_ref) {
	LA_CAST_PTR(r, adsc_earth_air_ref_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  True track: %.1f deg%s\n"
		"  Ground speed: %.1f kt\n"
		"  Vertical speed: %d ft/min",
		label,
		r->heading,
		r->heading_invalid ? " (invalid)" : "",
		r->speed,
		r->vert_speed
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_air_ref) {
	LA_CAST_PTR(r, adsc_earth_air_ref_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  True heading: %.1f deg%s\n"
		"  Mach speed: %.4f\n"
		"  Vertical speed: %d ft/min",
		label,
		r->heading,
		r->heading_invalid ? " (invalid)" : "",
		r->speed / 1000.0,
		r->vert_speed
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_intermediate_projection) {
	LA_CAST_PTR(p, adsc_intermediate_projection_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Distance: %.3f nm\n"
		"  True track: %.1f deg%s\n"
		"  Alt: %d ft\n"
		"  ETA: %d sec",
		label,
		p->distance,
		p->track,
		p->track_invalid ? " (invalid)" : "",
		p->alt,
		p->eta
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_fixed_projection) {
	LA_CAST_PTR(p, adsc_fixed_projection_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Lat: %.7f\n"
		"  Lon: %.7f\n"
		"  Alt: %d ft\n"
		"  ETA: %d sec",
		label,
		p->lat,
		p->lon,
		p->alt,
		p->eta
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_meteo) {
	LA_CAST_PTR(m, adsc_meteo_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  Wind speed: %.1f kt\n"
		"  True wind direction: %.1f deg%s\n"
		"  Temperature: %.2f C",
		label,
		m->wind_speed,
		m->wind_dir,
		m->wind_dir_invalid ? " (invalid)" : "",
		m->temp
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_airframe_id) {
	LA_CAST_PTR(a, adsc_airframe_id_t *, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s:\n"
		"  ICAO ID: %02X%02X%02X",
		label,
		a->icao_hex[0], a->icao_hex[1], a->icao_hex[2]
	);
	return str;
}

/****************
 * Uplink tags
 ****************/

static la_dict const adsc_uplink_tag_descriptor_table[] = {
	{
		.id = 1,
		.val = &(adsc_type_descriptor_t){
			.label = "Cancel all contracts and terminate connection",
			.parse = NULL,
			.format = &adsc_format_empty_tag,
			.destroy = NULL
		}
	},
	{
		.id = 2,
		.val = &(adsc_type_descriptor_t){
			.label = "Cancel contract",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_tag_with_contract_number,
			.destroy = NULL
		}
	},
	{
		.id = 6,
		.val = &(adsc_type_descriptor_t){
			.label = "Cancel emergency mode",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_tag_with_contract_number,
			.destroy = NULL
		}
	},
	{
		.id = 7,
		.val = &(adsc_type_descriptor_t){
			.label = "Periodic contract request",
			.parse = &adsc_parse_contract_request,
			.format = &adsc_format_contract_request,
			.destroy = &adsc_destroy_contract_request
		}
	},
	{
		.id = 8,
		.val = &(adsc_type_descriptor_t){
			.label = "Event contract request",
			.parse = &adsc_parse_contract_request,
			.format = &adsc_format_contract_request,
			.destroy = &adsc_destroy_contract_request
		}
	},
	{
		.id = 9,
		.val = &(adsc_type_descriptor_t){
			.label = "Emergency periodic contract request",
			.parse = &adsc_parse_contract_request,
			.format = &adsc_format_contract_request,
			.destroy = &adsc_destroy_contract_request
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

static la_dict const adsc_request_tag_descriptor_table[] = {
	{
		.id = 10,
		.val = &(adsc_type_descriptor_t){
			.label = "Report when lateral deviation exceeds",
			.parse = &adsc_parse_lat_dev_change,
			.format = &adsc_format_lat_dev_change,
			.destroy = NULL
		}
	},
	{
		.id = 11,
		.val = &(adsc_type_descriptor_t){
			.label = "Reporting interval",
			.parse = &adsc_parse_reporting_interval,
			.format = &adsc_format_reporting_interval,
			.destroy = NULL
		}
	},
	{
		.id = 12,
		.val = &(adsc_type_descriptor_t){
			.label = "Flight ID",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 13,
		.val = &(adsc_type_descriptor_t){
			.label = "Predicted route",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 14,
		.val = &(adsc_type_descriptor_t){
			.label = "Earth reference data",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 15,
		.val = &(adsc_type_descriptor_t){
			.label = "Air reference data",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 16,
		.val = &(adsc_type_descriptor_t){
			.label = "Meteo data",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 17,
		.val = &(adsc_type_descriptor_t){
			.label = "Airframe ID",
			.parse = &adsc_parse_uint8_t,
			.format = &adsc_format_modulus,
			.destroy = NULL
		}
	},
	{
		.id = 18,
		.val = &(adsc_type_descriptor_t){
			.label = "Report when vertical speed is",
			.parse = &adsc_parse_vspd_change,
			.format = &adsc_format_vspd_change,
			.destroy = NULL
		}
	},
	{
		.id = 19,
		.val = &(adsc_type_descriptor_t){
			.label = "Report when altitude out of range",
			.parse = &adsc_parse_alt_range,
			.format = &adsc_format_alt_range,
			.destroy = NULL
		}
	},
	{
		.id = 20,
		.val = &(adsc_type_descriptor_t){
			.label = "Report waypoint changes",
			.parse = NULL,
			.format = &adsc_format_empty_tag,
			.destroy = NULL
		}
	},
	{
		.id = 21,
		.val = &(adsc_type_descriptor_t){
			.label = "Aircraft intent data",
			.parse = &adsc_parse_acft_intent_group,
			.format = &adsc_format_acft_intent_group,
			.destroy = NULL
		}
	},
	{
		.id = 0,
		.val = NULL
	}
};

/*****************
 * Tag destructors
 *****************/

static void adsc_tag_destroy(void *tag) {
	if(tag == NULL)
		return;
	LA_CAST_PTR(t, adsc_tag_t *, tag);
	if(t->data == NULL || t->type == NULL) {
		LA_XFREE(tag);
		return;
	}
	if(t->type->destroy != NULL)
// simple types do not have any special destructors
// and can be freed directly
		t->type->destroy(t->data);
	else
		LA_XFREE(t->data);
	LA_XFREE(tag);
}

static void adsc_destroy_contract_request(void *data) {
	if(data == NULL) return;
	LA_CAST_PTR(r, adsc_req_t *, data);
	if(r->req_tag_list != NULL) {
		la_list_free_full(r->req_tag_list, adsc_tag_destroy);
		r->req_tag_list = NULL;
	}
	LA_XFREE(data);
}

/****************
 * Tag formatters
 ****************/

ADSC_FORMATTER_PROTOTYPE(adsc_format_empty_tag) {
	return strdup(label);
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_tag_with_contract_number) {
	char *str = NULL;
	XASPRINTF(NULL, &str, "%s:\n  Contract number: %u", label, *(uint8_t *)data);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_modulus) {
	char *str = NULL;
	XASPRINTF(NULL, &str, "%s: every %u reports", label, *(uint8_t *)data);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_reporting_interval) {
	LA_CAST_PTR(t, adsc_report_interval_req_t const * const, data);
	char *str = NULL;
	XASPRINTF(NULL, &str, "%s: %d seconds", label, (int)(t->scaling_factor) * (int)(t->rate));
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_acft_intent_group) {
	LA_CAST_PTR(t, adsc_acft_intent_group_req_t const * const, data);
	char *str = NULL;
	XASPRINTF(NULL, &str, "%s: every %u reports, projection time: %u minutes",
		label, t->modulus, t->acft_intent_projection_time);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_lat_dev_change) {
	LA_CAST_PTR(e, adsc_lat_dev_chg_event_t const * const, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s: %.3f nm",
		label,
		e->lat_dev_threshold
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_vspd_change) {
	LA_CAST_PTR(e, adsc_vspd_chg_event_t const * const, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s: %c%d ft",
		label,
		e->vspd_threshold >= 0 ? '>' : '<',
		abs(e->vspd_threshold)
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_alt_range) {
	LA_CAST_PTR(e, adsc_alt_range_event_t const * const, data);
	char *str = NULL;
	XASPRINTF(NULL, &str,
		"%s: %d-%d ft",
		label,
		e->floor_alt,
		e->ceiling_alt
	);
	return str;
}

ADSC_FORMATTER_PROTOTYPE(adsc_format_contract_request) {
	LA_CAST_PTR(r, adsc_req_t const * const, data);
	char *header = NULL;
	XASPRINTF(NULL, &header,
		"%s:\n"
		"  Contract number: %u",
		label,
		r->contract_num
	);
	size_t len = la_list_length(r->req_tag_list);
	if(len == 0) {
		return header;
	}
	la_list *str_list = NULL;
	size_t total_len = strlen(header);
	for(la_list *ptr = r->req_tag_list; ptr != NULL; ptr = la_list_next(ptr)) {
		LA_CAST_PTR(t, adsc_tag_t *, ptr->data);
		if(!t->type) {
			char *s = NULL;
			XASPRINTF(NULL, &s, "-- Unparseable tag %u", t->tag);
			str_list = la_list_append(str_list, s);
			total_len += strlen(s) + 3;
			break;
		}
		la_assert(t->type->format != NULL);
		char *s = (*(t->type->format))(t->type->label, t->data);
		if(s != NULL) {
			la_debug_print("fmt tag: %s\n", s);
			str_list = la_list_append(str_list, s);
			total_len += strlen(s) + 3;	// add room for newline + 2 indenting spaces
		}
	}
	char *str = LA_XCALLOC(total_len + 1, sizeof(char));	// add room for '\0'
	strcat(str, header);
	LA_XFREE(header);
	for(la_list *ptr = str_list; ptr != NULL; ptr = la_list_next(ptr)) {
		strcat(str, "\n  ");
		strcat(str, (char const *)(ptr->data));
		LA_XFREE(ptr->data);
	}
	la_list_free(str_list);
	return str;
}

/**************
 * Tag parsers
 **************/

ADSC_PARSER_PROTOTYPE(adsc_parse_uint8_t) {
	uint32_t tag_len = 1;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	uint8_t *ptr = LA_XCALLOC(1, sizeof(uint8_t));
	*ptr = buf[0];
	la_debug_print("val=%u\n", *ptr);
	t->data = ptr;
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_reporting_interval) {
	uint32_t tag_len = 1;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_report_interval_req_t *ri = LA_XCALLOC(1, sizeof(adsc_report_interval_req_t));
	t->data = ri;
	uint8_t sf = (buf[0] & 0xc0) >> 6;
// convert scaling factor to multiplier value
	if(sf == 2)
		sf = 8;
	else if(sf == 3)
		sf = 64;
	ri->scaling_factor = sf;
	ri->rate = buf[0] & 0x3f;
	la_debug_print("SF=%u rate=%u\n", ri->scaling_factor, ri->rate);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_lat_dev_change) {
	uint32_t tag_len = 1;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_lat_dev_chg_event_t *e = LA_XCALLOC(1, sizeof(adsc_lat_dev_chg_event_t));
	t->data = e;

	e->lat_dev_threshold = (double)buf[0] / 8.0;
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_vspd_change) {
	uint32_t tag_len = 1;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_vspd_chg_event_t *e = LA_XCALLOC(1, sizeof(adsc_vspd_chg_event_t));
	t->data = e;

	e->vspd_threshold = (char)buf[0] * 64;
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_alt_range) {
	uint32_t tag_len = 4;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_alt_range_event_t *e = LA_XCALLOC(1, sizeof(adsc_alt_range_event_t));
	t->data = e;

	uint32_t tmp = 0;
	tmp = (buf[0] << 8) | buf[1];
	e->ceiling_alt = adsc_parse_altitude(tmp);
	tmp = (buf[2] << 8) | buf[3];
	e->floor_alt = adsc_parse_altitude(tmp);
	return tag_len;
}

ADSC_PARSER_PROTOTYPE(adsc_parse_acft_intent_group) {
	uint32_t tag_len = 2;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	adsc_acft_intent_group_req_t *aig = LA_XCALLOC(1, sizeof(adsc_acft_intent_group_req_t));
	t->data = aig;
	aig->modulus = buf[0];
	aig->acft_intent_projection_time = buf[1];
	la_debug_print("modulus=%u projection_time=%u\n", aig->modulus, aig->acft_intent_projection_time);
	return tag_len;
}


ADSC_PARSER_PROTOTYPE(adsc_parse_contract_request) {
	uint32_t tag_len = 1;
	LA_CAST_PTR(t, adsc_tag_t *, dest);
	ADSC_CHECK_LEN(t->tag, len, tag_len);
	int consumed_bytes = 0;
	adsc_req_t *r = LA_XCALLOC(1, sizeof(adsc_req_t));
	t->data = r;

	r->contract_num = buf[0];
	buf++; len--;

	while(len > 0) {
		la_debug_print("Remaining length: %u\n", len);
		adsc_tag_t *req_tag = LA_XCALLOC(1, sizeof(adsc_tag_t));
		r->req_tag_list = la_list_append(r->req_tag_list, req_tag);
		if((consumed_bytes = adsc_parse_tag(req_tag, adsc_request_tag_descriptor_table, buf, len)) < 0) {
			return -1;
		}
		buf += consumed_bytes; len -= consumed_bytes;
		tag_len += consumed_bytes;
	}
	return tag_len;
}

static int adsc_parse_tag(adsc_tag_t *t, la_dict const *tag_descriptor_table, uint8_t *buf, uint32_t len) {
	uint32_t tag_len = 1;
	if(len < tag_len) {
		la_debug_print("%s", "Buffer len is 0\n");
		return -1;
	}

	t->tag = buf[0];
	buf++; len--;
	LA_CAST_PTR(type, adsc_type_descriptor_t *, la_dict_search(tag_descriptor_table, t->tag));
	if(type == NULL) {
		la_debug_print("Unknown tag %u\n", t->tag);
		return -1;
	}
	la_debug_print("Found tag %u (%s)\n", t->tag, type->label);
	int consumed_bytes = 0;
	if(type->parse == NULL) {	// tag is empty, no parsing required - return with success
		goto end;
	}
	if((consumed_bytes = (*(type->parse))(t, buf, len)) < 0) {
		return -1;
	}
end:
	tag_len += consumed_bytes;
	t->type = type;
	return tag_len;
}

la_proto_node *adsc_parse(uint8_t *buf, int len, la_msg_dir msg_dir, la_arinc_imi imi) {
	if(buf == NULL)
		return NULL;

	la_proto_node *node = la_proto_node_new();
	adsc_msg_t *msg = LA_XCALLOC(1, sizeof(adsc_msg_t));
	node->data = msg;
	node->td = &la_DEF_adsc_message;
	adsc_tag_t *tag = NULL;
	int consumed_bytes;

// Uplink and downlink tag values are the same, but their syntax is different.
// Figure out the la_dictionary to use based on the message direction.
	static la_dict const *tag_table = NULL;
	if(msg_dir == LA_MSG_DIR_GND2AIR)
		tag_table = adsc_uplink_tag_descriptor_table;
	else if(msg_dir == LA_MSG_DIR_AIR2GND)
		tag_table = adsc_downlink_tag_descriptor_table;
	la_assert(tag_table != NULL);

	switch(imi) {
	case ARINC_MSG_ADS:
		while(len > 0) {
			la_debug_print("Remaining length: %u\n", len);
			tag = LA_XCALLOC(1, sizeof(adsc_tag_t));
			msg->tag_list = la_list_append(msg->tag_list, tag);
			if((consumed_bytes = adsc_parse_tag(tag, tag_table, buf, len)) < 0) {
				msg->err = 1;
				break;
			}
			buf += consumed_bytes; len -= consumed_bytes;
		}
		break;
	case ARINC_MSG_DIS:
// DIS payload consists of an error code only, without any tag.
// Let's insert a fake tag value of 255.
		if(len < 1) {
			la_debug_print("%s", "DIS message too short");
			msg->err = 1;
			break;
		}
		tag = LA_XCALLOC(1, sizeof(adsc_tag_t));
		msg->tag_list = la_list_append(msg->tag_list, tag);
		len = 2;
		uint8_t *tmpbuf = LA_XCALLOC(len, sizeof(uint8_t));
		tmpbuf[0] = 255;
		tmpbuf[1] = buf[0];
		if(adsc_parse_tag(tag, tag_table, tmpbuf, len) < 0) {
			msg->err = 1;
		}
		LA_XFREE(tmpbuf);
		break;
	default:
		break;
	}
	return node;
}

static void adsc_output_tag(void const * const p, void *ctx) {
	la_assert(p);
	la_assert(ctx);

	LA_CAST_PTR(t, adsc_tag_t *, p);
	LA_CAST_PTR(vstr, la_vstring *, ctx);
	if(!t->type) {
		la_vstring_append_sprintf(vstr, "-- Unparseable tag %u\n", t->tag);
		return;
	}
	if(t->type->format != NULL) {
		char *str = (*(t->type->format))(t->type->label, t->data);
		if(str != NULL) {
			la_vstring_append_sprintf(vstr, " %s\n", str);
			LA_XFREE(str);
		}
	}
}

void adsc_format_text(la_vstring * const vstr, void const * const data) {
	la_assert(vstr);
	la_assert(data);

	LA_CAST_PTR(msg, adsc_msg_t *, data);
	if(msg->tag_list == NULL) {
		la_vstring_append_sprintf(vstr, "-- Empty ADS-C message\n");
		return;
	}
	la_list_foreach(msg->tag_list, adsc_output_tag, vstr);
	if(msg->err != 0) {
		la_vstring_append_sprintf(vstr, "-- Malformed ADS-C message\n");
	}
}

void adsc_destroy(void *data) {
	if(data == NULL) {
		return;
	}
	LA_CAST_PTR(msg, adsc_msg_t *, data);
	la_list_free_full(msg->tag_list, adsc_tag_destroy);
	msg->tag_list = NULL;
	msg->err = 0;
	LA_XFREE(data);
}

la_type_descriptor const la_DEF_adsc_message = {
	.header = NULL,
	.format_text = &adsc_format_text,
	.destroy = &adsc_destroy
};
