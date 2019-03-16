/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <libacars/macros.h>		// la_assert()
#include <libacars/vstring.h>		// la_vstring

static void la_json_trim_comma(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	size_t len = vstr->len;
	if(len > 0 && vstr->str[len-1] == ',') {
		vstr->str[len-1] = '\0';
		vstr->len--;
	}
}

static inline void la_json_print_key(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	if(key != NULL && key[0] != '\0') {
		la_vstring_append_sprintf(vstr, "\"%s\":", key);
	}
}

void la_json_append_bool(la_vstring * const vstr, char const * const key, bool const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s,", (val == true ? "true" : "false"));
}

void la_json_append_double(la_vstring * const vstr, char const * const key, double const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%f,", val);
}

void la_json_append_long(la_vstring * const vstr, char const * const key, long const val) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%ld,", val);
}

void la_json_append_string(la_vstring * const vstr, char const * const key, char const * const val) {
	la_assert(vstr != NULL);
	if(val == NULL) {
		return;
	}
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "\"%s\",", val);
}

void la_json_object_start(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "{");
}

void la_json_object_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "},");
}

void la_json_array_start(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	la_json_print_key(vstr, key);
	la_vstring_append_sprintf(vstr, "%s", "[");
}

void la_json_array_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_trim_comma(vstr);
	la_vstring_append_sprintf(vstr, "%s", "],");
}

void la_json_start(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_object_start(vstr, NULL);
}

void la_json_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
	la_json_object_end(vstr);
	la_json_trim_comma(vstr);
}
