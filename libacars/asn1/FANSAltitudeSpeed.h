/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "../../../libacars.asn1/fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSAltitudeSpeed_H_
#define	_FANSAltitudeSpeed_H_


#include "asn_application.h"

/* Including external dependencies */
#include "FANSAltitude.h"
#include "FANSSpeed.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FANSAltitudeSpeed */
typedef struct FANSAltitudeSpeed {
	FANSAltitude_t	 altitude;
	FANSSpeed_t	 speed;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} FANSAltitudeSpeed_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSAltitudeSpeed;

#ifdef __cplusplus
}
#endif

#endif	/* _FANSAltitudeSpeed_H_ */
#include "asn_internal.h"
