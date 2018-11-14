set(ASN_MODULE_SOURCES
	FANSATCUplinkMessage.c
	FANSATCUplinkMsgElementIdSequence.c
	FANSATCMessageHeader.c
	FANSMsgIdentificationNumber.c
	FANSMsgReferenceNumber.c
	FANSTimestamp.c
	FANSTimehours.c
	FANSTimeminutes.c
	FANSTimeSeconds.c
	FANSATCUplinkMsgElementId.c
	FANSATCDownlinkMessage.c
	FANSATCDownlinkMsgElementIdSequence.c
	FANSATCDownlinkMsgElementId.c
	FANSAirport.c
	FANSAltimeter.c
	FANSAltimeterEnglish.c
	FANSAltimeterMetric.c
	FANSAltitude.c
	FANSAltitudeQNH.c
	FANSAltitudeQNHMeters.c
	FANSAltitudeQFE.c
	FANSAltitudeQFEMeters.c
	FANSAltitudeGNSSFeet.c
	FANSAltitudeGNSSMeters.c
	FANSAltitudeFlightLevel.c
	FANSAltitudeFlightLevelMetric.c
	FANSAltitudePosition.c
	FANSAltitudeAltitude.c
	FANSAltitudeSpeed.c
	FANSAltitudeSpeedSpeed.c
	FANSAltitudeTime.c
	FANSATISCode.c
	FANSATWAltitude.c
	FANSATWAltitudeTolerance.c
	FANSBeaconCode.c
	FANSBeaconCodeOctalDigit.c
	FANSDegrees.c
	FANSDegreesMagnetic.c
	FANSDegreesTrue.c
	FANSDirection.c
	FANSDirectionDegrees.c
	FANSDistanceOffset.c
	FANSDistanceOffsetNm.c
	FANSDistanceOffsetKm.c
	FANSDistanceOffsetDirection.c
	FANSErrorInformation.c
	FANSFixName.c
	FANSFreeText.c
	FANSFrequency.c
	FANSFrequencyhf.c
	FANSFrequencyvhf.c
	FANSFrequencyuhf.c
	FANSFrequencysatchannel.c
	FANSHoldClearance.c
	FANSICAOUnitName.c
	FANSICAOFacilityIdentification.c
	FANSICAOfacilityDesignation.c
	FANSICAOFacilityName.c
	FANSICAOFacilityFunction.c
	FANSICAOUnitNameFrequency.c
	FANSICAOFacilityDesignationTp4Table.c
	FANSLatitudeLongitude.c
	FANSLatitudeLongitudeSequence.c
	FANSLatitude.c
	FANSLatitudeDegrees.c
	FANSMinutesLatLon.c
	FANSLatitudeDirection.c
	FANSLongitude.c
	FANSLongitudeDegrees.c
	FANSLongitudeDirection.c
	FANSLegType.c
	FANSLegDistance.c
	FANSLegDistanceEnglish.c
	FANSLegDistanceMetric.c
	FANSLegTime.c
	FANSPlaceBearingDistance.c
	FANSDistance.c
	FANSDistanceNm.c
	FANSDistanceKm.c
	FANSPosition.c
	FANSNavaid.c
	FANSPositionAltitude.c
	FANSPositionAltitudeAltitude.c
	FANSPositionAltitudeSpeed.c
	FANSPositionDegrees.c
	FANSPositionDistanceOffsetDirection.c
	FANSPositionICAOUnitNameFrequency.c
	FANSPositionPosition.c
	FANSPositionProcedureName.c
	FANSPositionReport.c
	FANSPositionCurrent.c
	FANSTimeAtPositionCurrent.c
	FANSFixNext.c
	FANSTimeEtaAtFixNext.c
	FANSFixNextPlusOne.c
	FANSTimeEtaDestination.c
	FANSTemperature.c
	FANSTemperatureC.c
	FANSTemperatureF.c
	FANSWinds.c
	FANSWindDirection.c
	FANSWindSpeed.c
	FANSWindSpeedEnglish.c
	FANSWindSpeedMetric.c
	FANSTurbulence.c
	FANSIcing.c
	FANSVerticalChange.c
	FANSVerticalDirection.c
	FANSTrackAngle.c
	FANSTrueheading.c
	FANSSupplementaryInformation.c
	FANSReportedWaypointPosition.c
	FANSReportedWaypointTime.c
	FANSReportedWaypointAltitude.c
	FANSPositionRouteClearance.c
	FANSPositionSpeed.c
	FANSPositionSpeedSpeed.c
	FANSPositionTime.c
	FANSPositionTimeAltitude.c
	FANSPositionTimeTime.c
	FANSPredepartureClearance.c
	FANSAircraftFlightIdentification.c
	FANSAircraftType.c
	FANSAircraftEquipmentCode.c
	FANSCOMNAVApproachEquipmentAvailable.c
	FANSCOMNAVEquipmentStatus.c
	FANSCOMNAVEquipmentStatusSequence.c
	FANSSSREquipmentAvailable.c
	FANSTimeDepartureEdct.c
	FANSAltitudeRestriction.c
	FANSFrequencyDeparture.c
	FANSPDCrevision.c
	FANSProcedureName.c
	FANSProcedureType.c
	FANSProcedure.c
	FANSProcedureTransition.c
	FANSRemainingFuelRemainingSouls.c
	FANSRemainingFuel.c
	FANSRemainingSouls.c
	FANSRouteClearance.c
	FANSAirportDeparture.c
	FANSAirportDestination.c
	FANSRunwayDeparture.c
	FANSRunway.c
	FANSRunwayDirection.c
	FANSRunwayConfiguration.c
	FANSProcedureDeparture.c
	FANSRunwayArrival.c
	FANSProcedureApproach.c
	FANSProcedureArrival.c
	FANSAirwayIntercept.c
	FANSRouteInformation.c
	FANSRouteInformationSequence.c
	FANSPublishedIdentifier.c
	FANSPlaceBearing.c
	FANSPlaceBearingPlaceBearing.c
	FANSAirwayIdentifier.c
	FANSTrackDetail.c
	FANSTrackName.c
	FANSATWAlongTrackWaypointSequence.c
	FANSInterceptCourseFromSequence.c
	FANSHoldatwaypointSequence.c
	FANSWaypointSpeedAltitudesequence.c
	FANSRTARequiredTimeArrivalSequence.c
	FANSRouteInformationAdditional.c
	FANSATWAltitudeSequence.c
	FANSATWAlongTrackWaypoint.c
	FANSATWDistance.c
	FANSATWDistanceTolerance.c
	FANSReportingPoints.c
	FANSLatLonReportingPoints.c
	FANSLatitudeReportingPoints.c
	FANSLongitudeReportingPoints.c
	FANSDegreeIncrement.c
	FANSInterceptCourseFrom.c
	FANSInterceptCourseFromSelection.c
	FANSHoldatwaypointSpeedLow.c
	FANSHoldatwaypointSpeedHigh.c
	FANSEFCtime.c
	FANSHoldatwaypoint.c
	FANSWaypointSpeedAltitude.c
	FANSRTARequiredTimeArrival.c
	FANSRTATime.c
	FANSTimeTolerance.c
	FANSRTATolerance.c
	FANSSpeed.c
	FANSSpeedIndicated.c
	FANSSpeedIndicatedMetric.c
	FANSSpeedTrue.c
	FANSSpeedTrueMetric.c
	FANSSpeedGround.c
	FANSSpeedGroundMetric.c
	FANSSpeedMach.c
	FANSSpeedMachLarge.c
	FANSSpeedSpeed.c
	FANSTime.c
	FANSTimeAltitude.c
	FANSTimeDistanceOffsetDirection.c
	FANSTimeDistanceToFromPosition.c
	FANSTimeTime.c
	FANSToFrom.c
	FANSTimeICAOunitnameFrequency.c
	FANSTimePosition.c
	FANSTimePositionAltitude.c
	FANSTimePositionAltitudeSpeed.c
	FANSTimeSpeed.c
	FANSTimeSpeedSpeed.c
	FANSToFromPosition.c
	FANSTp4table.c
	FANSVersionNumber.c
	FANSVerticalRate.c
	FANSVerticalRateEnglish.c
	FANSVerticalRateMetric.c
)

