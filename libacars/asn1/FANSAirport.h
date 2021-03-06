/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "../../../libacars.asn1/fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSAirport_H_
#define	_FANSAirport_H_


#include "asn_application.h"

/* Including external dependencies */
#include "IA5String.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FANSAirport */
typedef IA5String_t	 FANSAirport_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSAirport;
asn_struct_free_f FANSAirport_free;
asn_struct_print_f FANSAirport_print;
asn_constr_check_f FANSAirport_constraint;
ber_type_decoder_f FANSAirport_decode_ber;
der_type_encoder_f FANSAirport_encode_der;
xer_type_decoder_f FANSAirport_decode_xer;
xer_type_encoder_f FANSAirport_encode_xer;
per_type_decoder_f FANSAirport_decode_uper;
per_type_encoder_f FANSAirport_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _FANSAirport_H_ */
#include "asn_internal.h"
