/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "../../../libacars.asn1/fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSPlaceBearingPlaceBearing_H_
#define	_FANSPlaceBearingPlaceBearing_H_


#include "asn_application.h"

/* Including external dependencies */
#include "asn_SEQUENCE_OF.h"
#include "constr_SEQUENCE_OF.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct FANSPlaceBearing;

/* FANSPlaceBearingPlaceBearing */
typedef struct FANSPlaceBearingPlaceBearing {
	A_SEQUENCE_OF(struct FANSPlaceBearing) list;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} FANSPlaceBearingPlaceBearing_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSPlaceBearingPlaceBearing;

#ifdef __cplusplus
}
#endif

/* Referred external types */
#include "FANSPlaceBearing.h"

#endif	/* _FANSPlaceBearingPlaceBearing_H_ */
#include "asn_internal.h"
