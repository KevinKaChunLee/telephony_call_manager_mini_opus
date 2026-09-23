/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef CALL_MANAGER_ERRORS_H
#define CALL_MANAGER_ERRORS_H

#include "telephony_errors.h"

namespace OHOS {
namespace Telephony {
/**
 * @brief Indicates the error code of CallManager.
 */
enum CallManagerErrorCode {
    /**
     * Indicates an invalid slot id.
     */
    CALL_ERR_INVALID_SLOT_ID = CALL_ERR_OFFSET,
    /**
     * Indicates an invalid call id.
     */
    CALL_ERR_INVALID_CALLID,
    /**
     * Indicates the phone number is empty.
     */
    CALL_ERR_PHONE_NUMBER_EMPTY,
    /**
     * Indicates number length out of range.
     */
    CALL_ERR_NUMBER_OUT_OF_RANGE,
    /**
     * Indicates network type is not supported.
     */
    CALL_ERR_UNSUPPORTED_NETWORK_TYPE,
    /**
     * Indicates an invalid dial scene, which is used when the dial scene fails to be obtained.
     */
    CALL_ERR_INVALID_DIAL_SCENE,
    /**
     * Indicates an invalid video state, which is used when the video state fails to be obtained.
     */
    CALL_ERR_INVALID_VIDEO_STATE,
    /**
     * Indicates an invalid dial type, which is used when the dial type fails to be obtained.
     */
    CALL_ERR_UNKNOW_DIAL_TYPE,
    /**
     * Indicates an invalid call type, which is used when the call type fails to be obtained.
     */
    CALL_ERR_UNKNOW_CALL_TYPE,
    /**
     * Indicates the call type is illegal.
     */
    CALL_ERR_VIDEO_ILLEGAL_CALL_TYPE,
    /**
     * Indicates the call state is duplicate.
     */
    CALL_ERR_NOT_NEW_STATE,
    /**
     * Indicates the call object is null.
     */
    CALL_ERR_CALL_OBJECT_IS_NULL,
    /**
     * Indicates there is already a call.
     */
    CALL_ERR_DIAL_IS_BUSY,
    /**
     * Indicates current call is not activated.
     */
    CALL_ERR_CALL_IS_NOT_ACTIVATED,
    /**
     * Indicates current call is not on holding state.
     */
    CALL_ERR_CALL_IS_NOT_ON_HOLDING,
    /**
     * Indicates current call operation is not allowed.
     */
    CALL_ERR_ILLEGAL_CALL_OPERATION,
    /**
     * Indicates the current call state is mismatch with the expected call state.
     */
    CALL_ERR_CALL_STATE_MISMATCH_OPERATION,
    /**
     * Indicates the conference call is not exists.
     */
    CALL_ERR_CONFERENCE_NOT_EXISTS,
    /**
     * Indicates the number of calls in the conference exceeds limits.
     */
    CALL_ERR_CONFERENCE_CALL_EXCEED_LIMIT,
    /**
     * Indicates call state error.
     */
    CALL_ERR_CALL_STATE,
    /**
     * Indicates the call count exceeds limit.
     */
    CALL_ERR_CALL_COUNTS_EXCEED_LIMIT,
    /**
     * Indicates the connection is null.
     */
    CALL_ERR_CALL_CONNECTION_NOT_EXIST,
    /**
     * Indicates radio state error.
     */
    CALL_ERR_GET_RADIO_STATE_FAILED,
    /**
     * Indicates emergency call not allowed to join conference.
     */
    CALL_ERR_EMERGENCY_UNSUPPORT_CONFERENCEABLE,
    /**
     * Indicates the device is currently not ringing.
     */
    CALL_ERR_PHONE_ANSWER_IS_BUSY,
    /**
     * Indicates this call has existed.
     */
    CALL_ERR_PHONE_CALL_ALREADY_EXISTS,
    /**
     * Indicates the number of phone calls less than two.
     */
    CALL_ERR_PHONE_CALLS_TOO_FEW,
    /**
     * Indicates unexpected type event occurs.
     */
    CALL_ERR_PHONE_TYPE_UNEXPECTED,
    /**
     * Indicates notify call state failed.
     */
    CALL_ERR_PHONE_CALLSTATE_NOTIFY_FAILED,
    /**
     * Indicates unknown call media type.
     */
    CALL_ERR_VIDEO_ILLEGAL_MEDIA_TYPE,
    /**
     * Indicates video update in progress.
     */
    CALL_ERR_VIDEO_IN_PROGRESS,
    /**
     * Indicates illegal media state.
     */
    CALL_ERR_VIDEO_ILLEAGAL_SCENARIO,
    /**
     * Indicates video mode change notify failed.
     */
    CALL_ERR_VIDEO_MODE_CHANGE_NOTIFY_FAILED,
    /**
     * Indicates dial failed.
     */
    CALL_ERR_DIAL_FAILED,
    /**
     * Indicates answer call failed.
     */
    CALL_ERR_ANSWER_FAILED,
    /**
     * Indicates reject call failed.
     */
    CALL_ERR_REJECT_FAILED,
    /**
     * Indicates holdcall call failed.
     */
    CALL_ERR_HOLD_FAILED,
    /**
     * Indicates unHoldCall call failed.
     */
    CALL_ERR_UNHOLD_FAILED,
    /**
     * Indicates hangUp call failed.
     */
    CALL_ERR_HANGUP_FAILED,
    /**
     * Indicates separate conference failed.
     */
    CALL_ERR_CONFERENCE_SEPERATE_FAILED,
    /**
     * Indicates conference call is not active.
     */
    CALL_ERR_CONFERENCE_CALL_IS_NOT_ACTIVE,
    /**
     * Indicates the call is not in the conference.
     */
    CALL_ERR_THE_CALL_IS_NOT_IN_THE_CONFERENCE,
    /**
     * Indicates start Rtt failed.
     */
    CALL_ERR_STARTRTT_FAILED,
    /**
     * Indicates stop Rtt failed.
     */
    CALL_ERR_STOPRTT_FAILED,
    /**
     * Indicates volte is not supported.
     */
    CALL_ERR_VOLTE_NOT_SUPPORT,
    /**
     * Indicates volte provisioning disabled.
     */
    CALL_ERR_VOLTE_PROVISIONING_DISABLED,
    /**
     * Indicates bluetooth connection failed.
     */
    CALL_ERR_BLUETOOTH_CONNECTION_FAILED,
    /**
     * Indicates set audio device failed.
     */
    CALL_ERR_SETTING_AUDIO_DEVICE_FAILED,
    /**
     * Indicates tone descriptor unknown.
     */
    CALL_ERR_AUDIO_UNKNOWN_TONE,
    /**
     * Indicates play call tone failed.
     */
    CALL_ERR_AUDIO_TONE_PLAY_FAILED,
    /**
     * Indicates stop call tone failed.
     */
    CALL_ERR_AUDIO_TONE_STOP_FAILED,
    /**
     * Indicates set mute failed.
     */
    CALL_ERR_AUDIO_SETTING_MUTE_FAILED,
    /**
     * Indicates set audio device failed.
     */
    CALL_ERR_AUDIO_SET_AUDIO_DEVICE_FAILED,
    /**
     * Indicates start vibrate failed.
     */
    CALL_ERR_AUDIO_START_VIBRATE_FAILED,
    /**
     * Indicates cancel vibrate failed.
     */
    CALL_ERR_AUDIO_CANCEL_VIBRATE_FAILED,
    /**
     * Indicates some error happpens in audio interface.
     */
    CALL_ERR_AUDIO_OPERATE_FAILED,
    /**
     * Indicates invalid coordinates.
     */
    CALL_ERR_VIDEO_INVALID_COORDINATES,
    /**
     * Indicates invalid zoom.
     */
    CALL_ERR_VIDEO_INVALID_ZOOM,
    /**
     * Indicates invalid rotation.
     */
    CALL_ERR_VIDEO_INVALID_ROTATION,
    /**
     * Indicates camera id is error.
     */
    CALL_ERR_VIDEO_INVALID_CAMERA_ID,
    /**
     * Indicates invalid path.
     */
    CALL_ERR_INVALID_PATH,
    /**
     * Indicates camera not turned on.
     */
    CALL_ERR_CAMERA_NOT_TURNED_ON,
    /**
     * Indicates callback already exist.
     */
    CALL_ERR_CALLBACK_ALREADY_EXIST,
    /**
     * Indicates callback is null.
     */
    CALL_ERR_CALLBACK_NOT_EXIST,
    /**
     * Indicates napi interface failed.
     */
    CALL_ERR_NAPI_INTERFACE_FAILED,
    /**
     * Indicates dtmf exceed limit.
     */
    CALL_ERR_DTMF_EXCEED_LIMIT,
    /**
     * Indicates invalid restriction type.
     */
    CALL_ERR_INVALID_RESTRICTION_TYPE,
    /**
     * Indicates invalid restriction mode.
     */
    CALL_ERR_INVALID_RESTRICTION_MODE,
    /**
     * Indicates invalid transfer type.
     */
    CALL_ERR_INVALID_TRANSFER_TYPE,
    /**
     * Indicates invalid transfer setting type.
     */
    CALL_ERR_INVALID_TRANSFER_SETTING_TYPE,
    /**
     * Indicates invalid transfer time.
     */
    CALL_ERR_INVALID_TRANSFER_TIME,
    /**
     * Indicates format phone number failed.
     */
    CALL_ERR_FORMAT_PHONE_NUMBER_FAILED,
    /**
     * Indicates handle system event failed.
     */
    CALL_ERR_SYSTEM_EVENT_HANDLE_FAILURE,
    /**
     * Indicates service dump failed.
     */
    CALL_ERR_SERVICE_DUMP_FAILED,
    /**
     * Indicates function not supported.
     */
    CALL_ERR_FUNCTION_NOT_SUPPORTED,
    /**
     * Indicates call not support video.
     */
    CALL_ERR_VIDEO_NOT_SUPPORTED,
    /**
     * Indicates ims call supplyment doesn't connect.
     */
    CALL_ERR_UT_NO_CONNECTION,
    /**
     * Indicates parmas input phone number not in phone.
     */
    CALL_ERR_INVALID_CALL_NUMBER,
    /**
     * Indicates operation too frequent.
     */
    CALL_ERR_OPERATION_TOO_FREQUENT,
};

/**
 * @brief Indicates the tye of protocol error.
 *
 * 3GPP TS 24.008 V3.9.0 (2001-09)  10.5.4.11 Cause
 */
enum PROTOCOL_ERROR_TYPE {
    /**
     * Indicates parameter out of range.
     */
    CALL_ERR_PARAMETER_OUT_OF_RANGE = PROTOCOL_ERR_OFFSET,
    /**
     * Indicates call completed elsewhere.
     */
    CALL_ERR_CALL_ALREADY_EXISTS,
    /**
     * Indicates radio state error, network out of order.
     */
    CALL_ERR_RADIO_STATE,
    /**
     * Indicates resources unavailable, unspecified.
     */
    CALL_ERR_RESOURCE_UNAVAILABLE,
    /**
     * Indicates service or option not available.
     */
    CALL_ERR_OPTION_NOT_AVAILABLE,
    /**
     * Indicates service or option not implemented.
     */
    CALL_ERR_OPTION_NOT_IMPLEMENTED,
    /**
     * Indicates forbidden dial call when transfer call alive.
     */
    CALL_ERR_TRANSFER_CALL_ALIVE_FORBIDDEN_DIAL,
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_MANAGER_ERRORS_H
