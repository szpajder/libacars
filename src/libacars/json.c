/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <libacars/macros.h>		// la_assert()
#include <libacars/vstring.h>		// la_vstring

void la_json_append_bool(la_vstring * const vstr, char const * const key, bool const val) {
	la_assert(vstr != NULL);
	la_assert(key != NULL);
	la_vstring_append_sprintf(vstr, "\"%s\":%s,", key, (val == true ? "true" : "false"));
}

void la_json_append_string(la_vstring * const vstr, char const * const key, char const * const val) {
	la_assert(vstr != NULL);
	la_assert(key != NULL);
	if(val == NULL) {
		return;
	}
	la_vstring_append_sprintf(vstr, "\"%s\":\"%s\",", key, val);
}

void la_json_object_start(la_vstring * const vstr, char const * const key) {
	la_assert(vstr != NULL);
	if(key == NULL || key[0] == '\0') {
		la_vstring_append_sprintf(vstr, "%s", "{");
	} else {
		la_vstring_append_sprintf(vstr, "\"%s\":{", key);
	}
}

void la_json_object_end(la_vstring * const vstr) {
	la_assert(vstr != NULL);
// FIXME: la_vstring_rtrim
	size_t len = vstr->len;
	if(len > 0 && vstr->str[len-1] == ',') {
		vstr->str[len-1] = '\0';
		vstr->len--;
	}
	la_vstring_append_sprintf(vstr, "%s", "}");
}
