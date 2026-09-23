/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef CALL_MANAGER_INNER_TYPE_H
#define CALL_MANAGER_INNER_TYPE_H

#include "call_manager_base.h"
#include "call_manager_disconnected_details.h"
#include "call_manager_info.h"
#include "securec.h"

namespace OHOS {
namespace Telephony {
/**
 * @brief Indicates the Call Waiting response result.
 */
struct CallWaitResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA.
     */
    int32_t status = 0;
    /**
     * parameter shows the subscriber CW service status in the network.
     */
    int32_t classCw = 0;
};

/**
 * @brief Indicates the Clip response result.
 */
struct ClipResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA.
     */
    int32_t action = 0;
    /**
     * parameter shows the subscriber CLIP service status in the network.
     */
    int32_t clipStat = 0;
};

/**
 * @brief Indicates the Clir response result.
 */
struct ClirResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA.
     */
    int32_t action = 0;
    /**
     * parameter shows the subscriber CLIR service status in the network.
     */
    int32_t clirStat = 0;
};

/**
 * @brief Indicates the Colr response result.
 */
struct ColrResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA.
     */
    int32_t action = 0;
    /**
     * parameter shows the subscriber COLR service status in the network.
     */
    int32_t colrStat = 0;
};

/**
 * @brief Indicates the Colp response result.
 */
struct ColpResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA.
     */
    int32_t action = 0;
    /**
     * parameter shows the subscriber COLP service status in the network.
     */
    int32_t colpStat = 0;
};

/**
 * @brief Indicates the MMI Code response result.
 */
struct MmiCodeInfo {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific message of MMI code response.
     */
    char message[kMaxMessageLen + 1] = { 0 };
    /**
     * SC type
     */
    int32_t mmiCodeType = -1;
    /**
     * 0: query, 1:active, 2:deactive
     */
    int32_t action = -1;
    /**
     * parameter sets/shows the result code presentation status in the TA
     * 0: disable
     * 1: enable
     */
    int32_t status = -1;
    /*
     * <classCw> is a sum of integers each representing a class of information (default 7):
     * 1	voice (telephony)
     * 2	data (refers to all bearer services)
     * 4	fax (facsimile services)
     * 8	short message service
     * 16	data circuit sync
     * 32	data circuit async
     * 64	dedicated packet access
     * 128	dedicated PAD access
     */
    int32_t classCw = -1;
    /**
     * Indication the phone number which will forward to.
     */
    char number[kMaxNumberLen + 1] = { 0 };
    int32_t reason = -1;
    int32_t time = -1;
};

/**
 * @brief Indicates the Call Transfer response result.
 */
struct CallTransferResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the status of the CF option. 0: not active; 1: active
     */
    int32_t status = 0;
    /**
     * <classx> is a sum of integers each representing a class of information
     * (default 7):
     * 1	voice (telephony)
     * 2	data (refers to all bearer services)
     * 4	fax (facsimile services)
     * 8	short message service
     * 16	data circuit sync
     * 32	data circuit async
     * 64	dedicated packet access
     * 128	dedicated PAD access
     */
    int32_t classx = 0;
    /**
     * type of address octet in integer format (refer GSM 04.08 [8]
     * subclause 10.5.4.7); default 145 when dialling string includes
     * international access code character "+", otherwise 129.
     */
    int32_t type = 0;
    /**
     * Indication the phone number which will forward to.
     */
    char number[kMaxNumberLen + 1] = { 0 };
    int32_t reason = 0;
    int32_t time = 0;
    /**
     * Indicates the Call forwarding start time.
     */
    int32_t startHour = INVALID_TIME;
    int32_t startMinute = INVALID_TIME;
    /**
     * Indicates the Call forwarding end time.
     */
    int32_t endHour = INVALID_TIME;
    int32_t endMinute = INVALID_TIME;
};

/**
 * @brief Indicates the Call Restriction response result.
 */
