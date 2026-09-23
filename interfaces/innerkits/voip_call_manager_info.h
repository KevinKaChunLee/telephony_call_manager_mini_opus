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

#ifndef VOIP_CALL_MANAGER_INFO_H
#define VOIP_CALL_MANAGER_INFO_H
#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace Telephony {
/**
 * @brief Indicates the call audio event.
 */
enum class CallAudioEvent {
    /**
     * Indicates the event of muted.
     */
    AUDIO_EVENT_MUTED = 0,
    /**
     * Indicates the event of unmuted.
     */
    AUDIO_EVENT_UNMUTED = 1,
    /**
     * Indicates the event of speaker on.
     */
    AUDIO_EVENT_SPEAKER_ON = 2,
    /**
     * Indicates the event of speaker off.
     */
    AUDIO_EVENT_SPEAKER_OFF = 3,
    /**
     * Indicates whether the VOIP incoming call allows user to mute ringtone.
     */
    AUDIO_EVENT_MUTED_RINGTONE = 4,
    /**
     * Indicates the event of VOIP call muted and disabled the microphone switch.
     */
    AUDIO_EVENT_MIC_DISABLE = 5,
};

/**
 * @brief Indicates the voip call state.
 */
enum class VoipCallState {
    /**
     * Indicates the call state of idle.
     */
    VOIP_CALL_STATE_IDLE = 0,
    /**
     * Indicates the call state of incoming.
     */
    VOIP_CALL_STATE_INCOMING,
    /**
     * Indicates the call state of active.
     */
    VOIP_CALL_STATE_ACTIVE,
    /**
     * Indicates the call state is holding.
     */
    VOIP_CALL_STATE_HOLDING,
    /**
     * Indicates the call state of disconnected.
     */
    VOIP_CALL_STATE_DISCONNECTED,
    /**
     * Indicates the call state of dialing.
     */
    VOIP_CALL_STATE_DIALING,
    /**
     * Indicates the call is answered.
     */
    VOIP_CALL_STATE_ANSWERED,
    /**
     * Indicates the call is disconnecting.
     */
    VOIP_CALL_STATE_DISCONNECTING,
    /**
     * Indicates whether the VOIP incoming call allows user to mute ringtone.
     */
    VOIP_CALL_EVENT_MUTE_RINGTONE,
};

/**
 * @brief Indicates the voip call type.
 */
enum class VoipCallType {
    /**
     * Indicates the type of voip voice call.
     */
    VOIP_CALL_VOICE = 0,
    /**
     * Indicates the type of voip video call.
     */
    VOIP_CALL_VIDEO,
};

/**
 * @brief Indicates the live video pane is released successfully or fails to be released.
 */
enum class LiveVideWindowEvent {
    /**
     * Indicates the live video window is released successfully.
     */
    LIVE_VIDEO_WINDOW_EVENT_SUCCESS = 0,
    /**
     * Indicates failed to publish the live video window.
     */
    LIVE_VIDEO_WINDOW_EVENT_FAIL,
};

enum class VoipProcedureScenarioEvent {
    PUSH_REPORT_CALL_INFO = 1,
    VOIP_REPORT_CALL_INFO,
    VOIP_REPORT_CALLSTATE,
    VOIP_REPORT_AUDIO_CHANGE,
    SEND_UI_EVENT_TO_VOIP,
    REPORT_CALL_INFO_TO_CALLUI,
    CALLUI_SEND_UI_EVENT,
    AUDIO_OPERATION_FOR_VOIP,
    AUDIO_EVENT_FROM_AUDIOFW,
    COMMON_EVENT,
};

enum class VoipProcedureEvent {
    // PUSH -> CallServiceKit
    PUSH_REPORT_INCOMING_CALL = 1,                      // detail ref: 0
    PUSH_REPORT_INCOMING_CALL_PRECEDURE,                // detail ref: PushReportIncomingCallProcedureDetail

    // Voip -> CallServiceKit
    VOIP_REPORT_INCOMING_CALL,                          // detail ref: 0
    VOIP_REPORT_INCOMING_CALL_ERROR,                    // detail ref: ReportVoipCallFailedCauser from @voip
    VOIP_REPORT_INCOMING_CALL_PROCEDURE,                // VoipReportIncomingCallProcedureDetail
    VOIP_REPORT_OUTGOING_CALL,                          // detail ref: 0
    VOIP_REPORT_OUTGOING_CALL_PROCEDURE,                // detail ref: VoipReportOutgoingCallProcedureDetail

