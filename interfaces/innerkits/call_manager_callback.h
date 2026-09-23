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

#ifndef CALL_MANAGER_CALLBACK_H
#define CALL_MANAGER_CALLBACK_H

#include <cstdio>

#include "call_manager_inner_type.h"

namespace OHOS {
namespace Telephony {
class CallManagerCallback {
public:
    /**
     * @brief Construct a new Call Manager Callback object
     */
    CallManagerCallback() {}

    /**
     * @brief Destroy the Call Manager Callback object
     */
    virtual ~CallManagerCallback() {}

    /**
     * @brief indicate the call detail informaion while the call state is changed
     *
     * @param info[in] call detail infomation, contains call type, account number, call state,
     * call start or end time .etc
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnCallDetailsChange(const CallAttributeInfo &info) = 0;

    /**
     * @brief indicate the meeTime detail informaion while the call state is changed
     *
     * @param info[in] meeTime detail infomation, contains call type, account number, call state,
     * call start or end time .etc
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnMeeTimeDetailsChange(const CallAttributeInfo &info)
    {
        return -1;
    }

    /**
     * @brief indicate the call event informaion while the call event is changed
     *
     * @param info[in] call event information, contains event id, phone number, bundle name.
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnCallEventChange(const CallEventInfo &info) = 0;

    /**
     * @brief indicate the call disconnected detail information while the call disconnected
     *
     * @param details[in], call disconnected information, contains disconnected reason and message.
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnCallDisconnectedCause(const DisconnectedDetails &details) = 0;

    /**
     * @brief report the result infomation for the specific call event
     *
     * @param reportId[in] the event id, indicate the specific event that the report information about
     * @param resultInfo[in] the detail information corresponding specific event
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnReportAsyncResults(CallResultReportId reportId, AppExecFwk::PacMap &resultInfo) = 0;

    /**
     * @brief report the result infomation for the specific ott call event
     *
     * @param requestId[in] the event id, indicate the specific ott call event that the report information about
     * @param info[in] the detail information corresponding specific event
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnOttCallRequest(OttCallRequestId requestId, AppExecFwk::PacMap &info) = 0;

    /**
     * @brief report the mmi code result
     *
     * @param info[out] the mmi code result, contains result(success or fail) and the corresponding message.
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnReportMmiCodeResult(const MmiCodeInfo &info) = 0;

    /**
     * @brief report the audio device detail information
     *
     * @param info[out] the audio information, contains the audio device list, current audio device and ismuted.
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnReportAudioDeviceChange(const AudioDeviceInfo &info) = 0;

    virtual int32_t OnReportPostDialDelay(const std::string &str) = 0;

    virtual int32_t OnUpdateImsCallModeChange(const CallMediaModeInfo &imsCallModeInfo) = 0;
    virtual int32_t OnCallSessionEventChange(const CallSessionEvent &callSessionEventOptions) = 0;
    virtual int32_t OnPeerDimensionsChange(const PeerDimensionsDetail &peerDimensionsDetail) = 0;
    virtual int32_t OnCallDataUsageChange(const int64_t dataUsage) = 0;
    virtual int32_t OnUpdateCameraCapabilities(const CameraCapabilities &cameraCapabilities) = 0;
    virtual int32_t OnReportRttCallEvtChanged(const RttEvent &info) { return 0; }
    virtual int32_t OnReportRttCallError(const RttError &info) { return 0; }
    virtual int32_t OnReportRttCallMessage(AppExecFwk::PacMap &msgResult) { return 0; }

    /**
     * @brief report phone state change
     *
     * @param numActive[in] number of active calls
     * @param numHeld[in] number of hold calls
     * @param callState[in] call state
     * @param number[in] phone number
     * @return Returns 0 on success, others on failure.
     */
    virtual int32_t OnPhoneStateChange(int32_t numActive, int32_t numHeld, int32_t callState,
        const std::string &number)
    {
        return 0;
    }
};
} // namespace Telephony
} // namespace OHOS
#endif // CALL_MANAGER_CALLBACK_H