struct CallRestrictionResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * parameter sets/shows the result code presentation status in the TA
     */
    int32_t status = 0;
    /**
     * parameter shows the subscriber Call Restriction service status
     * in the network.
     */
    int32_t classCw = 0;
};

/**
 * @brief Indicates the Call Preference response result.
 */
struct CallPreferenceResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific value of the call preference mode.
     *
     * 1：CS Voice only
     * 2：CS Voice preferred, IMS PS Voice as secondary
     * 3：IMS PS Voice preferred, CS Voice as secondary
     * 4：IMS PS Voice only
     */
    int32_t mode = 0;
};

/**
 * @brief Indicates the GetImsConfig response result.
 */
struct GetImsConfigResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific value of the corresponding IMS config.
     */
    int32_t value = 0;
};

/**
 * @brief Indicates the GetImsFeatureValue response result.
 */
struct GetImsFeatureValueResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific value of the corresponding IMS feature.
     */
    int32_t value = 0;
};

struct GetLteEnhanceModeResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific value of the LTE Enhance mode.
     */
    int32_t value = 0;
};

struct MuteControlResponse {
    /**
     * 0: ok  other: error
     */
    int32_t result = 0;
    /**
     * Indicates the specific value of mute setting. 0: not mute 1: set mute
     */
    int32_t value = 0;
};

/**
 * @brief Indicates the type of supplementary service request.
 */
enum class SsRequestType {
    SS_ACTIVATION = 0,
    SS_DEACTIVATION,
    SS_INTERROGATION,
    SS_REGISTRATION,
    SS_ERASURE,
};

/**
 * @brief Indicates the video window size and location in video call.
 */
struct VideoWindow {
    /**
     * X coordinate of window
     */
    int32_t x = 0;
    /**
     * Y coordinate of window
     */
    int32_t y = 0;
    /**
     * Z coordinate of window
     */
    int32_t z = 0;
    /**
     * the width of window
     */
    int32_t width = 0;
    /**
     * the height of window
     */
    int32_t height = 0;
};

/**
 * @brief Indicates the +CHLD related supplementary services
 *
 * 3GPP TS 27.007 V3.9.0 (2001-06) Call related supplementary services +CHLD
 * 3GPP TS 27.007 V3.9.0 (2001-06) 7.22	Informative examples
 */
enum CallSupplementType {
    /**
     * default type
     */
    TYPE_DEFAULT = 0,
    /**
     * release the held call and the wait call
     */
    TYPE_HANG_UP_HOLD_WAIT = 1,
    /**
     * release the active call and recover the held call
     */
    TYPE_HANG_UP_ACTIVE = 2,
    /**
     * release all calls
     */
    TYPE_HANG_UP_ALL = 3,
};

enum CellularCallReturnType {
    /**
     * 3GPP TS 27.007 V3.9.0 (2001-06) 6.27	Informative examples
     */
    RETURN_TYPE_MMI = 0,
};

/**
 * @brief Indicates the voice domain preference.
 *
 * 3GPP TS 27.007 V17.3.0 (2021-09) 10.1.35	UE's voice domain
 * preference E-UTRAN +CEVDP
 */
enum DomainPreferenceMode {
    /**
     * Indicates the voice domain preference is CS only.
     */
    CS_VOICE_ONLY = 1,
    /**
     * Indicates the voice domain preference is CS prefered.
     */
    CS_VOICE_PREFERRED = 2,
    /**
     * Indicates the voice domain preference is PS prefered.
     */
    IMS_PS_VOICE_PREFERRED = 3,
    /**
     * Indicates the voice domain preference is PS only.
     */
    IMS_PS_VOICE_ONLY = 4,
};

/**
 * @brief Indicates the IMS call mode.
 */
