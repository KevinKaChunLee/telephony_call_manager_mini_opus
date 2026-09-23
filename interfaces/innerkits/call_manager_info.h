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

#ifndef CALL_MANAGER_INFO_H
#define CALL_MANAGER_INFO_H

#include "call_manager_base.h"
#include "call_manager_disconnected_details.h"

namespace OHOS {
namespace Telephony {
/**
 * @brief Indicates the cellular call detail information.
 */
struct CellularCallInfo {
    /**
     * Indicates the call index
     */
    int32_t callId = 0;
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the slot id.
     */
    int32_t slotId = 0;
    int32_t accountId = 0;
    /**
     * Indicates the type of call, includs CS, IMS, OTT, OTHER. {@link CallType}
     */
    CallType callType = CallType::TYPE_ERR_CALL;
    /**
     * Indicates whether the call is video. 0: audio 1: send only 2: revive ony 3:video
     */
    int32_t videoState = 0;
    int32_t index = 0;
    /**
     * Indicates the call is RTT.
     */
    bool isRTT = false;
};

/**
 * @brief Indicates the cellular call event information.
 */
struct CellularCallEventInfo {
    /**
     * Indicates the call event type. {@link CellularCallEventType}
     */
    CellularCallEventType eventType = CellularCallEventType::EVENT_REQUEST_RESULT_TYPE;
    /**
     * Indicates the call event id, one id corresponds to one request.
     * {@link RequestResultEventId}
     */
    RequestResultEventId eventId = RequestResultEventId::INVALID_REQUEST_RESULT_EVENT_ID;
};

/**
 * @brief Indicates the ringback tone playing mode.
 */
enum class RBTPlayInfo {
    /**
     * Indicates the mode is network, the tone is from network.
     */
    NETWORK_ALERTING,
    /**
     * Indicates the mode is local, the tone is from local file.
     */
    LOCAL_ALERTING,
};

struct VoipCallReportInfo {
    std::string voipCallId = "";
    std::string userName = "";
    std::vector<uint8_t> userProfile = {};
    std::string abilityName = "";
    std::string extensionId = "";
    std::string voipBundleName = "";
    bool showBannerForIncomingCall = true;
    bool isConferenceCall = false;
    bool isVoiceAnswerSupported = true;
    bool isUserMuteRingToneAllowed = false;
    bool isDialingAllowedDuringCarrierCall = false;
    bool hasMicPermission = true;
    bool isCapsuleSticky = false;
    int32_t uid = 0;
};

/**
 * @brief Indicates detail information of the number mark.
 */
struct NumberMarkInfo {
    /**
     * Indicates the type of number mark.
     */
    MarkType markType = MarkType::MARK_TYPE_DEFAULT;
    /**
     * Indicates the content of number mark.
     */
    char markContent[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the count of number mark.
     */
    int32_t markCount = -1;
    /**
     * Indicates the source of number mark.
     */
    char markSource[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates if this is a number mark from cloud.
     */
    bool isCloud = false;
    /**
     * Indicates the details of number mark.
     */
    char markDetails[kMaxNumberLen + 1] = { 0 };
};

/**
 * @brief Indicates the call event detail information.
 */
struct CallEventInfo {
    /**
     * Indicates the event ID of call ability. {@link CallAbilityEventId}
     */
    CallAbilityEventId eventId = CallAbilityEventId::EVENT_DIAL_NO_CARRIER;
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
};

struct AccountInfo {
    int32_t accountId = 0;
    int32_t power = 0;
    char bundleName[kMaxNumberLen + 1] = { 0 };
    bool isEnabled = false;
};

/**
 * @brief Indicates the call report information.
 */
struct CallReportInfo {
    int32_t index = 0;
    /**
     * Indicates the call phone number.
     */
    char accountNum[kMaxNumberLen + 1] = { 0 };
    int32_t accountId = 0;
    /**
     * Indicates the type of call, includs CS, IMS, OTT, OTHER. {@link CallType}
     */
    CallType callType = CallType::TYPE_ERR_CALL;
    /**
     * Indicates the type of video state. {@link VideoStateType}
     */
    VideoStateType callMode = VideoStateType::TYPE_VOICE;
    /**
     * Indicates the detailed state of call. {@link TelCallState}
     */
    TelCallState state = TelCallState::CALL_STATUS_UNKNOWN;
    /**
     * Indicates the voica domain, 0: CS, 1: IMS
     */
    int32_t voiceDomain = 0;
    /**
     * Indicates the Multi-party call status:
     * 0: not a multi-party(conference) call
     * 1: a multi-party(conference) call
     */
    int32_t mpty = 0;
    /**
     * Indicates the color tone type.
     */
    int32_t crsType = 0;
    /**
     * Indicates the initial type of this call.
     */
    int32_t originalCallType = 0;
    /**
     * Indicates the VoIP call specific information
     */
    VoipCallReportInfo voipCallInfo;
    /**
     * Call state between with holdRequest and holdResponse
     */
    bool isPendingHold = false;
    /**
     * Identifies the CNAP display name;
     * - 0：Allowed
     * - 1：Restricted
     * - 2：Not Specified/Unknown
     * - 3：Payphone
     */
    int32_t namePresentation = 0;
    /**
     * Indicates remote party name.
     */
    std::string name = "";
    /**
     * Indicates the call disconnect information
    */
    DisconnectedReason reason = DisconnectedReason::FAILED_UNKNOWN;
    /**
     * Indicates the detail message for the ending call reason.
     */
    std::string message = "";
    /**
     * Whether to use the message box.
     * - 0: No
     * - 1: Yes
     */
    int32_t newCallUseBox = 0;
    /**
     * Whether to use the RTT call.
     */
    RttCallState rttState = RttCallState::RTT_STATE_NO;
    /**
     * RTT Call Channel Id
     */
    int32_t rttChannelId = -1;
    /**
     * Indicates the ims domain.
     * - 0：volte
     * - 1：vowifi
     * - 2：vonr
     */
    int32_t imsDomain = 0;
    /**
     * Remote Bluetooth real mac address
     */
    std::string remoteBluetoothAddr = "";
    /**
     * Indicates the phone index.
     * - 0：Primary phone
     * - 1：Secondary phone
     */
    int32_t phoneIndex = 0;
};

/**
 * @brief Indicates the detail information of some calls record.
 */
struct CallsReportInfo {
    /**
     * Indicates a list of the call report information.
     */
    std::vector<CallReportInfo> callVec {};
    /**
     * Indicates the slot id.
     */
    int32_t slotId = 0;
};

/**
 * @brief Indicates the detail information of a call.
 */
struct CallAttributeInfo {
    /**
     * Indicates the call phone number.
     */
    char accountNumber[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
    /**
     * Indicates whether the speaker phone is on.
     */
    bool speakerphoneOn = false;
    int32_t accountId = 0;
    /**
     * Indicates the type of video state. {@link VideoStateType}
     */
    VideoStateType videoState = VideoStateType::TYPE_VOICE;
    /**
     * Indicates the Call start time.
     */
    int64_t startTime = 0;
    /**
     * Indicates whether the call is emergency call.
     */
    bool isEcc = false;
    /**
     * Indicates whether the call is emergency Contact call.
     */
    bool isEccContact = false;
    /**
     * Indicates the type of call, includs CS, IMS, OTT, OTHER. {@link CallType}
     */
    CallType callType = CallType::TYPE_ERR_CALL;
    /**
     * Indicates the call index.
     */
    int32_t callId = 0;
    /**
     * Indicates the detailed state of call. {@link TelCallState}
     */
    TelCallState callState = TelCallState::CALL_STATUS_UNKNOWN;
    /**
     * Indicates the state of conference call. {@link TelConferenceState}
     */
    TelConferenceState conferenceState = TelConferenceState::TEL_CONFERENCE_IDLE;
    /**
     * Indicates the call beginning and ending time.
     */
    time_t callBeginTime = 0;
    time_t callCreateTime = 0;
    time_t callEndTime = 0;
    time_t ringBeginTime = 0;
    time_t ringEndTime = 0;
    /**
     * Indicates the call is MO or MT. {@link CallDirection}
     */
    CallDirection callDirection = CallDirection::CALL_DIRECTION_UNKNOW;
    /**
     * Indicates the cause when the call is answered. {@link CallAnswerType}
     */
    CallAnswerType answerType = CallAnswerType::CALL_ANSWER_MISSED;
    /*
     * Indicates the call index in cellular call
     */
    int32_t index = 0;
    /**
     * Indicates the color tone type.
     */
    int32_t crsType = 0;
    /**
     * Indicates the initial type of this call.
     */
    int32_t originalCallType = 0;
    /**
     * Indicates the VoIP call specific information
     */
    VoipCallReportInfo voipCallInfo;
    /**
     * Indicates the location of phone number.
     */
    char numberLocation[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the mark information of the phone number.
     */
    NumberMarkInfo numberMarkInfo;
    /**
     * Indicates the reason of call blocked.
     */
    int32_t blockReason = 0;
    /**
     * Indicates the type of celia call.
     */
    int32_t celiaCallType = -1;
    /**
     * Indicates the extraParamsString information.
     */
    std::string extraParamsString;
    /**
     * Indicates the contact name.
     */
    char contactName[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the details of call detect result.
     */
    char detectDetails[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the type of dial call from phone or watch.
     */
    int32_t phoneOrWatch = 0;
    /**
     * Identifies the CNAP display name;
     * - 0：Allowed
     * - 1：Restricted
     * - 2：Not Specified/Unknown
     * - 3：Payphone
     */
    int32_t namePresentation = 0;
    /**
     * Indicates remote party name.
     */
    std::string name = "";
    /**
     * Indicates the AntiFraud state.
     * - 0: AntiFraud is not started.
     * - 1: AntiFraud is started.
     * - 2: detect the call is fraud call.
     * - 3: AntiFraud is finished.
     */
    int32_t antiFraudState = 0;
    /**
     * Whether to use the message box.
     * - 0: No
     * - 1: Yes
     */
    int32_t newCallUseBox = 0;
    /**
     * Identifies the sim card type;
     * - 0: PSIM
     * - 1: ESIM
     */
    int32_t simType = 0;
    /**
     * Identifies the sim card index;
     */
    int32_t simIndex = 0;

    /**
     * Whether to use the RTT call.
     */
    RttCallState rttState = RttCallState::RTT_STATE_NO;

    /**
     * The disconnected call is RTT call.
     */
    bool isPrevRtt = false;
    /**
     * Indicates the ims domain.
     * - 0：volte
     * - 1：vowifi
     * - 2：vonr
     */
    int32_t imsDomain = 0;

    /**
     * Whether the third-party app supports custom accessibility features for accessible calling.
     */
    bool isCustomAccessibility = false;
    /**
     * Indicates the phone index.
     * - 0: Primary phone
     * - 1: Secondary phone
     */
    int32_t phoneIndex = 0;
};

/**
 * @brief Indicates the detail information of call record.
 */
struct CallRecordInfo {
    /**
     * Indicates the call index.
     */
    int32_t callId = 0;
    /**
     * Indicates the call phone number.
     */
    char phoneNumber[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the number after formatted.
     */
    char formattedNumber[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the phone number after formatted.
     */
    char formattedNumberToE164[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the location of phone number.
     */
    char numberLocation[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the type of call, includs CS, IMS, OTT, OTHER. {@link CallType}
     */
    CallType callType = CallType::TYPE_ERR_CALL;
    /**
     * Indicates the type of video state. {@link VideoStateType}
     */
    VideoStateType videoState = VideoStateType::TYPE_VOICE;
    /**
     * Indicates the call beginning time.
     */
    time_t callBeginTime = 0;
    /**
     * Indicates the call create time.
     */
    time_t callCreateTime = 0;
    /**
     * Indicates the call ending time.
     */
    time_t callEndTime = 0;
    /**
     * Indicates the call ring duration.
     */
    uint32_t ringDuration = 0;
    /**
     * Indicates the call session duration.
     */
    uint32_t callDuration = 0;
    /**
     * Indicates the call is MO or MT. {@link CallDirection}
     */
    CallDirection directionType = CallDirection::CALL_DIRECTION_UNKNOW;
    /**
     * Indicates the cause when the call is answered. {@link CallAnswerType}
     */
    CallAnswerType answerType = CallAnswerType::CALL_ANSWER_MISSED;
    /**
     * Indicates the country code of the call.
     */
    int32_t countryCode = 0;
    /**
     * Indicates the slot id.
     */
    int32_t slotId = 0;
    /**
     * Indicates the call features.
     */
    int32_t features = 0;
    /**
     * Indicates the mark information of the phone number.
     */
    NumberMarkInfo numberMarkInfo;
    /**
     * Indicates the reason of call blocked.
     */
    int32_t blockReason = 0;

    /**
     * Indicates the type of celia call.
     */
    int32_t celiaCallType = -1;

    /**
     * Indicates the details of call detect result.
     */
    char detectDetails[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates remote party name.
     */
    std::string name = "";
    /**
     * Identifies the CNAP display name;
     * - 0：Allowed
     * - 1：Restricted
     * - 2：Not Specified/Unknown
     * - 3：Payphone
     */
    int32_t namePresentation = 0;
    /**
     * Whether to use the message box.
     * - 0: No
     * - 1: Yes
     */
    int32_t newCallUseBox = 0;
    /**
     * Identifies the sim card type;
     * - 0: PSIM
     * - 1: ESIM
     */
    int32_t simType = 0;
    /**
     * Identifies the sim card index;
     */
    int32_t simIndex = 0;
    /**
     * Indicates the phone index.
     * - 0：Primary phone
     * - 1：Secondary phone
     */
    int32_t phoneIndex = 0;
    /**
     * Remote Bluetooth device name
     */
    std::string deviceName = "";

    CallRecordInfo() {}

    CallRecordInfo(const CallRecordInfo &temp)
    {
        *this = temp;
    }

    CallRecordInfo &operator=(const CallRecordInfo &temp)
    {
        callId = temp.callId;
        std::copy(std::begin(temp.phoneNumber), std::end(temp.phoneNumber), std::begin(phoneNumber));
        std::copy(std::begin(temp.formattedNumberToE164), std::end(temp.formattedNumberToE164),
            std::begin(formattedNumberToE164));
        std::copy(std::begin(temp.formattedNumber), std::end(temp.formattedNumber),
            std::begin(formattedNumber));
        std::copy(std::begin(temp.numberLocation), std::end(temp.numberLocation),
            std::begin(numberLocation));
        callType = temp.callType;
        callBeginTime = temp.callBeginTime;
        callCreateTime = temp.callCreateTime;
        callEndTime = temp.callEndTime;
        ringDuration = temp.ringDuration;
        callDuration = temp.callDuration;
        directionType = temp.directionType;
        answerType = temp.answerType;
        countryCode = temp.countryCode;
        slotId = temp.slotId;
        videoState = temp.videoState;
        features = temp.features;
        numberMarkInfo.markType = temp.numberMarkInfo.markType;
        std::copy(std::begin(temp.numberMarkInfo.markContent), std::end(temp.numberMarkInfo.markContent),
            std::begin(numberMarkInfo.markContent));
        numberMarkInfo.markCount = temp.numberMarkInfo.markCount;
        std::copy(std::begin(temp.numberMarkInfo.markSource), std::end(temp.numberMarkInfo.markSource),
            std::begin(numberMarkInfo.markSource));
        numberMarkInfo.isCloud = temp.numberMarkInfo.isCloud;
        std::copy(std::begin(temp.numberMarkInfo.markDetails), std::end(temp.numberMarkInfo.markDetails),
            std::begin(numberMarkInfo.markDetails));
        blockReason = temp.blockReason;
        celiaCallType = temp.celiaCallType;
        std::copy(std::begin(temp.detectDetails), std::end(temp.detectDetails),
            std::begin(detectDetails));
        namePresentation = temp.namePresentation;
        name = temp.name;
        newCallUseBox = temp.newCallUseBox;
        simType = temp.simType;
        simIndex = temp.simIndex;
        phoneIndex = temp.phoneIndex;
        deviceName = temp.deviceName;
        return *this;
    }
};

/**
 * @brief Indicates detail information of a call.
 */
struct CallDetailInfo {
    /**
     * Indicates the call index.
     */
    int32_t index = 0;
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
    /**
     * Indicates the account id.
     */
    int32_t accountId = 0;
    /**
     * Indicates the call type, CS, IMS, ERROR
     */
    CallType callType = CallType::TYPE_ERR_CALL;
    /**
     * Indicates the call video state, 0: audio 1:video.
     */
    VideoStateType callMode = VideoStateType::TYPE_VOICE;
    /**
     * Indicates the call status.
     */
    TelCallState state = TelCallState::CALL_STATUS_UNKNOWN;
    /**
     * Indicates the voice domain. 0: CS, 1: IMS
     */
    int32_t voiceDomain = 0;
    /**
     * Indicates the Multi-party call status:
     * 0: not a multi-party(conference) call
     * 1: a multi-party(conference) call
     */
    int32_t mpty = 0;
    /**
     * Indicates the color tone type.
     */
    int32_t crsType = 0;
    /**
     * Indicates the initial type of this call.
     */
    int32_t originalCallType = 0;
    /**
     * Indicates the VoIP call specific information
     */
    VoipCallReportInfo voipCallInfo;
    /**
     * Indicates remote party name.
     */
    std::string name = "";
    /**
     * Identifies the CNAP display name;
     * - 0：Allowed
     * - 1：Restricted
     * - 2：Not Specified/Unknown
     * - 3：Payphone
     */
    int32_t namePresentation = 0;

    int32_t phoneOrWatch = 0;
    /**
     * Indicates the disconnect reason.
    */
    DisconnectedReason reason = DisconnectedReason::FAILED_UNKNOWN;
    /**
     * Indicates the AntiFraud state.
     * - 0: AntiFraud is not started.
     * - 1: AntiFraud is started.
     * - 2: detect the call is fraud call.
     * - 3: AntiFraud is finished.
     */
    int32_t antiFraudState = 0;
    /**
     * Indicates the detail message for the ending call reason.
     */
    std::string message = "";
    /**
     * Whether to use the message box.
     * - 0: No
     * - 1: Yes
     */
    int32_t newCallUseBox = 0;
    /**
     * RTT call state
     */
    RttCallState rttState = RttCallState::RTT_STATE_NO;
    /**
     * RTT Call Channel Id
     */
    int32_t rttChannelId = -1;
    /**
     * Indicates the ims domain.
     * - 0：volte
     * - 1：vowifi
     * - 2：vonr
     */
    int32_t imsDomain = 0;
    /**
     * Indicates the phone index.
     * - 0：Primary phone
     * - 1：Secondary phone
     */
    int32_t phoneIndex = 0;

    CallDetailInfo() {}

    CallDetailInfo(const CallDetailInfo &temp)
    {
        *this = temp;
    }

    CallDetailInfo &operator=(const CallDetailInfo &temp)
    {
        index = temp.index;
        std::copy(std::begin(temp.phoneNum), std::end(temp.phoneNum), std::begin(phoneNum));
        std::copy(std::begin(temp.bundleName), std::end(temp.bundleName), std::begin(bundleName));
        accountId = temp.accountId;
        callType = temp.callType;
        callMode = temp.callMode;
        state = temp.state;
        voiceDomain = temp.voiceDomain;
        mpty = temp.mpty;
        crsType = temp.crsType;
        originalCallType = temp.originalCallType;
        voipCallInfo.voipCallId = temp.voipCallInfo.voipCallId;
        voipCallInfo.extensionId = temp.voipCallInfo.extensionId;
        voipCallInfo.userName = temp.voipCallInfo.userName;
        (voipCallInfo.userProfile).assign(
            (temp.voipCallInfo.userProfile).begin(), (temp.voipCallInfo.userProfile).end());
        voipCallInfo.abilityName = temp.voipCallInfo.abilityName;
        voipCallInfo.voipBundleName = temp.voipCallInfo.voipBundleName;
        voipCallInfo.showBannerForIncomingCall = temp.voipCallInfo.showBannerForIncomingCall;
        voipCallInfo.isConferenceCall = temp.voipCallInfo.isConferenceCall;
        voipCallInfo.isVoiceAnswerSupported = temp.voipCallInfo.isVoiceAnswerSupported;
        voipCallInfo.isUserMuteRingToneAllowed = temp.voipCallInfo.isUserMuteRingToneAllowed;
        voipCallInfo.isDialingAllowedDuringCarrierCall = temp.voipCallInfo.isDialingAllowedDuringCarrierCall;
        voipCallInfo.hasMicPermission = temp.voipCallInfo.hasMicPermission;
        voipCallInfo.isCapsuleSticky = temp.voipCallInfo.isCapsuleSticky;
        voipCallInfo.uid = temp.voipCallInfo.uid;
        name = temp.name;
        namePresentation = temp.namePresentation;
        phoneOrWatch = temp.phoneOrWatch;
        reason = temp.reason;
        antiFraudState = temp.antiFraudState;
        message = temp.message;
        newCallUseBox = temp.newCallUseBox;
        rttState = temp.rttState;
        rttChannelId = temp.rttChannelId;
        imsDomain = temp.imsDomain;
        phoneIndex = temp.phoneIndex;
        return *this;
    }
};

/**
 * @brief Indicates detail information of some calls.
 */
struct CallDetailsInfo {
    /**
     * Indicates a CallDetailInfo list. {@link CallDetailInfo}
     */
    std::vector<CallDetailInfo> callVec {};
    /**
     * Indicates the slot id.
     */
    int32_t slotId = 0;
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
};

/**
 * @brief Indicates the type of call transfer setting.
 *
 * 27007-430_2001 7.11	Call forwarding number and conditions +CCFC
 * 3GPP TS 22.082 [4]
 * <mode>:
 * 0	disable
 * 1	enable
 * 3	registration
 * 4	erasure
 */
enum class CallTransferSettingType {
    /**
     * Indicates disable the call transfer.
     */
    CALL_TRANSFER_DISABLE = 0,
    /**
     * Indicates enable the call transfer.
     */
    CALL_TRANSFER_ENABLE = 1,
    /**
     * Indicates register the call transfer.
     */
    CALL_TRANSFER_REGISTRATION = 3,
    /**
     * Indicates erasure the call transfer.
     */
    CALL_TRANSFER_ERASURE = 4,
};

/**
 * @brief Indicates the Call forwarding type
 *
 * 27007-430_2001 7.11	Call forwarding number and conditions +CCFC
 * 3GPP TS 22.082 [4]
 * <reason>:
 * 0	unconditional
 * 1	mobile busy
 * 2	no reply
 * 3	not reachable
 */
enum class CallTransferType {
    /**
     * Indicates transfer the call unconditionally.
     */
    TRANSFER_TYPE_UNCONDITIONAL = 0,
    /**
     * Indicates transfer the call when busy.
     */
    TRANSFER_TYPE_BUSY = 1,
    /**
     * Indicates transfer the call when no reply.
     */
    TRANSFER_TYPE_NO_REPLY = 2,
    /**
     * Indicates transfer the call when unreachable.
     */
    TRANSFER_TYPE_NOT_REACHABLE = 3,
    /**
     * Indicates transfer the call when all.
     */
    TRANSFER_TYPE_ALL = 4,
    /**
     * Indicates transfer the call conditionally.
     */
    TRANSFER_TYPE_CONDITIONAL = 5,
};

/**
 * @brief Indicates the information of call transfer.
 */
struct CallTransferInfo {
    /**
     * Indication the phone number which will forward to.
     */
    char transferNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the type of call transfer setting. {@link CallTransferSettingType}
     */
    CallTransferSettingType settingType = CallTransferSettingType::CALL_TRANSFER_DISABLE;
    /**
     * Indicates the Call forwarding type. {@link CallTransferType}
     */
    CallTransferType type = CallTransferType::TRANSFER_TYPE_UNCONDITIONAL;
    /**
     * Start time hours.
     */
    int32_t startHour = 0;
    /**
     * Start time minutes.
     */
    int32_t startMinute = 0;
    /**
     * End time hours.
     */
    int32_t endHour = 0;
    /**
     * End time minutes.
     */
    int32_t endMinute = 0;
};

/**
 * @brief Indicates the type of call restriction.
 *
 * 3GPP TS 22.030 V4.0.0 (2001-03)
 * 3GPP TS 22.088 V4.0.0 (2001-03)
 */
enum class CallRestrictionType {
    /**
     * Indicates restrict all incoming calls.
     */
    RESTRICTION_TYPE_ALL_INCOMING = 0,
    /**
     * Indicates restrict all outgoing calls.
     */
    RESTRICTION_TYPE_ALL_OUTGOING,
    /**
     * Indicates restrict international calls.
     */
    RESTRICTION_TYPE_INTERNATIONAL,
    /**
     * Indicates restrict international roaming calls.
     */
    RESTRICTION_TYPE_INTERNATIONAL_EXCLUDING_HOME,
    /**
     * Indicates restrict roaming calls.
     */
    RESTRICTION_TYPE_ROAMING_INCOMING,
    /**
     * Indicates restrict all calls.
     */
    RESTRICTION_TYPE_ALL_CALLS,
    /**
     * Indicates restrict all outgoing services.
     */
    RESTRICTION_TYPE_OUTGOING_SERVICES,
    /**
     * Indicates restrict all incoming services.
     */
    RESTRICTION_TYPE_INCOMING_SERVICES,
};

/**
 * @brief Indicates the mode of call restriction.
 *
 * 3GPP TS 22.088 V4.0.0 (2001-03)
 */
enum class CallRestrictionMode {
    /**
     * Indicates call restriction is deactivated.
     */
    RESTRICTION_MODE_DEACTIVATION = 0,
    /**
     * Indicates call restriction is activated.
     */
    RESTRICTION_MODE_ACTIVATION = 1,
};

/**
 * @brief Indicates the type of call reject.
 */
enum class RejectType {
    /**
     * Indicates call reject is normal.
     */
    CALL_REJECT_NORMAL = 0,
    /**
     * Indicates call reject but add missed call log.
     */
    CALL_REJECT_MISSED_CALL,
};

/**
 * @brief Indicates the information of call restriction.
 */
struct CallRestrictionInfo {
    /**
     * Indicates the password required to set call restrictions.
     */
    char password[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the type of call restriction.
     */
    CallRestrictionType fac = CallRestrictionType::RESTRICTION_TYPE_ALL_INCOMING;
    /**
     * Indicates the mode of call restriction.
     */
    CallRestrictionMode mode = CallRestrictionMode::RESTRICTION_MODE_DEACTIVATION;
};

struct TransferCallInfo {
    std::string devMac = "";
    uint32_t callId = 0;
    CallDirection callDirect = CallDirection::CALL_DIRECTION_UNKNOW;
    TelCallState callState = TelCallState::CALL_STATUS_UNKNOWN;
    CallType callType = CallType::TYPE_BLUETOOTH;
};

struct TransferResultInfo {
    std::string deviceShowName = "";
    uint32_t secPolicy = 0;
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_MANAGER_INFO_H
