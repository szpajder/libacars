/*
 *  decpdlc - a simple FANS-1/A CPDLC message decoder
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libacars.h>
#include <cpdlc.h>
#include <util.h>		// la_slurp_hexstring()
#include <vstring.h>		// la_vstring

FILE *outf;

void usage() {
	fprintf(stderr,
	"decpdlc - an example program for decoding FANS-1/A CPDLC messages in ACARS text\n\n"
	"(c) 2018 Tomasz Lemiech <szpajder@gmail.com>\n"
	"Usage:\n\n"
	"To decode a single message from command line:\n\n"
	"\t./decpdlc <direction> <acars_message_text>\n\n"
	"where <direction> is one of:\n"
	"\tu - means \"uplink\" (ground-to-air message)\n"
	"\td - means \"downlink\" (air-to-ground message)\n\n"
	"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
	"special shell characters, like '#'.\n\n"
	"Example: ./decpdlc u '- #MD/AA ATLTWXA.CR1.N7881A203A44E8E5C1A932E80E'\n\n"
	"To decode multiple messages from a text file:\n\n"
	"1. Prepare a file with multiple messages, one per line. Precede each line\n"
	"   with 'u' or 'd' (to indicate message direction) and a space. Direction\n"
	"   indicator must appear as a first character on the line (no preceding\n"
	"   spaces please). Example:\n\n"
	"u /AKLCDYA.AT1.9M-MTB215B659D84995674293583561CB9906744E9AF40F9EB\n"
	"u /AKLCDYA.AT1.B-27372142ABDD84A7066418F583561CB9906744E9AF405DA1\n"
	"d /MSTEC7X.AT1.VT-ANE21409DCC3DD03BB52350490502B2E5129D5A15692BA009A08892E7CC831E210A4C06EEBC28B1662BC02360165C80E1F7\n"
	"u - #MD/AA ATLTWXA.CR1.N856DN203A3AA8E5C1A9323EDD\n\n"
	"2. Run decpdlc and pipe the the file contents on standard input:\n\n"
	"\t./decpdlc < cpdlc_messages.txt\n\n"
	"Supported FANS-1/A message types: CR1, CC1, DR1, AT1\n");
}

void parse(char *txt, la_msg_dir msg_dir) {
	int msgid = 0;
//	dump_asn1 = 0;
	char *s = strstr(txt, ".AT1");
	if(s != NULL) {
		msgid = 1;
		goto msgid_set;
	}
	s = strstr(txt, ".CR1");
	if(s != NULL) {
		msgid = 1;
		goto msgid_set;
	}
	s = strstr(txt, ".CC1");
	if(s != NULL) {
		msgid = 1;
		goto msgid_set;
	}
	s = strstr(txt, ".DR1");
	if(s != NULL) {
		msgid = 1;
		goto msgid_set;
	}
msgid_set:
	if(msgid == 0) {
		fprintf(stderr, "not a FANS-1/A CPDLC message\n");
		return;
	}
	s += 4;
	if(strlen(s) < 7) {
		fprintf(stderr, "regnr not found\n");
		goto end;
	}
	s += 7;
	uint8_t *buf = NULL;
	size_t buflen = la_slurp_hexstring(s, &buf);
// cut off CRC
	if(buflen >= 2)
		buflen -= 2;
// FIXME: prevent overflow on buflen cast
	la_proto_node *node = la_cpdlc_parse(buf, (int)buflen, msg_dir);
end:
	fprintf(outf, "%s\n", txt);
	if(node != NULL) {
		la_vstring *vstr = la_proto_tree_format_text(NULL, node);
		fprintf(outf, "%s\n", vstr->str);
		la_vstring_destroy(vstr, true);
	}
	la_proto_tree_destroy(node);
	free(buf);
}

int main(int argc, char **argv) {
	la_msg_dir msg_dir = LA_MSG_DIR_UNKNOWN;
	outf = stdout;
	if(argc > 1 && !strcmp(argv[1], "-h")) {
		usage();
		exit(0);
	} else if(argc < 2) {
		fprintf(stderr,
			"No command line options found - reading messages from standard input.\n"
			"Use '-h' option for help.\n"
		);

		char buf[1024];
		for(;;) {
			memset(buf, 0, sizeof(buf));
			msg_dir = LA_MSG_DIR_UNKNOWN;
			if(fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			char *end = strchr(buf, '\n');
			if(end)
				*end = '\0';
			if(strlen(buf) < 3 || (buf[0] != 'u' && buf[0] != 'd') || buf[1] != ' ') {
				fprintf(stderr, "Garbled input: expecting 'u|d acars_message_text'\n");
				continue;
			}
			if(buf[0] == 'u')
				msg_dir = LA_MSG_DIR_GND2AIR;
			else if(buf[0] == 'd')
				msg_dir = LA_MSG_DIR_AIR2GND;
			parse(buf, msg_dir);
		}
	} else if(argc == 3) {
		if(argv[1][0] == 'u')
			msg_dir = LA_MSG_DIR_GND2AIR;
		else if(argv[1][0] == 'd')
			msg_dir = LA_MSG_DIR_AIR2GND;
		else {
			fprintf(stderr, "Invalid command line options\n\n");
			usage();
			exit(1);
		}
		parse(argv[2], msg_dir);
	} else {
		fprintf(stderr, "Invalid command line options\n\n");
		usage();
		exit(1);
	}
}