enum ImsCallMode {
    CALL_MODE_AUDIO_ONLY = 0,
    CALL_MODE_SEND_ONLY,
    CALL_MODE_RECEIVE_ONLY,
    CALL_MODE_SEND_RECEIVE,
    CALL_MODE_VIDEO_PAUSED,
};

#ifdef SUPPORT_RTT_CALL
/**
 * @brief Indicates the IMS RTT call mode.
 */
enum ImsRTTCallMode : int32_t  {
    LOCAL_REQUEST_UPGRADE = 0,
    LOCAL_REQUEST_DOWNGRADE = 1,
    REMOTE_REQUEST_UPGRADE_LOCAL_ACCEPT = 2,
    REMOTE_REQUEST_UPGRADE_LOCAL_REJECT = 3,
};

/**
 * @brief Indicates the IMS RTT Event type
 */
enum ImsRTTEventType : int32_t  {
    EVENT_RTT_ENABLED = 0,
    EVENT_RTT_CLOSED = 1,
    EVENT_REMOTE_REQUEST_ENABLE_RTT = 2,
    EVENT_RTT_PARAM_CHANGE = 3,
};
#endif

/**
 * @brief Indicates the IMS call mode request result.
 */
enum VideoRequestResultType {
    TYPE_REQUEST_SUCCESS = 0,
    TYPE_REQUEST_FAIL,
    TYPE_REQUEST_INVALID,
    TYPE_REQUEST_TIMED_OUT,
    REQUEST_REJECTED_BY_REMOTE,
    TYPE_REQUEST_UPGRADE_CANCELED,
    TYPE_MODIFY_DOWNGRADE_RTP_OR_RTCP_TIMEOUT = 100,
    TYPE_MODIFY_DOWNGRADE_RTP_AND_RTCP_TIMEOUT,
};

/**
 * @brief Indicates the request call media mode.
 */
struct CallMediaModeRequest {
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the IMS call mode. {@link ImsCallMode}
     */
    ImsCallMode mode = ImsCallMode::CALL_MODE_AUDIO_ONLY;
};

/**
 * @brief Indicates the response of the call media mode request.
 */
struct CallMediaModeInfo {
    /**
     * Indicates the call id.
     */
    int32_t callId = 0;
    /**
     * Indicates if the ims call info is request which received from remote.
     */
    bool isRequestInfo = false;
    /**
     * Indicates the response result.
     */
    VideoRequestResultType result = VideoRequestResultType::TYPE_REQUEST_SUCCESS;
    /**
     * Indicates the call mode result.
     */
    ImsCallMode callMode = ImsCallMode::CALL_MODE_AUDIO_ONLY;

    CallMediaModeInfo() {}

    CallMediaModeInfo(const CallMediaModeInfo &temp)
    {
        *this = temp;
    }

    CallMediaModeInfo &operator=(const CallMediaModeInfo &temp)
    {
        callId = temp.callId;
        isRequestInfo = temp.isRequestInfo;
        result = temp.result;
        callMode = temp.callMode;
        return *this;
    }
};

/**
 * @brief Indicates the ImsCallMode report information from cellular call.
 */
struct CallModeReportInfo {
    /**
     * Indicates the call index.
     */
    int32_t callIndex = 0;
    /**
     * Indicates the response result.
     */
    VideoRequestResultType result = VideoRequestResultType::TYPE_REQUEST_SUCCESS;
    /**
     * Indicates the response result.
     */
    ImsCallMode callMode = ImsCallMode::CALL_MODE_AUDIO_ONLY;

    int32_t slotId = -1;
};

/**
 * @brief Indicates the RTT Call Event report information from cellular call.
 */
struct RttEvent {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * RTT事件类型
     */
    int32_t eventType = -1;

    /**
     * 0: 本地用户发起 1: 远端用户发起
     */
    int32_t reason = -1;

    RttEvent() {}

    RttEvent(const RttEvent &temp)
    {
        *this = temp;
    }