set (ASN_MODULE_HEADERS
	FANSATCUplinkMessage.h
	FANSATCUplinkMsgElementIdSequence.h
	FANSATCMessageHeader.h
	FANSMsgIdentificationNumber.h
	FANSMsgReferenceNumber.h
	FANSTimestamp.h
	FANSTimehours.h
	FANSTimeminutes.h
	FANSTimeSeconds.h
	FANSATCUplinkMsgElementId.h
	FANSATCDownlinkMessage.h
	FANSATCDownlinkMsgElementIdSequence.h
	FANSATCDownlinkMsgElementId.h
	FANSAirport.h
	FANSAltimeter.h
	FANSAltimeterEnglish.h
	FANSAltimeterMetric.h
	FANSAltitude.h
	FANSAltitudeQNH.h
	FANSAltitudeQNHMeters.h
	FANSAltitudeQFE.h
	FANSAltitudeQFEMeters.h
	FANSAltitudeGNSSFeet.h
	FANSAltitudeGNSSMeters.h
	FANSAltitudeFlightLevel.h
	FANSAltitudeFlightLevelMetric.h
	FANSAltitudePosition.h
	FANSAltitudeAltitude.h
	FANSAltitudeSpeed.h
	FANSAltitudeSpeedSpeed.h
	FANSAltitudeTime.h
	FANSATISCode.h
	FANSATWAltitude.h
	FANSATWAltitudeTolerance.h
	FANSBeaconCode.h
	FANSBeaconCodeOctalDigit.h
	FANSDegrees.h
	FANSDegreesMagnetic.h
	FANSDegreesTrue.h
	FANSDirection.h
	FANSDirectionDegrees.h
	FANSDistanceOffset.h
	FANSDistanceOffsetNm.h
	FANSDistanceOffsetKm.h
	FANSDistanceOffsetDirection.h
	FANSErrorInformation.h
	FANSFixName.h
	FANSFreeText.h
	FANSFrequency.h
	FANSFrequencyhf.h
	FANSFrequencyvhf.h
	FANSFrequencyuhf.h
	FANSFrequencysatchannel.h
	FANSHoldClearance.h
	FANSICAOUnitName.h
	FANSICAOFacilityIdentification.h
	FANSICAOfacilityDesignation.h
	FANSICAOFacilityName.h
	FANSICAOFacilityFunction.h
	FANSICAOUnitNameFrequency.h
	FANSICAOFacilityDesignationTp4Table.h
	FANSLatitudeLongitude.h
	FANSLatitudeLongitudeSequence.h
	FANSLatitude.h
	FANSLatitudeDegrees.h
	FANSMinutesLatLon.h
	FANSLatitudeDirection.h
	FANSLongitude.h
	FANSLongitudeDegrees.h
	FANSLongitudeDirection.h
	FANSLegType.h
	FANSLegDistance.h
	FANSLegDistanceEnglish.h
	FANSLegDistanceMetric.h
	FANSLegTime.h
	FANSPlaceBearingDistance.h
	FANSDistance.h
	FANSDistanceNm.h
	FANSDistanceKm.h
	FANSPosition.h
	FANSNavaid.h
	FANSPositionAltitude.h
	FANSPositionAltitudeAltitude.h
	FANSPositionAltitudeSpeed.h
	FANSPositionDegrees.h
	FANSPositionDistanceOffsetDirection.h
	FANSPositionICAOUnitNameFrequency.h
	FANSPositionPosition.h
	FANSPositionProcedureName.h
	FANSPositionReport.h
	FANSPositionCurrent.h
	FANSTimeAtPositionCurrent.h
	FANSFixNext.h
	FANSTimeEtaAtFixNext.h
	FANSFixNextPlusOne.h
	FANSTimeEtaDestination.h
	FANSTemperature.h
	FANSTemperatureC.h
	FANSTemperatureF.h
	FANSWinds.h
	FANSWindDirection.h
	FANSWindSpeed.h
	FANSWindSpeedEnglish.h
	FANSWindSpeedMetric.h
	FANSTurbulence.h
	FANSIcing.h
	FANSVerticalChange.h
	FANSVerticalDirection.h
	FANSTrackAngle.h
	FANSTrueheading.h
	FANSSupplementaryInformation.h
	FANSReportedWaypointPosition.h
	FANSReportedWaypointTime.h
	FANSReportedWaypointAltitude.h
	FANSPositionRouteClearance.h
	FANSPositionSpeed.h
	FANSPositionSpeedSpeed.h
	FANSPositionTime.h
	FANSPositionTimeAltitude.h
	FANSPositionTimeTime.h
	FANSPredepartureClearance.h
	FANSAircraftFlightIdentification.h
	FANSAircraftType.h
	FANSAircraftEquipmentCode.h
	FANSCOMNAVApproachEquipmentAvailable.h
	FANSCOMNAVEquipmentStatus.h
	FANSCOMNAVEquipmentStatusSequence.h
	FANSSSREquipmentAvailable.h
	FANSTimeDepartureEdct.h
	FANSAltitudeRestriction.h
	FANSFrequencyDeparture.h
	FANSPDCrevision.h
	FANSProcedureName.h
	FANSProcedureType.h
	FANSProcedure.h
	FANSProcedureTransition.h
	FANSRemainingFuelRemainingSouls.h
	FANSRemainingFuel.h
	FANSRemainingSouls.h
	FANSRouteClearance.h
	FANSAirportDeparture.h
	FANSAirportDestination.h
	FANSRunwayDeparture.h
	FANSRunway.h
	FANSRunwayDirection.h
	FANSRunwayConfiguration.h
	FANSProcedureDeparture.h
	FANSRunwayArrival.h
	FANSProcedureApproach.h
	FANSProcedureArrival.h
	FANSAirwayIntercept.h
	FANSRouteInformation.h
	FANSRouteInformationSequence.h
	FANSPublishedIdentifier.h
	FANSPlaceBearing.h
	FANSPlaceBearingPlaceBearing.h
	FANSAirwayIdentifier.h
	FANSTrackDetail.h
	FANSTrackName.h
	FANSATWAlongTrackWaypointSequence.h
	FANSInterceptCourseFromSequence.h
	FANSHoldatwaypointSequence.h
	FANSWaypointSpeedAltitudesequence.h
	FANSRTARequiredTimeArrivalSequence.h
	FANSRouteInformationAdditional.h
	FANSATWAltitudeSequence.h
	FANSATWAlongTrackWaypoint.h
	FANSATWDistance.h
	FANSATWDistanceTolerance.h
	FANSReportingPoints.h
	FANSLatLonReportingPoints.h
	FANSLatitudeReportingPoints.h
	FANSLongitudeReportingPoints.h
	FANSDegreeIncrement.h
	FANSInterceptCourseFrom.h
	FANSInterceptCourseFromSelection.h
	FANSHoldatwaypointSpeedLow.h
	FANSHoldatwaypointSpeedHigh.h
	FANSEFCtime.h
	FANSHoldatwaypoint.h
	FANSWaypointSpeedAltitude.h
	FANSRTARequiredTimeArrival.h
	FANSRTATime.h
	FANSTimeTolerance.h
	FANSRTATolerance.h
	FANSSpeed.h
	FANSSpeedIndicated.h
	FANSSpeedIndicatedMetric.h
	FANSSpeedTrue.h
	FANSSpeedTrueMetric.h
	FANSSpeedGround.h
	FANSSpeedGroundMetric.h
	FANSSpeedMach.h
	FANSSpeedMachLarge.h
	FANSSpeedSpeed.h
	FANSTime.h
	FANSTimeAltitude.h
	FANSTimeDistanceOffsetDirection.h
	FANSTimeDistanceToFromPosition.h
	FANSTimeTime.h
	FANSToFrom.h
	FANSTimeICAOunitnameFrequency.h
	FANSTimePosition.h
	FANSTimePositionAltitude.h
	FANSTimePositionAltitudeSpeed.h
	FANSTimeSpeed.h
	FANSTimeSpeedSpeed.h
	FANSToFromPosition.h
	FANSTp4table.h
	FANSVersionNumber.h
	FANSVerticalRate.h
	FANSVerticalRateEnglish.h
	FANSVerticalRateMetric.h
)

