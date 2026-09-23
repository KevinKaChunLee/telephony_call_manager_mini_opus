/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CALL_MANAGER_DISCONNECTED_REASON_H
#define CALL_MANAGER_DISCONNECTED_REASON_H

namespace OHOS {
namespace Telephony {
/**
 * @brief Indicates the causes of call disconnection.
 *
 * 3GPP TS 24.008
 * V17.4.0 10.5.4.11 Cause
 * The purpose of the cause information element is to describe the reason for
 * generating certain messages, to provide diagnostic information in the event
 * of procedural errors and to indicate the location of the cause originator.
 */
enum class DisconnectedReason : int32_t {
    /**
     * Indicates the call disconnect due to Unassigned (unallocated) number.
     */
    UNASSIGNED_NUMBER = 1,
    /**
     * Indicates the call disconnect due to No route to destination.
     */
    NO_ROUTE_TO_DESTINATION = 3,
    /**
     * Indicates the call disconnect due to Channel unacceptable.
     */
    CHANNEL_UNACCEPTABLE = 6,
    /**
     * Indicates the call disconnect due to Operator determined barring.
     */
    OPERATOR_DETERMINED_BARRING = 8,
    /**
     * Indicates the call disconnect due to Call completed elsewhere.
     */
    CALL_COMPLETED_ELSEWHERE = 13,
    /**
     * Indicates the call disconnect due to Normal call clearing.
     */
    NORMAL_CALL_CLEARING = 16,
    /**
     * Indicates the call disconnect due to User busy.
     */
    USER_BUSY = 17,
    /**
     * Indicates the call disconnect due to No user responding.
     */
    NO_USER_RESPONDING = 18,
    /**
     * Indicates the call disconnect due to User alerting, no answer.
     */
    USER_ALERTING_NO_ANSWER = 19,
    /**
     * Indicates the call disconnect due to Call rejected.
     */
    CALL_REJECTED = 21,
    /**
     * Indicates the call disconnect due to Number changed.
     */
    NUMBER_CHANGED = 22,
    /**
     * Indicates the call disconnect cause is Call rejected due to feature
     * at the destination.
     */
    CALL_REJECTED_DUE_TO_FEATURE_AT_THE_DESTINATION = 24,
    /**
     * Indicates the call disconnect due to Pre-emption.
     */
    FAILED_PRE_EMPTION = 25,
    /**
     * Indicates the call disconnect due to Non selected user clearing.
     */
    NON_SELECTED_USER_CLEARING = 26,
    /**
     * Indicates the call disconnect due to Destination out of order.
     */
    DESTINATION_OUT_OF_ORDER = 27,
    /**
     * Indicates the call disconnect due to Invalid number format
     * (incomplete number).
     */
    INVALID_NUMBER_FORMAT = 28,
    /**
     * Indicates the call disconnect due to Facility rejected.
     */
    FACILITY_REJECTED = 29,
    /**
     * Indicates the call disconnect due to Response to STATUS ENQUIRY.
     */
    RESPONSE_TO_STATUS_ENQUIRY = 30,
    /**
     * Indicates the call disconnect due to Normal, unspecified.
     */
    NORMAL_UNSPECIFIED = 31,
    /**
     * Indicates the call disconnect due to No circuit/channel available.
     */
    NO_CIRCUIT_CHANNEL_AVAILABLE = 34,
    /**
     * Indicates the call disconnect due to Network out of order.
     */
    NETWORK_OUT_OF_ORDER = 38,
    /**
     * Indicates the call disconnect due to Temporary failure.
     */
    TEMPORARY_FAILURE = 41,
    /**
     * Indicates the call disconnect due to Switching equipment congestion.
     */
    SWITCHING_EQUIPMENT_CONGESTION = 42,
    /**
     * Indicates the call disconnect due to Access information discarded.
     */
    ACCESS_INFORMATION_DISCARDED = 43,
    /**
     * Indicates the call disconnect due to requested circuit/channel
     * not available.
     */
    REQUEST_CIRCUIT_CHANNEL_NOT_AVAILABLE = 44,
    /**
     * Indicates the call disconnect due to Resources unavailable, unspecified.
     */
    RESOURCES_UNAVAILABLE_UNSPECIFIED = 47,
    /**
     * Indicates the call disconnect due to Quality of service unavailable.
     */
    QUALITY_OF_SERVICE_UNAVAILABLE = 49,
    /**
     * Indicates the call disconnect due to Requested facility not subscribed.
     */
    REQUESTED_FACILITY_NOT_SUBSCRIBED = 50,
    /**
     * Indicates the call disconnect due to Incoming calls barred within the CUG.
     */
    INCOMING_CALLS_BARRED_WITHIN_THE_CUG = 55,
    /**
     * Indicates the call disconnect due to Bearer capability not authorized.
     */
    BEARER_CAPABILITY_NOT_AUTHORIZED = 57,
    /**
     * Indicates the call disconnect due to Bearer capability not presently
     * available.
     */
    BEARER_CAPABILITY_NOT_PRESENTLY_AVAILABLE = 58,
    /**
     * Indicates the call disconnect due to Service or option not available,
     * unspecified.
     */
    SERVICE_OR_OPTION_NOT_AVAILABLE_UNSPECIFIED = 63,
    /**
     * Indicates the call disconnect due to Bearer service not implemented.
     */
    BEARER_SERVICE_NOT_IMPLEMENTED = 65,
    /**
     * Indicates the call disconnect due to ACM equal to or greater than ACMmax.
     */
    ACM_EQUALTO_OR_GREATE_THAN_ACMMAX = 68,
    /**
     * Indicates the call disconnect due to Requested facility not implemented.
     */
    REQUESTED_FACILITY_NOT_IMPLEMENTED = 69,
    /**
     * Indicates the call disconnect due to Only restricted digital information
     * bearer capability is available.
     */
    ONLY_RESTRICTED_DIGITAL_INFO_BEARER_CAPABILITY_IS_AVAILABLE = 70,
    /**
     * Indicates the call disconnect due to Service or option not implemented,
     * unspecified.
     */
    SERVICE_OR_OPTION_NOT_IMPLEMENTED_UNSPECIFIED = 79,
    /**
     * Indicates the call disconnect due to Invalid transaction identifier
     * value.
     */
    INVALID_TRANSACTION_IDENTIFIER_VALUE = 81,
    /**
     * Indicates the call disconnect due to User not member of CUG.
     */
    USER_NOT_MEMBER_OF_CUG = 87,
    /**
     * Indicates the call disconnect due to Incompatible destination.
     */
    INCOMPATIBLE_DESTINATION = 88,
    /**
     * Indicates the call disconnect due to Invalid transit network selection.
     */
    INVALID_TRANSIT_NETWORK_SELECTION = 91,
    /**
     * Indicates the call disconnect due to Semantically incorrect message.
     */
    SEMANTICALLY_INCORRECT_MESSAGE = 95,
    /**
     * Indicates the call disconnect due to Invalid mandatory information.
     */
    INVALID_MANDATORY_INFORMATION = 96,
    /**
     * Indicates the call disconnect due to Message type non-existent or not
     * implemented.
     */
    MESSAGE_TYPE_NON_EXISTENT_OR_NOT_IMPLEMENTED = 97,
    /**
     * Indicates the call disconnect due to Message type not compatible with
     * protocol state.
     */
    MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
    /**
     * Indicates the call disconnect due to Information element non-existent or
     * not implemented.
     */
    INFORMATION_ELEMENT_NON_EXISTENT_OR_NOT_IMPLEMENTED = 99,
    /**
     * Indicates the call disconnect due to Conditional IE error.
     */
    CONDITIONAL_IE_ERROR = 100,
    /**
     * Indicates the call disconnect due to Message not compatible with
     * protocol state.
     */
    MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
    /**
     * Indicates the call disconnect due to Recovery on timer expiry.
     */
    RECOVERY_ON_TIMER_EXPIRED = 102,
    /**
     * Indicates the call disconnect due to Protocol error, unspecified.
     */
    PROTOCOL_ERROR_UNSPECIFIED = 111,
    /**
     * Indicates the call disconnect due to Interworking, unspecified.
     */
    INTERWORKING_UNSPECIFIED = 127,
    /**
     * Indicates the call disconnect due to Call barred.
     */
    CALL_BARRED = 240,
    /**
     * Indicates the call disconnect due to FDN blocked.
     */
    FDN_BLOCKED = 241,
    /**
     * Indicates the call disconnect due to IMSI in VLR is unknown.
     */
    IMSI_UNKNOWN_IN_VLR = 242,
    /**
     * Indicates the call disconnect due to IMEI not accepted.
     */
    IMEI_NOT_ACCEPTED = 243,
    /**
     * Indicates the call disconnect due to dial modified to USSD.
     */
    DIAL_MODIFIED_TO_USSD = 244,
    /**
     * Indicates the call disconnect due to dial modified to SS.
     */
    DIAL_MODIFIED_TO_SS = 245,
    /**
     * Indicates the call disconnect due to dial modified to dial.
     */
    DIAL_MODIFIED_TO_DIAL = 246,
    /**
     * Indicates the call disconnect due to Radio off.
     */
    RADIO_OFF = 247,
    /**
     * Indicates the call disconnect due to out of service.
     */
    OUT_OF_SERVICE = 248,
    /**
     * Indicates the call disconnect due to invalid SIM.
     */
    NO_VALID_SIM = 249,
    /**
     * Indicates the call disconnect due to radio internal error in modem.
     */
    RADIO_INTERNAL_ERROR = 250,
    /**
     * Indicates the call disconnect due to network response timeout.
     */
    NETWORK_RESP_TIMEOUT = 251,
    /**
     * Indicates the call disconnect due to network reject.
     */
    NETWORK_REJECT = 252,
    /**
     * Indicates the call disconnect due to RRC connection failure. Eg.RACH
     */
    RADIO_ACCESS_FAILURE = 253,
    /**
     * Indicates the call disconnect due to radio link failure.
     */
    RADIO_LINK_FAILURE = 254,
    /**
     * Indicates the call disconnect due to poor coverage.
     */
    RADIO_LINK_LOST = 255,
    /**
     * Indicates the call disconnect due to Radio uplink failure.
     */
    RADIO_UPLINK_FAILURE = 256,
    /**
     * Indicates the call disconnect due to RRC connection setup failure.
     */
    RADIO_SETUP_FAILURE = 257,
    /**
     * Indicates the call disconnect due to RRC connection release, normal.
     */
    RADIO_RELEASE_NORMAL = 258,
    /**
     * Indicates the call disconnect due to RRC connection release, abnormal.
     */
    RADIO_RELEASE_ABNORMAL = 259,
    /**
     * Indicates the call disconnect due to Access class barring.
     */
    ACCESS_CLASS_BLOCKED = 260,
    /**
     * Indicates the call disconnect due to Explicit network detach.
     */
    NETWORK_DETACH = 261,
    /**
     * Indicates the call disconnect due to invalid parameter.
     */
    INVALID_PARAMETER = 1025,
    /**
     * Indicates the call disconnect due to sim not exit.
     */
    SIM_NOT_EXIT = 1026,
    /**
     * Indicates the call disconnect due to sim pin need.
     */
    SIM_PIN_NEED = 1027,
    /**
     * Indicates the call disconnect due to call not allow.
     */
    CALL_NOT_ALLOW = 1029,
    /**
     * Indicates the call disconnect due to sim invalid.
     */
    SIM_INVALID = 1045,
    /**
     * Indicates the call disconnect due to unknown error.
     */
    FAILED_UNKNOWN = 1279,
};

/**
 * @brief Indicates the ril causes of call disconnection.
 */
enum class RilDisconnectedReason : int32_t {
    /**
     * Indicates the invalid reason
     */
    DISCONNECTED_REASON_INVALID = -1,
    /**
     * Indicates the call disconnect due to normal of ril.
     */
    DISCONNECTED_REASON_NORMAL = 0,
    /**
    * Indicates the cs call is answered elsewhere.
    */
    DISCONNECTED_REASON_CS_CALL_ANSWERED_ELSEWHER = 13,
    /**
     * Indicates the call disconnect due to user reject.
     */
    DISCONNECTED_REASON_USER_DECLINE = 143,
    /**
     * Indicates the call disconnect due to usr hangup.
     */
    DISCONNECTED_REASON_USER_TERMINATED = 501,
    /**
     * Indicates the call is answered elsewhere.
     */
    DISCONNECTED_REASON_ANSWERED_ELSEWHER = 18632,
};

/**
 * @brief Indicates the cause detail of a call disconnection.
 */
struct DisconnectedDetails {
    /**
     * Indicates the reason for ending the call.
     */
    DisconnectedReason reason = DisconnectedReason::FAILED_UNKNOWN;
    /**
     * Indicates the detail message for the ending call reason.
     */
    std::string message = "";

    DisconnectedDetails() {}

    DisconnectedDetails(const DisconnectedDetails &temp)
    {
        *this = temp;
    }

    DisconnectedDetails &operator=(const DisconnectedDetails &temp)
    {
        reason = temp.reason;
        message = temp.message;
        return *this;
    }
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_MANAGER_DISCONNECTED_REASON_H