/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdio.h>              // fprintf
#include <stdint.h>
#include <stdlib.h>             // calloc, realloc, free
#include <string.h>             // strerror, strlen, strdup, strnlen, strspn, strpbrk
#include <time.h>               // struct tm
#include <limits.h>             // CHAR_BIT
#include <errno.h>              // errno
#include "config.h"             // HAVE_STRSEP, WITH_LIBXML2, WITH_JANSSON, HAVE_UNISTD_H
#include "libacars.h"           // la_config_get_bool
#ifdef HAVE_UNISTD_H
#include <unistd.h>             // _exit
#endif
#ifdef WITH_LIBXML2
#include <libxml/parser.h>      // xmlParseDoc
#include <libxml/tree.h>        // xmlBuffer.*, xmlNodeDump, xmlDocGetRootElement, xmlFreeDoc
#include <libxml/xmlerror.h>    // initGenericErrorDefaultFunc, xmlGenericError
#endif
#ifdef WITH_ZLIB
#include <zlib.h>               // z_stream, inflateInit2(), inflate(), inflateEnd()
#endif
#ifdef WITH_JANSSON
#include <jansson.h>
#endif
#include <libacars/macros.h>    // la_debug_print()
#include <libacars/util.h>

void *la_xcalloc(size_t nmemb, size_t size, char const *file, int line, char const *func) {
	void *ptr = calloc(nmemb, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): calloc(%zu, %zu) failed: %s\n",
				file, line, func, nmemb, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_xrealloc(void *ptr, size_t size, char const *file, int line, char const *func) {
	ptr = realloc(ptr, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): realloc(%zu) failed: %s\n",
				file, line, func, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

size_t la_slurp_hexstring(char* string, uint8_t **buf) {
	if(string == NULL)
		return 0;
	size_t slen = strlen(string);
	if(slen & 1)
		slen--;
	size_t dlen = slen / 2;
	if(dlen == 0)
		return 0;
	*buf = LA_XCALLOC(dlen, sizeof(uint8_t));

	for(size_t i = 0; i < slen; i++) {
		char c = string[i];
		int value = 0;
		if(c >= '0' && c <= '9') {
			value = (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			value = (10 + (c - 'A'));
		} else if (c >= 'a' && c <= 'f') {
			value = (10 + (c - 'a'));
		} else {
			la_debug_print(D_ERROR, "stopped at invalid char %u at pos %zu\n", c, i);
			return i/2;
		}
		(*buf)[(i/2)] |= value << (((i + 1) % 2) * 4);
	}
	return dlen;
}

char *la_hexdump(uint8_t *data, size_t len) {
	static char const hex[] = "0123456789abcdef";
	if(data == NULL) return strdup("<undef>");
	if(len == 0) return strdup("<none>");

	size_t rows = len / 16;
	if((len & 0xf) != 0) {
		rows++;
	}
	size_t rowlen = 16 * 2 + 16;            // 32 hex digits + 16 spaces per row
	rowlen += 16;                           // ASCII characters per row
	rowlen += 10;                           // extra space for separators
	size_t alloc_size = rows * rowlen + 1;  // terminating NULL
	char *buf = LA_XCALLOC(alloc_size, sizeof(char));
	char *ptr = buf;
	size_t i = 0, j = 0;
	while(i < len) {
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				*ptr++ = hex[((data[j] >> 4) & 0xf)];
				*ptr++ = hex[data[j] & 0xf];
			} else {
				*ptr++ = ' ';
				*ptr++ = ' ';
			}
			*ptr++ = ' ';
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = ' ';
		*ptr++ = '|';
		for(j = i; j < i + 16; j++) {
			if(j < len) {
				if(data[j] < 32 || data[j] > 126) {
					*ptr++ = '.';
				} else {
					*ptr++ = data[j];
				}
			} else {
				*ptr++ = ' ';
			}
			if(j == i + 7) {
				*ptr++ = ' ';
			}
		}
		*ptr++ = '|';
		*ptr++ = '\n';
		i += 16;
	}
	return buf;
}

bool is_printable(uint8_t const *buf, uint32_t data_len) {
	if(buf == NULL || data_len == 0) {
		return false;
	}
	for(uint32_t i = 0; i < data_len; i++) {
		if((buf[i] >= 7 && buf[i] <= 13) ||
				(buf[i] >= 32 && buf[i] <= 126)) {
			// noop
		} else {
			la_debug_print(D_VERBOSE, "false due to character %u at position %u\n", buf[i], i);
			return false;
		}
	}
	la_debug_print(D_VERBOSE, "true\n");
	return true;
}

int la_strntouint16_t(char const *txt, int charcnt) {
	if(txt == NULL ||
			charcnt < 1 ||
			charcnt > 9 ||      // prevent overflowing int
			strnlen(txt, charcnt) < (size_t)charcnt)
	{
		return -1;
	}
	int ret = 0;
	int base = 1;
	int j;
	for(int i = 0; i < charcnt; i++) {
		j = charcnt - 1 - i;
		if(txt[j] < '0' || txt[j] > '9') {
			return -2;
		}
		ret += (txt[j] - '0') * base;
		base *= 10;
	}
	return ret;
}

// returns the length of the string ignoring the terminating
// newline characters
size_t chomped_strlen(char const *s) {
	char *p = strchr(s, '\0');
	size_t ret = p - s;
	while(--p >= s) {
		if(*p != '\n' && *p != '\r') {
			break;
		}
		ret--;
	}
	return ret;
}

// parse and perform basic sanitization of timestamp
// in YYMMDDHHMMSS format.
// Do not use strptime() - it's not available on WIN32
// Do not use sscanf() - it's too liberal on the input
// (we do not want whitespaces between fields, for example)
char *la_simple_strptime(char const *s, struct tm *t) {
	if(strspn(s, "0123456789") < 12) {
		return NULL;
	}
	t->tm_year = ATOI2(s[0],  s[1]) + 100;
	t->tm_mon  = ATOI2(s[2],  s[3]) - 1;
	t->tm_mday = ATOI2(s[4],  s[5]);
	t->tm_hour = ATOI2(s[6],  s[7]);
	t->tm_min  = ATOI2(s[8],  s[9]);
	t->tm_sec  = ATOI2(s[10], s[11]);
	t->tm_isdst = -1;
	if(t->tm_mon > 11 || t->tm_mday > 31 || t->tm_hour > 23 || t->tm_min > 59 || t->tm_sec > 59) {
		return NULL;
	}
	return (char *)s + 12;
}

la_octet_string *la_octet_string_new(void *buf, size_t len) {
	LA_NEW(la_octet_string, ostring);
	ostring->buf = buf;
	ostring->len = len;
	return ostring;
}

void la_octet_string_destroy(void *ostring_ptr) {
	if(ostring_ptr == NULL) {
		return;
	}
	la_octet_string *ostring = ostring_ptr;
	LA_XFREE(ostring->buf);
	LA_XFREE(ostring);
}

#ifndef HAVE_STRSEP
char *la_strsep(char **stringp, char const *delim) {
	char *start = *stringp;
	char *p;

	p = (start != NULL) ? strpbrk(start, delim) : NULL;
	if (p == NULL) {
		*stringp = NULL;
	} else {
		*p = '\0';
		*stringp = p + 1;
	}
	return start;
}
#endif

#ifndef HAVE_MEMMEM
void *memmem(void const *haystack, size_t haystack_len, void const *needle, size_t needle_len) {
    if (needle_len == 0) {
        return (void *)haystack;
    }
    if (needle_len > haystack_len) {
        return NULL;
    }

    void const *haystack_end = (char const *)haystack + haystack_len - needle_len;
    while (haystack <= haystack_end) {
        void const *match = memchr(haystack, *(char const *)needle, haystack_end - haystack + 1);
        if (match == NULL) {
            return NULL;
        }
        if (memcmp(match, needle, needle_len) == 0) {
            return (void *)match;
        }
        haystack = (char const *)match + 1;
    }
    return NULL;
}
#endif

#ifdef WITH_LIBXML2
void la_xml_errfunc_noop(void * ctx, char const *msg, ...) {
	(void)ctx;
	(void)msg;
}

xmlBufferPtr la_prettify_xml(char const *buf) {
	if(buf == NULL) {
		return NULL;
	}
	// Disables printing XML parser errors to stderr by setting error handler to noop.
	// Can't do this once in library constructor, because this is a per-thread setting.
	if(xmlGenericError != la_xml_errfunc_noop) {
		xmlGenericErrorFunc errfuncptr = la_xml_errfunc_noop;
		initGenericErrorDefaultFunc(&errfuncptr);
	}
	xmlDocPtr doc = xmlParseDoc((uint8_t const *)buf);
	if(doc == NULL) {
		return NULL;
	}
	xmlBufferPtr outbufptr = xmlBufferCreate();
	int result_len = xmlNodeDump(outbufptr, doc, xmlDocGetRootElement(doc), 0, 1);
	xmlFreeDoc(doc);
	if(result_len > 0) {
		return outbufptr;
	}
	xmlBufferFree(outbufptr);
	return NULL;
}
#endif

uint32_t la_reverse(uint32_t v, int numbits) {
	uint32_t r = v;                         // r will be reversed bits of v; first get LSB of v
	int s = sizeof(v) * CHAR_BIT - 1;       // extra shift needed at end

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}
	r <<= s;                                // shift when v's highest bits are zero
	r >>= 32 - numbits;
	return r;
}

// BASE64 decoder

static int32_t la_get_base64_idx(char c) {
    char const base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    for (int32_t i = 0; i < 64; i++) {
        if (base64_chars[i] == c) {
            return i;
        }
    }
	la_debug_print(D_VERBOSE, "char %hhd is invalid\n", c);
    return -1;
}

la_octet_string *la_base64_decode(char const *input, size_t input_len) {
	if(input == NULL || input_len == 0) {
		return NULL;
	}
	// Round off the input length to full 4-char blocks
	if((input_len & 3) != 0) {
		input_len &= ~3;
	}
    size_t decoded_len = (input_len * 3) / 4;

    if (input[input_len - 1] == '=') {
        decoded_len--;
    }
    if (input[input_len - 2] == '=') {
        decoded_len--;
    }

    uint8_t *output = LA_XCALLOC(decoded_len, sizeof(uint8_t));
    size_t output_idx = 0;
    size_t i = 0;

    while (i < input_len) {
        int32_t idx1 = la_get_base64_idx(input[i++]);
        if(idx1 < 0) goto fail;
        int32_t idx2 = la_get_base64_idx(input[i++]);
        if(idx2 < 0) goto fail;
        output[output_idx++] = (idx1 << 2) | (idx2 >> 4);

        if (output_idx >= decoded_len) {
            break;
        }

        int32_t idx3 = la_get_base64_idx(input[i++]);
        if(idx3 < 0) goto fail;
        output[output_idx++] = (idx2 << 4) | (idx3 >> 2);

        if (output_idx >= decoded_len) {
            break;
        }

        int32_t idx4 = la_get_base64_idx(input[i++]);
        if(idx4 < 0) goto fail;
        output[output_idx++] = (idx3 << 6) | idx4;
    }
	return la_octet_string_new(output, decoded_len);
fail:
	la_debug_print(D_VERBOSE, "Decoding failed at position %zu\n", i);
	LA_XFREE(output);
	return NULL;
}

// ZLIB decompressor

#ifdef WITH_ZLIB

#define MAX_INFLATED_LEN (1<<20)

la_inflate_result la_inflate(uint8_t const *buf, int in_len) {
	la_assert(buf != NULL);
	la_assert(in_len > 0);

	z_stream stream;
	memset(&stream, 0, sizeof(stream));
	la_inflate_result result;
	memset(&result, 0, sizeof(result));

	int ret = inflateInit2(&stream, -15);   // raw deflate with max window size
	if(ret != Z_OK) {
		la_debug_print(D_ERROR, "inflateInit failed: %d\n", ret);
		goto end;
	}
	stream.avail_in = (uInt)in_len;
	stream.next_in = (uint8_t *)buf;
	int chunk_len = 4 * in_len;
	int out_len = chunk_len;                // rough initial approximation
	uint8_t *outbuf = LA_XCALLOC(out_len, sizeof(uint8_t));
	stream.next_out = outbuf;
	stream.avail_out = out_len;

	while((ret = inflate(&stream, Z_FINISH)) == Z_BUF_ERROR) {
		la_debug_print(D_INFO, "Z_BUF_ERROR, avail_in=%u avail_out=%u\n", stream.avail_in, stream.avail_out);
		if(stream.avail_out == 0) {
			// Not enough output space
			int new_len = out_len + chunk_len;
			la_debug_print(D_INFO, "outbuf grow: %d -> %d\n", out_len, new_len);
			if(new_len > MAX_INFLATED_LEN) {
				// Do not go overboard with memory usage
				la_debug_print(D_ERROR, "new_len too large: %d > %d\n", new_len, MAX_INFLATED_LEN);
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
	la_debug_print(D_INFO, "zlib ret=%d avail_out=%u total_out=%lu\n", ret, stream.avail_out, stream.total_out);
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
#endif  // WITH_ZLIB

char *la_json_pretty_print(char const *json_string) {
	la_assert(json_string);

	bool prettify_json = false;
	(void)la_config_get_bool("prettify_json", &prettify_json);
	if(prettify_json == false) {
		return NULL;
	}

	char *result = NULL;
#ifdef WITH_JANSSON
	json_error_t err;
	json_t *root = json_loads(json_string, 0, &err);
	if(root) {
		result = json_dumps(root, JSON_INDENT(1) | JSON_REAL_PRECISION(6));
		if(result == NULL) {
			la_debug_print(D_INFO, "json_dumps() did not return any result\n");
		}
	} else {
		la_debug_print(D_ERROR, "Failed to decode JSON string at position %d: %s\n",
				err.position, err.text);
	}
	json_decref(root);
#else
	LA_UNUSED(json_string);
#endif // WITH_JANSSON
	return result;
}

