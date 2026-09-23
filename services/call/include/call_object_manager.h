/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef CALL_OBJECT_MANAGER_H
#define CALL_OBJECT_MANAGER_H

#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <memory>
#include "ffrt.h"

#include "refbase.h"

#include "call_base.h"
#include "common_type.h"
#include "call_manager_inner_type.h"

namespace OHOS {
namespace Telephony {
class CallObjectManager {
public:
    CallObjectManager();
    virtual ~CallObjectManager();

    static int32_t AddOneCallObject(sptr<CallBase> &call);
    static int32_t AddOneVoipCallObject(CallAttributeInfo info);
    static void DelayedDisconnectCallConnectAbility(uint64_t time);
    static int32_t DeleteOneCallObject(int32_t callId);
    static int32_t DeleteOneVoipCallObject(int32_t callId);
    static void DeleteOneCallObject(sptr<CallBase> &call);
    static sptr<CallBase> GetOneCallObject(int32_t callId);
    static sptr<CallBase> GetOneCallObject(std::string &phoneNumber);
    static sptr<CallBase> GetOneCallObjectByIndex(int32_t index);
    static sptr<CallBase> GetOneCallObjectByIndexAndSlotId(int32_t index, int32_t slotId);
    static sptr<CallBase> GetOneCallObjectByIndexSlotIdAndCallType(int32_t index, int32_t slotId, CallType callType,
        int32_t phoneIndex);
    static sptr<CallBase> GetOneCallObjectByVoipCallId(std::string voipCallId, std::string bundleName, int32_t uid);
    static sptr<CallBase> GetDialingCall();
    static void UpdateOneCallObjectByCallId(int32_t callId, TelCallState nextCallState);
    static int32_t UpdateOneVoipCallObjectByCallId(int32_t callId, TelCallState nextCallState);
    static int32_t HasNewCall();
    static int32_t IsNewCallAllowedCreate(bool &enabled);
    static int32_t GetCurrentCallNum();
    static int32_t GetCarrierCallList(std::list<int32_t> &list);
    static int32_t GetVoipCallNum(bool isOnlyIncludeNonVirtualVoIP = false);
    static int32_t GetVoipCallList(std::list<int32_t> &list);
    static bool HasRingingMaximum();
    static bool HasDialingMaximum();
    static int32_t HasEmergencyCall(bool &enabled);
    static int32_t GetNewCallId();
    static bool IsCallExist(int32_t callId);
    static bool IsCallExist(std::string &phoneNumber);
    static bool HasCallExist();
    static bool HasActivedCallExist(int32_t &callId, bool isIncludeVoipCall);
    static int32_t HasRingingCall(bool &hasRingingCall);
    static int32_t HasHoldCall(bool &hasHoldCall);
    static TelCallState GetCallState(int32_t callId);
    static sptr<CallBase> GetOneCallObject(CallRunningState callState);
    static sptr<CallBase> GetOneCarrierCallObject(CallRunningState callState);
    static bool IsCallExist(CallType type, TelCallState callState);
    static bool IsCallExist(TelCallState callState);
    static bool IsCallExist(TelCallState callState, int32_t &callId);
    static bool IsConferenceCallExist(TelConferenceState state, int32_t &callId);
    static int32_t GetCallNum(TelCallState callState, bool isIncludeVoipCall = true);
    static std::string GetCallNumber(TelCallState callState, bool isIncludeVoipCall = true);
    static CallAttributeInfo GetVoipCallInfo();
    static CallAttributeInfo GetActiveVoipCallInfo();
    static void ClearVoipList();
    static std::vector<CallAttributeInfo> GetCallInfoList(int32_t slotId, bool isIncludeVoipCall = true);
    static sptr<CallBase> GetForegroundCall(bool isIncludeVoipCall = true);
    static sptr<CallBase> GetForegroundLiveCall(bool isIncludeVoipCall = true);
    static sptr<CallBase> GetIncomingCall(bool isIncludeVoipCall = true);
    static sptr<CallBase> GetAudioLiveCall();
    static std::vector<CallAttributeInfo> GetAllCallInfoList(bool isIncludeVoipCall = true);
    static std::vector<CallAttributeInfo> GetVoipCallInfoList();
    int32_t DealFailDial(sptr<CallBase> call);
    int32_t ReportCallDisconnected(sptr<CallBase> call);
    static bool HasVideoCall();
    static std::list<sptr<CallBase>> GetAllCallList();
    static bool HasCellularCallExist();
    static bool HasVoipCallExist();
    static bool HasOtherBtCallExist(int32_t callId);
    static bool IsVoipCallExist();
    static bool IsVoipCallExist(TelCallState callState, int32_t &callId);
    static bool HasIncomingCallCrsType();
    static bool HasIncomingCallVideoRingType();
    static CellularCallInfo GetDialCallInfo();
    static bool HasSatelliteCallExist();
    static int32_t GetSatelliteCallList(std::list<int32_t> &list);
    static int32_t GetCallNumByRunningState(CallRunningState callState);
    static sptr<CallBase> GetForegroundLiveCallByCallId(int32_t callId);
    static bool IsNeedSilentInDoNotDisturbMode();
    static bool IsVideoRing(const std::string &personalNotificationRingtone, const std::string &ringtonePath);
    static bool HasRttCall();
#ifdef NOT_SUPPORT_MULTICALL
    static bool HasBtCallWithDifferentNumber(const std::string &accountNumber);
#endif

#ifdef CALL_MANAGER_CALL_TRANSFER
    static bool HasNonTransferCall();
    static sptr<CallBase> GetIncomingBtTransferCall();
    static bool HasTransferCall();
    static sptr<CallBase> GetOtherBtTransferCall(int32_t excludeCallId);
#endif

    static void ConnectAbilityIfNeed(sptr<CallBase> &call);

protected:
    static ffrt::condition_variable cv_;
    static bool isFirstDialCallAdded_;
    static bool needWaitHold_;
    static CellularCallInfo dialCallInfo_;

private:
    static std::list<sptr<CallBase>> callObjectPtrList_;
    static std::map<int32_t, CallAttributeInfo> voipCallObjectList_;
    static ffrt::mutex listMutex_;
    static int32_t callId_;
};
} // namespace Telephony
} // namespace OHOS

#endif // CALL_OBJECT_MANAGER_H