    VOIP_REPORT_CALL_STATE_CHANGE,                      // detail ref: state from @voip
    VOIP_REPORT_CALL_STATE_CHANGE_PROCEDURE,            // detail ref: VoipReportCallStateChangeProcedureDetail
    VOIP_REPORT_CALL_AUDIO_CHANGE,                      // detail ref: audio event from @voip
    VOIP_REPORT_CALL_AUDIO_CHANGE_PROCEDURE,            // detail ref: VoipReportAudioStateChangeProcedureDetail
    // CallServiceKit -> VoIP,
    REPORT_UI_EVENT_TO_VOIP,                            // detail ref: voipCallEvent: answer, hungup,reject,
                                                        // audio operation.
    REPORT_UI_EVENT_TO_VOIP_PROCEDURE,                  // detail ref: ret of OnVoipCallEventChange()
    // CallManager -> CallUi
    REPORT_CALL_STATE_TO_CALLUI,                        // detail ref: tel call state
    REPORT_AUDIO_EVENT_TO_CALLUI,                       // detail ref: audio event
    // CallUi -> CallManager
    CALLUI_SEND_UI_EVENT,                               // detail ref: CalluiSendUiEventDetail, such as answer, hungup,
                                                        // reject, audio operation.
    //CallManager inner
    CALLMANAGER_ANSWER_VOIP,                            // detail ref: 0
    CALLMANAGER_REJECT_VOIP,                            // detail no
    CALLMANAGER_HUNGUP_VOIP,                            // detail ref: errorReason
    CALLMANAGER_MUTE_VOIP,                              // detail ref: callAudioEvent
    CALLMANAGER_HANDLE_CALL_STATE_CHANGE,               // detail no
    CALLMANAGER_HANDLE_AUDIO_CHANGE,                    // detail no
};

enum class CalluiSendUiEventDetail {
    CALLUI_ANSWER_VOIP,
    CALLUI_HUNGUP_VOIP,
    CALLUI_REJECT_VOIP,
    CALLUI_MUTE_VOIP,
    CALLUI_UNMUTE_VOIP,
    CALLUI_MUTE_VOIP_RINGER,
};

/**
 * @brief Indicates the voip call attribute.
 */
struct VoipCallAttribute {
    /**
     * Indicates the callid
     */
    std::string callId = "";
    /**
     * Indicates the bundleName.
     */
    std::string bundleName = "";
    /**
     * Indicates the type of voip call.
     */
    VoipCallType voipCallType = VoipCallType::VOIP_CALL_VOICE;
    /**
     * Indicates the userName.
     */
    std::string userName = "";
    /**
     * Indicates the user Porfile.
     */
    std::vector<uint8_t> userProfile = {};
    /**
     * Indicates the package Profile.
     */
    std::string abilityName = "";
    /**
     * Indicates the state of voip call.
     */
    VoipCallState voipCallState = VoipCallState::VOIP_CALL_STATE_IDLE;
    /**
     * Indicates the extensionId of voip call.
     */
    std::string extensionId = "";
    /**
     * Indicates the process mode of voip call. 1: main process mode; 0: sub process mode
     */
    std::string processMode = "1";
    /**
     * Indicates whether the VoIP incoming call default show live call banner. Default value is true.
     */
    bool showBannerForIncomingCall = true;
    /**
     * Indicates whether the VoIP incoming call is a conference. Default value is false.
     */
    bool isConferenceCall = false;
    /**
     * Indicates whether the VoIP incoming video call is support voice answer. Default value is true.
     */
    bool isVoiceAnswerSupported = true;
    /**
     * Indicates whether the VoIP incoming call allows user to mute ringtone. Default value is false.
     */
    bool isUserMuteRingToneAllowed = false;
    /**
     * Indicates whether the VoIP call is allowed to dial during carrier call. Default value is false.
     */
    bool isDialingAllowedDuringCarrierCall = false;
    /**
     * Indicates whether the app has the permission to use microphone. Default value is true.
     */
    bool hasMicPermission = true;
    /**
     * Indicates whether a call capsule should be displayed while the app is serving as a foreground process.
     * Default value is false.
     */
    bool isCapsuleSticky = false;
    /**
     * Indicates whether the VoIP call is from Push kit.
    */
    bool isCallByPushKit = true;
    /**
     * Indicates the uid of voip call.
     */
    int32_t uid = 0;
    /**
     * Indicates the fullTokenId of voip call.
     */
    uint64_t fullTokenId = 0;
    /**
     * Indicates the tokenId of voip call.
     */
    uint32_t tokenId = 0;
};

/**
 * @brief Indicates the voip call event.
 */