    RttEvent &operator=(const RttEvent &temp)
    {
        callId = temp.callId;
        eventType = temp.eventType;
        reason = temp.reason;
        return *this;
    }
};

struct RttEventInfo {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * RTT事件类型
     */
    int32_t eventType = -1;

    /**
     * 0: 本地用户发起 1: 远端用户发起
     */
    int32_t reason = -1;

    int32_t slotId = -1;
};

/**
 * @brief Indicates the RTT Call Error report information from cellular call.
 */
struct RttError {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * 0:Add Text; 1:Close Text
     */
    int32_t operationType = -1;

    /**
     * 错误码
     */
    int32_t causeCode = -1;

    /**
     * 错误码字符串信息
     */
    std::string reasonText;

    RttError() {}

    RttError(const RttError &temp)
    {
        *this = temp;
    }

    RttError &operator=(const RttError &temp)
    {
        callId = temp.callId;
        operationType = temp.operationType;
        causeCode = temp.causeCode;
        reasonText = temp.reasonText;
        return *this;
    }
};

struct RttErrorInfo {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * 0:Add Text; 1:Close Text
     */
    int32_t operationType = -1;

    /**
     * 错误码
     */
    int32_t causeCode;

    /**
     * 错误码字符串信息
     */
    std::string reasonText;

    int32_t slotId = -1;
};

/**
 * @brief Indicates the RTT Call Message report information
 */
struct RttMessage {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * RTT消息
     */
    char* rttMessage = nullptr;

    RttMessage() {}

    RttMessage(const RttMessage &temp)
    {
        *this = temp;
    }

    RttMessage &operator=(const RttMessage &temp)
    {
        callId = temp.callId;
        rttMessage = temp.rttMessage;
        return *this;
    }
};

struct RttMessageInfo {
    /**
     * Call ID, value can be 1~7
     */
    int32_t callId = -1;

    /**
     * RTT消息
     */
    char* rttMessage = nullptr;;
};

/**
 * @brief Indicates the response of the call media mode request.
 */
struct CallSessionEvent {
    /**
     * Indicates the event ID of call ability. {@link CallAbilityEventId}
     */
    CallSessionEventId eventId = CallSessionEventId::EVENT_CAMERA_FAILURE;
    /**
     * Indicates the call phone number.
     */
    int32_t callId = 0;

    CallSessionEvent() {}

    CallSessionEvent(const CallSessionEvent &temp)
    {
        *this = temp;
    }

    CallSessionEvent &operator=(const CallSessionEvent &temp)
    {
        callId = temp.callId;
        eventId = temp.eventId;
        return *this;
    }
};

/**
 * @brief Indicates the detail information of some calls record.
 */
struct CallSessionReportInfo {
    /**
     * Indicates call index.
     */
    int32_t index = 0;
    /**
     * Indicates the response result.
     */
    CallSessionEventId eventId = CallSessionEventId::EVENT_CAMERA_FAILURE;
};

struct PeerDimensionsDetail {
    /**
     * Indicates the call phone number.
     */
    int32_t callId = 0;
    /**
     * Indicates the call phone number.
     */
    int32_t width = 0;
    /**
     * Indicates the response result.
     */
    int32_t height = 0;

    PeerDimensionsDetail() {}

    PeerDimensionsDetail(const PeerDimensionsDetail &temp)
    {
        *this = temp;
    }

    PeerDimensionsDetail &operator=(const PeerDimensionsDetail &temp)
    {
        callId = temp.callId;
        width = temp.width;
        height = temp.height;
        return *this;
    }
};

struct PeerDimensionsReportInfo {
    /**
     * Indicates call index.
     */
    int32_t index = 0;
    /**
     * Indicates the peer window width.
     */
    int32_t width = 0;
    /**
     * Indicates the peer window height.
     */
    int32_t height = 0;
};

struct CameraCapabilities {
    /**
     * Indicates the call phone number.
     */
    int32_t callId = 0;
    /**
     * Indicates the call phone number.
     */
    int32_t width = 0;
    /**
     * Indicates the response result.
     */
    int32_t height = 0;

