/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2020 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <libacars/asn1/asn_application.h>      // asn_TYPE_descriptor_t, asn_sprintf
#include <libacars/asn1/OCTET_STRING.h>         // OCTET_STRING_t
#include <libacars/asn1/INTEGER.h>              // asn_INTEGER_enum_map_t, asn_INTEGER2long()
#include <libacars/asn1/BOOLEAN.h>              // BOOLEAN_t
#include <libacars/asn1/constr_CHOICE.h>        // _fetch_present_idx()
#include <libacars/asn1/asn_SET_OF.h>           // _A_CSET_FROM_VOID()
#include <libacars/asn1-util.h>                 // LA_ASN1_FORMATTER_PROTOTYPE
#include <libacars/macros.h>                    // LA_CAST_PTR
#include <libacars/util.h>                      // la_dict_search
#include <libacars/vstring.h>                   // la_vstring, la_vstring_append_sprintf(), LA_ISPRINTF
#include <libacars/json.h>                      // la_json_*()

char const *la_value2enum(asn_TYPE_descriptor_t *td, long const value) {
	if(td == NULL) return NULL;
	asn_INTEGER_enum_map_t const *enum_map = INTEGER_map_value2enum(td->specifics, value);
	if(enum_map == NULL) return NULL;
	return enum_map->enum_name;
}

void la_format_INTEGER_with_unit_as_text(la_asn1_formatter_params p,
		char const *unit, double multiplier, int decimal_places) {
	long const *val = p.sptr;
	LA_ISPRINTF(p.vstr, p.indent, "%s: %.*f%s\n", p.label, decimal_places, (double)(*val) * multiplier, unit);
}

void la_format_INTEGER_with_unit_as_json(la_asn1_formatter_params p,
		char const *unit, double multiplier) {
	long const *val = p.sptr;
	la_json_object_start(p.vstr, p.label);
	la_json_append_double(p.vstr, "val", (double)(*val) * multiplier);
	la_json_append_string(p.vstr, "unit", unit);
	la_json_object_end(p.vstr);
}

void la_format_CHOICE_as_text(la_asn1_formatter_params p, la_dict const *choice_labels,
		la_asn1_formatter_fun cb) {
	asn_CHOICE_specifics_t *specs = p.td->specifics;
	int present = _fetch_present_idx(p.sptr, specs->pres_offset, specs->pres_size);
	if(p.label != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s:\n", p.label);
		p.indent++;
	}
	if(choice_labels != NULL) {
		char const *descr = la_dict_search(choice_labels, present);
		if(descr != NULL) {
			LA_ISPRINTF(p.vstr, p.indent, "%s\n", descr);
		} else {
			LA_ISPRINTF(p.vstr, p.indent, "<no description for CHOICE value %d>\n", present);
		}
		p.indent++;
	}
	if(present > 0 && present <= p.td->elements_count) {
		asn_TYPE_member_t *elm = &p.td->elements[present-1];
		void const *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(void const * const *)((char const *)p.sptr + elm->memb_offset);
			if(!memb_ptr) {
				LA_ISPRINTF(p.vstr, p.indent, "%s: <not present>\n", elm->name);
				return;
			}
		} else {
			memb_ptr = (void const *)((char const *)p.sptr + elm->memb_offset);
		}

		p.td = elm->type;
		p.sptr = memb_ptr;
		cb(p);
	} else {
		LA_ISPRINTF(p.vstr, p.indent, "-- %s: value %d out of range\n", p.td->name, present);
	}
}

void la_format_CHOICE_as_json(la_asn1_formatter_params p, la_dict const *choice_labels,
		la_asn1_formatter_fun cb) {
	asn_CHOICE_specifics_t const *specs = p.td->specifics;
	int present = _fetch_present_idx(p.sptr, specs->pres_offset, specs->pres_size);
	la_json_object_start(p.vstr, p.label);
	if(choice_labels != NULL) {
		char const *descr = la_dict_search(choice_labels, present);
		la_json_append_string(p.vstr, "choice_label", descr != NULL ? descr : "");
	}
	if(present > 0 && present <= p.td->elements_count) {
		asn_TYPE_member_t *elm = &p.td->elements[present-1];
		void const *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(void const * const *)((char const *)p.sptr + elm->memb_offset);
			if(!memb_ptr) {
				goto end;
			}
		} else {
			memb_ptr = (void const *)((char const *)p.sptr + elm->memb_offset);
		}
		la_json_append_string(p.vstr, "choice", elm->name);
		la_json_object_start(p.vstr, "data");
		p.td = elm->type;
		p.sptr = memb_ptr;
		cb(p);
		la_json_object_end(p.vstr);
	}
end:
	la_json_object_end(p.vstr);
}

