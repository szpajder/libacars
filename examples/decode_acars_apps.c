/*
 *  decode_acars_apps - an example decoder for ACARS applications
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>            /* true */
#include <stdio.h>              /* printf(), fprintf(), fgets() */
#include <stdlib.h>             /* getenv() */
#include <string.h>             /* strcmp(), strchr(), strlen() */
#include <libacars/libacars.h>  /* la_proto_node, la_msg_dir,
                                   la_proto_tree_format_text(), la_proto_tree_destroy() */
#include <libacars/acars.h>     /* la_acars_decode_apps() */
#include <libacars/vstring.h>   /* la_vstring, la_vstring_destroy() */

void usage() {
	fprintf(stderr,
			"decode_acars_apps - an example decoder of ACARS applications\n"
			"(c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>\n\n"
			"Usage:\n\n"
			"To decode a single message from command line:\n\n"
			"\t./decode_acars_apps <direction> <acars_label> <acars_message_text>\n\n"
			"where <direction> is one of:\n"
			"\tu - means \"uplink\" (ground-to-air message)\n"
			"\td - means \"downlink\" (air-to-ground message)\n\n"
			"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
			"special shell characters, like '#'.\n\n"
			"Example: ./decode_acars_apps d B6 '/BOMASAI.ADS.VT-ANB072501A070A988CA73248F0E5DC10200000F5EE1ABC000102B885E0A19F5'\n\n"
			"To decode multiple messages from a text file:\n\n"
			"1. Prepare a file with multiple messages, one per line. Precede each line\n"
			"   with 'u' or 'd' (to indicate message direction) and a space. Direction\n"
			"   indicator must appear as a first character on the line (no preceding\n"
			"   spaces please). Example:\n\n"
			"u AA /AKLCDYA.AT1.9M-MTB215B659D84995674293583561CB9906744E9AF40F9EB\n"
			"d B6 /CTUE1YA.ADS.HB-JNB1424AB686D9308CA2EBA1D0D24A2C06C1B48CA004A248050667908CA004BF6\n"
			"d BA /MSTEC7X.AT1.VT-ANE21409DCC3DD03BB52350490502B2E5129D5A15692BA009A08892E7CC831E210A4C06EEBC28B1662BC02360165C80E1F7\n"
			"u H1 - #MD/AA ATLTWXA.CR1.N856DN203A3AA8E5C1A9323EDD\n"
			"d SA 0EV192001VS\n"
			"d H1 #T2BT-3![[mS0L8ZeIK0?J|EDDF\n\n"
			"2. Run decode_acars_apps and pipe the the file contents on standard input:\n\n"
			"\t./decode_acars_apps < messages.txt\n\n"
			"Note: ACARS label is used to identify the application (protocol) carried in\n"
			"the message text. Messages with an incorrect label value won't be decoded,\n"
			"because the library won't know, which decoder to execute.\n\nACARS label cheat sheet:\n"
			"- ARINC 622 ATS applications (ADS-C, CPDLC): A6, AA, B6, BA, H1\n"
			"- Media Advisory: SA\n"
			"- MIAM: MA (or H1 - if prefixed with a sublabel)\n\n"
			"decode_acars_apps produces human-readable text output by default.\n"
			"To switch to JSON output, set LA_JSON environment variable to any value.\n"
			);
}

bool json = false;

void parse(char *label, char *txt, la_msg_dir msg_dir) {
	int offset = la_acars_extract_sublabel_and_mfi(label, msg_dir, txt,
			strlen(txt), NULL, NULL);
	la_proto_node *node = la_acars_decode_apps(label, txt + offset, msg_dir);
	printf("%s\n", txt);
	if(node != NULL) {
		la_vstring *vstr = NULL;
		if(json) {
			vstr = la_proto_tree_format_json(NULL, node);
		} else {
			vstr = la_proto_tree_format_text(NULL, node);
		}
		fwrite(vstr->str, sizeof(char), vstr->len, stdout);
		fputc('\n', stdout);
		la_vstring_destroy(vstr, true);
	}
	la_proto_tree_destroy(node);
}

int main(int argc, char **argv) {
	la_msg_dir msg_dir = LA_MSG_DIR_UNKNOWN;
	char *dump_asn1 = getenv("ENABLE_ASN1_DUMPS");
	if(dump_asn1 != NULL && !strcmp(dump_asn1, "1")) {
		la_config_set_bool("dump_asn1", true);
	}
	if(getenv("LA_JSON") != NULL) {
		json = true;
	}
	la_config_set_bool("prettify_xml", true);
	la_config_set_bool("prettify_json", true);
	if(argc > 1 && !strcmp(argv[1], "-h")) {
		usage();
		exit(0);
	} else if(argc < 4) {
		fprintf(stderr,
				"No command line options found - reading messages from standard input.\n"
				"Use '-h' option for help.\n"
			   );

		char buf[10240], label[3];
		for(;;) {
			memset(buf, 0, sizeof(buf));
			msg_dir = LA_MSG_DIR_UNKNOWN;
			if(fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			char *end = strchr(buf, '\n');
			if(end)
				*end = '\0';
			if(strlen(buf) >= 6 &&
					(buf[0] == 'u' || buf[0] == 'd') &&
					buf[1] == ' ' &&
					buf[2] != ' ' &&
					buf[3] != ' ' &&
					buf[4] == ' '
			  ) {
				if(buf[0] == 'u')
					msg_dir = LA_MSG_DIR_GND2AIR;
				else if(buf[0] == 'd')
					msg_dir = LA_MSG_DIR_AIR2GND;
				label[0] = buf[2];
				label[1] = buf[3];
				label[2] = '\0';
				parse(label, buf + 5, msg_dir);
			} else {
				fprintf(stderr, "Garbled input: expecting 'u|d label acars_message_text'\n");
				continue;
			}
		}
	} else if(argc == 4) {
		if(argv[1][0] == 'u')
			msg_dir = LA_MSG_DIR_GND2AIR;
		else if(argv[1][0] == 'd')
			msg_dir = LA_MSG_DIR_AIR2GND;
		else {
			fprintf(stderr,
					"Incorrect message direction\n"
					"Use '-h' option for help\n"
				   );
			exit(1);
		}
		if(strlen(argv[2]) != 2) {
			fprintf(stderr,
					"Label field must have a length of 2 characters\n"
					"Use '-h' option for help\n"
				   );
			exit(1);
		}
		parse(argv[2], argv[3], msg_dir);
	} else {
		fprintf(stderr, "Invalid command line options\n\n");
		usage();
		exit(1);
	}
}