enum class VoipCallEvent {
    /**
     * Indicates the click event of default.
     */
    VOIP_CALL_EVENT_NONE = 0,
    /**
     * Indicates the click event of answer voice.
     */
    VOIP_CALL_EVENT_ANSWER_VOICE,
    /**
     * Indicates the click event of answer video.
     */
    VOIP_CALL_EVENT_ANSWER_VIDEO,
    /**
     * Indicates the click event of reject.
     */
    VOIP_CALL_EVENT_REJECT,
    /**
     * Indicates the voip call was hung up by other cause.
     */
    VOIP_CALL_EVENT_HANGUP,
    /**
     * Indicates the user clicked muted.
     */
    VOIP_CALL_EVENT_MUTED,
    /**
     * Indicates the user clicked unmuted.
     */
    VOIP_CALL_EVENT_UNMUTED,
    /**
     * Indicates the user clicked speaker on.
     */
    VOIP_CALL_EVENT_SPEAKER_ON,
    /**
     * Indicates the user clicked speaker off.
     */
    VOIP_CALL_EVENT_SPEAKER_OFF,
    /**
     * Indicates whether the VOIP incoming call allows user to mute ringtone.
     */
    VOIP_CALL_EVENT_MUTE_RINGTONE,
    /**
     * Indicates the VOIP call muted and disabled the microphone switch.
     */
    VOIP_CALL_EVENT_MIC_DISABLE,
};

/**
 * @brief Indicates the ErrorReason.
 */
enum class ErrorReason {
    /**
     * Indicates there is no error.
     */
    ERROR_NONE = 0,

    /**
     * Indicates there is already a cellular call.
     */
    CELLULAR_CALL_EXISTS = 1,

    /**
     * Indicates there is already a voip call.
     */
    VOIP_CALL_EXISTS = 2,

    /**
     * Indicates this is a invalid call.
     */
    INVALID_CALL = 3,

    /**
     * Indicates the user answered the cellular call first.
     */
    USER_ANSWER_CELLULAR_FIRST = 4,
};

/**
 * @brief Indicates the voip call Events.
 */
struct VoipCallEventInfo {
    /**
     * Indicates the callid
     */
    std::string voipCallId = "";
    /**
     * Indicates the bundleName.
     */
    std::string bundleName = "";
    /**
     * Indicates the uid.
     */
    int32_t uid = 0;
    /**
     * Indicates the click event of voip.
     */
    VoipCallEvent voipCallEvent = VoipCallEvent::VOIP_CALL_EVENT_NONE;
    /**
     * Indicates the error reason.
     */
    ErrorReason errorReason = ErrorReason::ERROR_NONE;
};

/**
 * @brief Indicates the ErrorReason.
 */
enum class ReportVoipCallFailedCause {
    /**
     * Indicates other failed cause.
     */
    OTHER_CAUSED,
    /**
     * Indicates application line is busy.
     */
    ROUTE_BUSY,
    /**
     * Indicates application failed to establish connect.
     */
    CONNECT_FAILED,
};

struct ReportVoIPCallParams {
    std::string callId = "";
    int32_t status;
};

enum VoipCallErrorCode {
    VOIP_ERR_CALLBACK_NOT_EXIST,
};

enum class VoIPCallStatus : uint32_t {
    ORIGINAL_STATE = 0,

    VOIP_EXTENSION_CONNECT_SUCCEED = 1,
    VOIP_EXTENSION_CONNECT_FAILED = 2,
    VOIP_EXTENSION_BUSY = 3,
    VOIP_EXTENSION_CALLBACK_ILLEAGAL_PARAM = 4,
    VOIP_EXTENSION_OTHER_REASON = 5,

    VOIP_MANAGER_BUSY = 6,
    VOIP_MANAGER_OTHER_REASON = 7,

    LIVE_VIDEO_PUBLISH_SUCCESSED = 13,
    LIVE_VIDEO_PUBLISH_FAILED = 14,

    EXTENSION_LOAD_SUCCEED = 20,
    EXTENSION_LOAD_FAILED = 21,
    REFUSE_ANSWER = 22,
    DELETE_CALL_NOTIFICATION = 23,
    HANG_UP_DURING_CALLING = 24,
    LONGTIME_CALL_NOT_ANSWER = 25,
    CALL_END_OTHER_REASON = 26,
    ANSWERED_SUCCEED = 27,

    HANG_UP_DURING_ANSWERED = 40,
    VOIP_MANAGER_DESTROYED = 41,
    HANG_UP_DURING_ANSWERED_OTHER_REASON = 42,

    VOIP_MANAGER_INVALID_PARAM = 58,
    VOIP_MANAGER_CALL_NOT_EXIST = 59,

    PUSH_MANAGER_SERVICE_LOAD_SUCCESS = 60,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_SYSTEM_ERROR = 61,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_ILLEGAL_EXTENSION = 62,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_CALLS_EXCEEDED = 63,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_VOIP_INTERNAL_ERROR = 64,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_ILLEGAL_CALL_ID = 65,
    PUSH_MANAGER_SERVICE_LOAD_FAILED_OTHER_REASON = 66,

    VOIP_TIMER_TRIGGER = 70,
};
} // namespace Telephony
} // namespace OHOS
#endif