list (APPEND ASN_MODULE_HEADERS BOOLEAN.h)
list (APPEND ASN_MODULE_SOURCES BOOLEAN.c)
list (APPEND ASN_MODULE_HEADERS INTEGER.h)
list (APPEND ASN_MODULE_HEADERS NativeEnumerated.h)
list (APPEND ASN_MODULE_HEADERS IA5String.h)
list (APPEND ASN_MODULE_SOURCES IA5String.c)
list (APPEND ASN_MODULE_SOURCES INTEGER.c)
list (APPEND ASN_MODULE_HEADERS NULL.h)
list (APPEND ASN_MODULE_SOURCES NULL.c)
list (APPEND ASN_MODULE_SOURCES NativeEnumerated.c)
list (APPEND ASN_MODULE_HEADERS NativeInteger.h)
list (APPEND ASN_MODULE_SOURCES NativeInteger.c)
list (APPEND ASN_MODULE_HEADERS NumericString.h)
list (APPEND ASN_MODULE_SOURCES NumericString.c)
list (APPEND ASN_MODULE_HEADERS asn_SEQUENCE_OF.h)
list (APPEND ASN_MODULE_SOURCES asn_SEQUENCE_OF.c)
list (APPEND ASN_MODULE_HEADERS asn_SET_OF.h)
list (APPEND ASN_MODULE_SOURCES asn_SET_OF.c)
list (APPEND ASN_MODULE_HEADERS constr_CHOICE.h)
list (APPEND ASN_MODULE_SOURCES constr_CHOICE.c)
list (APPEND ASN_MODULE_HEADERS constr_SEQUENCE.h)
list (APPEND ASN_MODULE_SOURCES constr_SEQUENCE.c)
list (APPEND ASN_MODULE_HEADERS constr_SEQUENCE_OF.h)
list (APPEND ASN_MODULE_SOURCES constr_SEQUENCE_OF.c)
list (APPEND ASN_MODULE_HEADERS constr_SET_OF.h)
list (APPEND ASN_MODULE_SOURCES constr_SET_OF.c)
list (APPEND ASN_MODULE_HEADERS asn_application.h)
list (APPEND ASN_MODULE_HEADERS asn_system.h)
list (APPEND ASN_MODULE_HEADERS asn_codecs.h)
list (APPEND ASN_MODULE_HEADERS asn_internal.h)
list (APPEND ASN_MODULE_HEADERS OCTET_STRING.h)
list (APPEND ASN_MODULE_SOURCES OCTET_STRING.c)
list (APPEND ASN_MODULE_HEADERS BIT_STRING.h)
list (APPEND ASN_MODULE_SOURCES BIT_STRING.c)
list (APPEND ASN_MODULE_SOURCES asn_codecs_prim.c)
list (APPEND ASN_MODULE_HEADERS asn_codecs_prim.h)
list (APPEND ASN_MODULE_HEADERS ber_tlv_length.h)
list (APPEND ASN_MODULE_SOURCES ber_tlv_length.c)
list (APPEND ASN_MODULE_HEADERS ber_tlv_tag.h)
list (APPEND ASN_MODULE_SOURCES ber_tlv_tag.c)
list (APPEND ASN_MODULE_HEADERS ber_decoder.h)
list (APPEND ASN_MODULE_SOURCES ber_decoder.c)
list (APPEND ASN_MODULE_HEADERS der_encoder.h)
list (APPEND ASN_MODULE_SOURCES der_encoder.c)
list (APPEND ASN_MODULE_HEADERS constr_TYPE.h)
list (APPEND ASN_MODULE_SOURCES constr_TYPE.c)
list (APPEND ASN_MODULE_HEADERS constraints.h)
list (APPEND ASN_MODULE_SOURCES constraints.c)
list (APPEND ASN_MODULE_HEADERS xer_support.h)
list (APPEND ASN_MODULE_SOURCES xer_support.c)
list (APPEND ASN_MODULE_HEADERS xer_decoder.h)
list (APPEND ASN_MODULE_SOURCES xer_decoder.c)
list (APPEND ASN_MODULE_HEADERS xer_encoder.h)
list (APPEND ASN_MODULE_SOURCES xer_encoder.c)
list (APPEND ASN_MODULE_HEADERS per_support.h)
list (APPEND ASN_MODULE_SOURCES per_support.c)
list (APPEND ASN_MODULE_HEADERS per_decoder.h)
list (APPEND ASN_MODULE_SOURCES per_decoder.c)
list (APPEND ASN_MODULE_HEADERS per_encoder.h)
list (APPEND ASN_MODULE_SOURCES per_encoder.c)
list (APPEND ASN_MODULE_HEADERS per_opentype.h)
list (APPEND ASN_MODULE_SOURCES per_opentype.c)

if (EMIT_ASN_DEBUG)
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEMIT_ASN_DEBUG=1")
endif()
add_library (asn1 STATIC ${ASN_MODULE_SOURCES})
set_property(TARGET asn1 PROPERTY POSITION_INDEPENDENT_CODE ON)

install (FILES ${ASN_MODULE_HEADERS} DESTINATION include/libacars/asn1)
