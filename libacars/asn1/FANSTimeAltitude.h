/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "../../../libacars.asn1/fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSTimeAltitude_H_
#define	_FANSTimeAltitude_H_


#include "asn_application.h"

/* Including external dependencies */
#include "FANSTime.h"
#include "FANSAltitude.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FANSTimeAltitude */
typedef struct FANSTimeAltitude {
	FANSTime_t	 time;
	FANSAltitude_t	 altitude;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} FANSTimeAltitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSTimeAltitude;

#ifdef __cplusplus
}
#endif

#endif	/* _FANSTimeAltitude_H_ */
#include "asn_internal.h"
