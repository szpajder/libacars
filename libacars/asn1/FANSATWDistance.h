/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "FANSACTwoWayDataLinkCommunications"
 * 	found in "../../../libacars.asn1/fans-cpdlc.asn1"
 * 	`asn1c -fcompound-names -fincludes-quoted -gen-PER`
 */

#ifndef	_FANSATWDistance_H_
#define	_FANSATWDistance_H_


#include "asn_application.h"

/* Including external dependencies */
#include "FANSATWDistanceTolerance.h"
#include "FANSDistance.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FANSATWDistance */
typedef struct FANSATWDistance {
	FANSATWDistanceTolerance_t	 atwDistanceTolerance;
	FANSDistance_t	 distance;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} FANSATWDistance_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_FANSATWDistance;

#ifdef __cplusplus
}
#endif

#endif	/* _FANSATWDistance_H_ */
#include "asn_internal.h"