    CameraCapabilities() {}

    CameraCapabilities(const CameraCapabilities &temp)
    {
        *this = temp;
    }

    CameraCapabilities &operator=(const CameraCapabilities &temp)
    {
        callId = temp.callId;
        width = temp.width;
        height = temp.height;
        return *this;
    }
};

struct CameraCapabilitiesReportInfo {
    /**
     * Indicates call index.
     */
    int32_t index = 0;
    /**
     * Indicates the camera width.
     */
    int32_t width = 0;
    /**
     * Indicates the camera height.
     */
    int32_t height = 0;
};

struct ImsSuppExtReportInfo {
    /**
     * Indicates the slot id for the call.
     */
    int32_t slotId = -1;
    /**
     * Indicates the supplementary service extension code for the call.
     * The possible values and their meanings are as follows:
     * - -1: default or invalid value.
     * - 0: The incoming call is a redirected call.
     * - 1: The incoming call is a Closed User Group (CUG) call.
     * - 2: The active call is on hold by the other party.
     * - 3: The on-hold call is resumed by the other party.
     * - 4: The call is joined in a conference call.
     * - 5: The on-hold call is terminated by the other party.
     * - 6: A forward check supplementary service message is received.
     * - 7: The call is transferred to a third party, and the caller sees the third party's number.
     * - 8: The call is transferred to a third party, and the callee sees the third party's number.
     * - 9: The incoming call is a deflected call.
     * - 10: The incoming call is redirected.
     * - 22: The incoming call is initiated by the network.
     */
    int32_t code = -1;
    /**
     * Indicates the call index for the call.
     */
    int32_t callIndex = 0;
};

/**
 * @brief Indicates the specific IMS config item.
 */
enum ImsConfigItem {
    /**
     * Indicates the specific IMS config item is video quality.
     */
    ITEM_VIDEO_QUALITY = 0,

    /**
     * Indicates the specific IMS config item is IMS switch status.
     *
     * 3GPP TS 24.167 V17.1.0 (2020-12) 5.31
     * /<X>/Mobility_Management_IMS_Voice_Termination
     * Ims Switch Status
     *
     * format: bool
     * Access Types: Get, Replace
     * Values: 0, 1
     * 0 – Mobility Management for IMS Voice Termination disabled.
     * 1 – Mobility Management for IMS Voice Termination enabled.
     */
    ITEM_IMS_SWITCH_STATUS,
};

/**
 * @brief Indicates the VoNR switch status.
 */
enum class VoNRState {
    /**
     * Indicates the VoNR switch is off.
     */
    VONR_STATE_OFF = 0,
    /**
     * Indicates the VoNR switch is on.
     */
    VONR_STATE_ON = 1,
};

/**
 * @brief Indicates the video quality is high or low.
 */
enum VideoQualityValue {
    /**
     * Indicates the video quality is high.
     */
    LOW = 0,
    /**
     * Indicates the video quality is low.
     */
    HIGH = 1,
};

enum ImsConfigValue {
    CONFIG_SUCCESS = 0,
    CONFIG_FAILED = 1,
};

/**
 * @brief Indicates the specific IMS feature.
 *
 * Official Document IR.92 - IMS Profile for Voice and SMS
 * Annex C MNO provisioning and Late Customization
 *
 * format: bool
 * 3GPP TS 24.167 V17.1.0 (2020-12) 5.43 /<X>/Media_type_restriction_policy
 */
enum FeatureType {
    /**
     * Indicates the IMS feature is Voice over LTE.
     */
    TYPE_VOICE_OVER_LTE = 0,
    /**
     * Indicates the IMS feature is Video over LTE.
     */
    TYPE_VIDEO_OVER_LTE,
    /**
     * Indicates the IMS feature is SS over UT.
     */
    TYPE_SS_OVER_UT,
};