void la_format_SEQUENCE_as_text(la_asn1_formatter_params p, la_asn1_formatter_fun cb) {
	if(p.label != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s:\n", p.label);
		p.indent++;
	}
	la_asn1_formatter_params cb_p = p;
	for(int edx = 0; edx < p.td->elements_count; edx++) {
		asn_TYPE_member_t *elm = &p.td->elements[edx];
		void const *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(void const * const *)((char const *)p.sptr + elm->memb_offset);
			if(!memb_ptr) {
				continue;
			}
		} else {
			memb_ptr = (void const *)((char const *)p.sptr + elm->memb_offset);
		}
		cb_p.td = elm->type;
		cb_p.sptr = memb_ptr;
		cb(cb_p);
	}
}

void la_format_SEQUENCE_as_json(la_asn1_formatter_params p, la_asn1_formatter_fun cb) {
	la_asn1_formatter_params cb_p = p;
	la_json_array_start(p.vstr, p.label);
	for(int edx = 0; edx < p.td->elements_count; edx++) {
		asn_TYPE_member_t *elm = &p.td->elements[edx];
		void const *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(void const * const *)((char const *)p.sptr + elm->memb_offset);
			if(!memb_ptr) {
				continue;
			}
		} else {
			memb_ptr = (void const *)((char const *)p.sptr + elm->memb_offset);
		}
		cb_p.td = elm->type;
		cb_p.sptr = memb_ptr;
		la_json_object_start(p.vstr, NULL);
		cb(cb_p);
		la_json_object_end(p.vstr);
	}
	la_json_array_end(p.vstr);
}

void la_format_SEQUENCE_OF_as_text(la_asn1_formatter_params p, la_asn1_formatter_fun cb) {
	if(p.label != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s:\n", p.label);
		p.indent++;
	}
	asn_TYPE_member_t *elm = p.td->elements;
	asn_anonymous_set_ const *list = _A_CSET_FROM_VOID(p.sptr);
	for(int i = 0; i < list->count; i++) {
		void const *memb_ptr = list->array[i];
		if(memb_ptr == NULL) {
			continue;
		}
		p.td = elm->type;
		p.sptr = memb_ptr;
		cb(p);
	}
}

void la_format_SEQUENCE_OF_as_json(la_asn1_formatter_params p, la_asn1_formatter_fun cb) {
	la_json_array_start(p.vstr, p.label);
	asn_TYPE_member_t *elm = p.td->elements;
	asn_anonymous_set_ const *list = _A_CSET_FROM_VOID(p.sptr);
	for(int i = 0; i < list->count; i++) {
		void const *memb_ptr = list->array[i];
		if(memb_ptr == NULL) {
			continue;
		}
		la_json_object_start(p.vstr, NULL);
		p.td = elm->type;
		p.sptr = memb_ptr;
		cb(p);
		la_json_object_end(p.vstr);
	}
	la_json_array_end(p.vstr);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_any) {
	if(p.label != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s: ", p.label);
	} else {
		LA_ISPRINTF(p.vstr, p.indent, "%s", "");
	}
	asn_sprintf(p.vstr, p.td, p.sptr, 1);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_OCTET_STRING) {
	LA_CAST_PTR(octstr, OCTET_STRING_t *, p.sptr);
	// replace nulls with periods for printf() to work correctly
	char *buf = (char *)octstr->buf;
	for(int i = 0; i < octstr->size; i++) {
		if(buf[i] == '\0') buf[i] = '.';
	}
	if(p.label != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s: ", p.label);
	} else {
		LA_ISPRINTF(p.vstr, p.indent, "");
	}
	asn_sprintf(p.vstr, p.td, p.sptr, 1);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_json_OCTET_STRING) {
	LA_CAST_PTR(octstr, OCTET_STRING_t *, p.sptr);
	char *buf = (char *)octstr->buf;
	int size = octstr->size;
	char *string_buf = LA_XCALLOC(size + 1, sizeof(char));
	memcpy(string_buf, buf, size);
	string_buf[size] = '\0';
	la_json_append_string(p.vstr, p.label, string_buf);
	LA_XFREE(string_buf);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_NULL) {
	LA_UNUSED(p);
	// NOOP
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_ENUM) {
	long const value = *(long const *)p.sptr;
	char const *s = la_value2enum(p.td, value);
	if(s != NULL) {
		LA_ISPRINTF(p.vstr, p.indent, "%s: %s\n", p.label, s);
	} else {
		LA_ISPRINTF(p.vstr, p.indent, "%s: %ld\n", p.label, value);
	}
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_json_ENUM) {
	long const value = *(long const *)p.sptr;
	char const *s = la_value2enum(p.td, value);
	if(s != NULL) {
		la_json_append_string(p.vstr, p.label, s);
	} else {
		la_json_append_long(p.vstr, p.label, value);
	}
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_json_long) {
	LA_CAST_PTR(valptr, long *, p.sptr);
	la_json_append_long(p.vstr, p.label, *valptr);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_json_bool) {
	LA_CAST_PTR(valptr, BOOLEAN_t *, p.sptr);
	la_json_append_bool(p.vstr, p.label, (*valptr) ? true : false);
}
