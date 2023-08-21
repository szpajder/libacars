/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2023 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ASN1_FORMAT_CPDLC_H
#define LA_ASN1_FORMAT_CPDLC_H 1
#include <libacars/asn1/asn_application.h>      // asn_TYPE_descriptor_t
#include <libacars/vstring.h>                   // la_vstring
#include <libacars/dict.h>                      // la_dict
#include <libacars/asn1-util.h>                 // LA_ASN1_FORMATTER_FUNC

// asn1-format-cpdlc-text.c
LA_ASN1_FORMATTER_FUNC(la_asn1_output_cpdlc_as_text);
extern la_dict const FANSATCUplinkMsgElementId_labels[];
extern la_dict const FANSATCDownlinkMsgElementId_labels[];

// asn1-format-cpdlc-json.c
LA_ASN1_FORMATTER_FUNC(la_asn1_output_cpdlc_as_json);

#define LA_ASN1_CPDLC_TABLE_SIZE 209

#endif // !LA_ASN1_FORMAT_CPDLC_H