/**
 * @brief Indicates the specific OTT call request service.
 */
enum class OttCallRequestId {
    OTT_REQUEST_DIAL = 0,
    OTT_REQUEST_HANG_UP,
    OTT_REQUEST_REJECT,
    OTT_REQUEST_ANSWER,
    OTT_REQUEST_HOLD,
    OTT_REQUEST_UN_HOLD,
    OTT_REQUEST_SWITCH,
    OTT_REQUEST_COMBINE_CONFERENCE,
    OTT_REQUEST_SEPARATE_CONFERENCE,
    OTT_REQUEST_KICK_OUT_CONFERENCE,
    OTT_REQUEST_INVITE_TO_CONFERENCE,
    OTT_REQUEST_UPDATE_CALL_MEDIA_MODE,
};

/**
 * @brief Indicates the OTT call request information.
 */
struct OttCallRequestInfo {
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
    /**
     * Indicates the call video state, 0: audio 1:video.
     */
    VideoStateType videoState = VideoStateType::TYPE_VOICE;
};

/**
 * @brief Indicates the OTT call detail information.
 */
struct OttCallDetailsInfo {
    /**
     * Indicates the call phone number.
     */
    char phoneNum[kMaxNumberLen + 1] = { 0 };
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };
    /**
     * Indicates the call video state, 0: audio 1:video.
     */
    VideoStateType videoState = VideoStateType::TYPE_VOICE;
    /**
     * Indicates the call status.
     */
    TelCallState callState = TelCallState::CALL_STATUS_UNKNOWN;
};

/**
 * @brief Indicates the OTT call event ID.
 */
enum class OttCallEventId {
    OTT_CALL_EVENT_FUNCTION_UNSUPPORTED = 0,
};

/**
 * @brief Indicates the OTT call event information.
 */
struct OttCallEventInfo {
    /**
     * Indicates the OTT call event ID.
     */
    OttCallEventId ottCallEventId = OttCallEventId::OTT_CALL_EVENT_FUNCTION_UNSUPPORTED;
    /**
     * Indicates the bundle name.
     */
    char bundleName[kMaxBundleNameLen + 1] = { 0 };

    OttCallEventInfo() {}

    OttCallEventInfo(const OttCallEventInfo &temp)
    {
        *this = temp;
    }

    OttCallEventInfo &operator=(const OttCallEventInfo &temp)
    {
        ottCallEventId = temp.ottCallEventId;
        std::copy(std::begin(temp.bundleName), std::end(temp.bundleName), std::begin(bundleName));
        return *this;
    }
};

/**
 * @brief Indicates the audio device type and address.
 */
struct AudioDevice {
    /**
     * Indicates the audio device type. {@link AudioDeviceType}
     */
    AudioDeviceType deviceType = AudioDeviceType::DEVICE_UNKNOWN;
    /**
     * Indicates the device address.
     */
    char address[kMaxAddressLen + 1] = { 0 };
    /**
     * Indicates the device name.
     */
    char deviceName[kMaxDeviceNameLen + 1] = { 0 };
};

/**
 * @brief Indicates the audio device information.
 */
struct AudioDeviceInfo {
    /**
     * Indicates the list of all audio devices.
     */
    std::vector<AudioDevice> audioDeviceList {};
    /**
     * Indicates the current audio device.
     */
    AudioDevice currentAudioDevice;
    /**
     * Indicates wether the device is muted, true: mute, false: no mute
     */
    bool isMuted = false;
    /**
     * The callId of audio device change.
     */
    int32_t callId = 0;
    /**
     * Indicates whether the microphone of device is disabled,
     * true: muted and microphone is disabled, false: microphone is not disabled
     */
    bool isMicDisabled = false;
};

enum class MmiCodeResult {
    MMI_CODE_SUCCESS = 0,
    MMI_CODE_FAILED = 1
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_MANAGER_INNER_TYPE_H
