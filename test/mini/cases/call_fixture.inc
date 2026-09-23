/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

// Test doubles and helpers for the call manager cases: a protocol stack the tests drive like a modem
// (commands are recorded, call lists are reported through the registered uplink), SIM/network and
// audio doubles, and an application callback that records what it is told. Business state is only
// ever read on the main loop (OnLoop) or after Settle(), which lets the chain of posted tasks drain.

#include <atomic>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "audio_control_manager.h"
#include "call_audio_lite_interface.h"
#include "call_control_manager.h"
#include "call_request_event_handler_helper.h"
#include "call_manager_client.h"
#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "call_manager_service.h"
#include "call_object_manager.h"
#include "call_permission_adapter.h"
#include "cellular_call_lite_interface.h"
#include "core_service_lite_interface.h"
#include "los_event_handler.h"
#include "string_ex.h"

namespace CallFixture {
using namespace OHOS;
using namespace OHOS::Telephony;

constexpr uint32_t WAIT_MS = 3000;
constexpr int32_t SA_ID = 4005;
constexpr const char *TRUSTED_PERMISSIONS = "ohos.permission.PLACE_CALL;ohos.permission.ANSWER_CALL;"
    "ohos.permission.SET_TELEPHONY_STATE;ohos.permission.GET_TELEPHONY_STATE;";
constexpr const char *TRUSTED_BUNDLE = "com.example.dialer";

struct Command {
    std::string name;
    CellularCallInfo info;
    int32_t type = 0;
};

class FakeStack : public CellularCallLiteInterface {
public:
    int32_t Dial(const CellularCallInfo &callInfo) override
    {
        Record("Dial", callInfo);
        return dialRet;
    }
    int32_t HangUp(const CellularCallInfo &callInfo, CallSupplementType type) override
    {
        Record("HangUp", callInfo, static_cast<int32_t>(type));
        return hangUpRet;
    }
    int32_t Answer(const CellularCallInfo &callInfo) override
    {
        Record("Answer", callInfo);
        return answerRet;
    }
    int32_t Reject(const CellularCallInfo &callInfo) override
    {
        Record("Reject", callInfo);
        return TELEPHONY_SUCCESS;
    }
    int32_t HoldCall(const CellularCallInfo &callInfo) override
    {
        Record("Hold", callInfo);
        return TELEPHONY_SUCCESS;
    }
    int32_t UnHoldCall(const CellularCallInfo &callInfo) override
    {
        Record("UnHold", callInfo);
        return TELEPHONY_SUCCESS;
    }
    int32_t SwitchCall(const CellularCallInfo &callInfo) override
    {
        Record("Switch", callInfo);
        return TELEPHONY_SUCCESS;
    }
    int32_t IsEmergencyPhoneNumber(int32_t slotId, const std::string &phoneNum, bool &enabled) override
    {
        enabled = (phoneNum == "112" || phoneNum == "911");
        return TELEPHONY_SUCCESS;
    }
    int32_t HangUpAllConnection() override
    {
        Record("HangUpAll", CellularCallInfo());
        return TELEPHONY_SUCCESS;
    }
    int32_t RegisterCallManagerCallBack(const sptr<ICallStatusCallback> &cb) override
    {
        registrations.fetch_add(1);
        if (registerRet == TELEPHONY_SUCCESS) {
            callback = cb;
        }
        return registerRet;
    }
    int32_t UnRegisterCallManagerCallBack() override
    {
        unregistrations.fetch_add(1);
        callback = nullptr;
        return TELEPHONY_SUCCESS;
    }

    std::string NumberOf(size_t position) const
    {
        return (position < commands.size()) ? std::string(commands[position].info.phoneNum) : std::string();
    }
    std::string LastNumber() const
    {
        return commands.empty() ? std::string() : std::string(commands.back().info.phoneNum);
    }
    int CountOf(const std::string &name) const
    {
        int count = 0;
        for (const Command &command : commands) {
            count += (command.name == name) ? 1 : 0;
        }
        return count;
    }

    // Network side, driven from the test task: sets one call and reports the whole list.
    int32_t Set(int32_t index, TelCallState state, const char *number, CallType type = CallType::TYPE_CS)
    {
        CallReportInfo &info = calls[index];
        info.index = index;
        info.accountId = 0;
        info.callType = type;
        info.callMode = VideoStateType::TYPE_VOICE;
        info.state = state;
        info.voiceDomain = (type == CallType::TYPE_IMS) ? 1 : 0;
        (void)std::memset(info.accountNum, 0, sizeof(info.accountNum));
        (void)std::strncpy(info.accountNum, number, kMaxNumberLen);
        return Report();
    }
    int32_t Drop(int32_t index)
    {
        calls.erase(index);
        return Report();
    }
    int32_t Report()
    {
        CallsReportInfo report;
        report.slotId = 0;
        for (const auto &entry : calls) {
            report.callVec.push_back(entry.second);
        }
        sptr<ICallStatusCallback> uplink = callback;
        return (uplink == nullptr) ? TELEPHONY_ERR_LOCAL_PTR_NULL : uplink->UpdateCallsReportInfo(report);
    }

    std::vector<Command> commands;
    std::map<int32_t, CallReportInfo> calls;
    sptr<ICallStatusCallback> callback;
    std::atomic<int> registrations { 0 };
    std::atomic<int> unregistrations { 0 };
    int32_t dialRet = TELEPHONY_SUCCESS;
    int32_t hangUpRet = TELEPHONY_SUCCESS;
    int32_t answerRet = TELEPHONY_SUCCESS;
    int32_t registerRet = TELEPHONY_SUCCESS;

private:
    void Record(const char *name, const CellularCallInfo &info, int32_t type = 0)
    {
        Command command;
        command.name = name;
        command.info = info;
        command.type = type;
        commands.push_back(command);
    }
};

class FakeCoreService : public CoreServiceLiteInterface {
public:
    int32_t HasSimCard(int32_t slotId, bool &hasSimCard) override
    {
        hasSimCard = hasSim;
        return TELEPHONY_SUCCESS;
    }
    int32_t IsNetworkInService(int32_t slotId, bool &inService) override
    {
        inService = this->inService;
        return TELEPHONY_SUCCESS;
    }
    bool hasSim = true;
    bool inService = true;
};

class FakeAudio : public CallAudioLiteInterface {
public:
    int32_t SetCallAudioActive(bool active) override
    {
        log.push_back(active ? "audio-on" : "audio-off");
        return TELEPHONY_SUCCESS;
    }
    int32_t StartRingtone() override
    {
        log.push_back("ring-start");
        return TELEPHONY_SUCCESS;
    }
    int32_t StopRingtone() override
    {
        log.push_back("ring-stop");
        return TELEPHONY_SUCCESS;
    }
    int32_t PlayWaitingTone() override
    {
        log.push_back("wait-start");
        return TELEPHONY_SUCCESS;
    }
    int32_t StopWaitingTone() override
    {
        log.push_back("wait-stop");
        return TELEPHONY_SUCCESS;
    }
    std::vector<std::string> log;
};

struct Seen {
    int32_t callId;
    TelCallState state;
};

// Written on the main loop by RecordingCallback; read after Settle().
struct Recorder {
    std::vector<Seen> states;
    std::vector<CallEventInfo> events;
    int causes = 0;
    int32_t detailsResult = TELEPHONY_SUCCESS;
    std::function<void(const CallAttributeInfo &)> onDetails;

    void Clear()
    {
        states.clear();
        events.clear();
        causes = 0;
        detailsResult = TELEPHONY_SUCCESS;
        onDetails = nullptr;
    }
    std::vector<TelCallState> StatesOf(int32_t callId) const
    {
        std::vector<TelCallState> out;
        for (const Seen &seen : states) {
            if (seen.callId == callId) {
                out.push_back(seen.state);
            }
        }
        return out;
    }
    int32_t LastCallId() const
    {
        return states.empty() ? -1 : states.back().callId;
    }
    TelCallState FirstStateOf(int32_t callId) const
    {
        std::vector<TelCallState> seen = StatesOf(callId);
        return seen.empty() ? TelCallState::CALL_STATUS_UNKNOWN : seen.front();
    }
    TelCallState LastStateOf(int32_t callId) const
    {
        std::vector<TelCallState> seen = StatesOf(callId);
        return seen.empty() ? TelCallState::CALL_STATUS_UNKNOWN : seen.back();
    }
    TelCallState LastState() const
    {
        return states.empty() ? TelCallState::CALL_STATUS_UNKNOWN : states.back().state;
    }
};

Recorder g_recorder;

class RecordingCallback : public CallManagerCallback {
public:
    int32_t OnCallDetailsChange(const CallAttributeInfo &info) override
    {
        g_recorder.states.push_back({ info.callId, info.callState });
        if (g_recorder.onDetails) {
            g_recorder.onDetails(info);
        }
        return g_recorder.detailsResult;
    }
    int32_t OnCallEventChange(const CallEventInfo &info) override
    {
        g_recorder.events.push_back(info);
        return TELEPHONY_SUCCESS;
    }
    int32_t OnCallDisconnectedCause(const DisconnectedDetails &details) override
    {
        g_recorder.causes++;
        return TELEPHONY_SUCCESS;
    }
    int32_t OnReportAsyncResults(CallResultReportId reportId, AppExecFwk::PacMap &resultInfo) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnOttCallRequest(OttCallRequestId requestId, AppExecFwk::PacMap &info) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnReportMmiCodeResult(const MmiCodeInfo &info) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnReportAudioDeviceChange(const AudioDeviceInfo &info) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnReportPostDialDelay(const std::string &str) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnUpdateImsCallModeChange(const CallMediaModeInfo &imsCallModeInfo) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnCallSessionEventChange(const CallSessionEvent &callSessionEventOptions) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnPeerDimensionsChange(const PeerDimensionsDetail &peerDimensionsDetail) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnCallDataUsageChange(const int64_t dataUsage) override
    {
        return TELEPHONY_SUCCESS;
    }
    int32_t OnUpdateCameraCapabilities(const CameraCapabilities &cameraCapabilities) override
    {
        return TELEPHONY_SUCCESS;
    }
};

sptr<FakeStack> g_stack;
sptr<FakeCoreService> g_core;
sptr<FakeAudio> g_audio;

std::shared_ptr<CallManagerClient> Client()
{
    return DelayedSingleton<CallManagerClient>::GetInstance();
}

// Runs fn on the main loop and waits for it.
bool OnLoop(const std::function<void()> &fn)
{
    return TelMiniMainLoop().PostSyncTask(fn, WAIT_MS) == TELEPHONY_SUCCESS;
}

// Lets reports, their state updates and the callbacks they post run to completion.
void Settle()
{
    constexpr int rounds = 4;
    for (int i = 0; i < rounds; i++) {
        OnLoop([]() {});
    }
}

void UseTrustedCaller()
{
    CallPermissionAdapter::OverrideForTest(TRUSTED_PERMISSIONS, true, TRUSTED_BUNDLE);
}

// Registers the doubles (the stack unless withStack is false) and brings the service up.
bool Start(bool withStack = true, bool withAudio = true)
{
    g_recorder.Clear();
    UseTrustedCaller();
    g_stack = new FakeStack();
    g_core = new FakeCoreService();
    g_audio = new FakeAudio();
    CoreServiceLiteRegistry::Register(g_core);
    CallAudioLiteRegistry::Register(withAudio ? sptr<CallAudioLiteInterface>(g_audio) : nullptr);
    CellularCallLiteRegistry::Register(withStack ? sptr<CellularCallLiteInterface>(g_stack) : nullptr);
    if (CallManagerServiceMiniStart() != TELEPHONY_SUCCESS) {
        return false;
    }
    Client()->Init(SA_ID);
    Settle();
    return !withStack || g_stack->callback != nullptr;
}

bool RegisterRecorder()
{
    return Client()->RegisterCallBack(std::make_unique<RecordingCallback>()) == TELEPHONY_SUCCESS;
}

void Stop()
{
    Client()->UnInit();
    CallManagerServiceMiniStop();
    CellularCallLiteRegistry::Register(nullptr);
    CoreServiceLiteRegistry::Register(nullptr);
    CallAudioLiteRegistry::Register(nullptr);
    CallPermissionAdapter::OverrideForTest(nullptr, false, nullptr);
    g_recorder.Clear();
    g_stack = nullptr;
    g_core = nullptr;
    g_audio = nullptr;
}

AppExecFwk::PacMap VoiceExtras()
{
    AppExecFwk::PacMap extras;
    extras.PutIntValue("accountId", 0);
    extras.PutIntValue("videoState", static_cast<int32_t>(VideoStateType::TYPE_VOICE));
    extras.PutIntValue("dialScene", static_cast<int32_t>(DialScene::CALL_NORMAL));
    extras.PutIntValue("dialType", static_cast<int32_t>(DialType::DIAL_CARRIER_TYPE));
    extras.PutIntValue("callType", static_cast<int32_t>(CallType::TYPE_CS));
    return extras;
}

int32_t Dial(const std::u16string &number)
{
    AppExecFwk::PacMap extras = VoiceExtras();
    return Client()->DialCall(number, extras);
}

size_t CallCount()
{
    size_t count = 0;
    OnLoop([&count]() { count = CallObjectManager::GetAllCallList().size(); });
    return count;
}

sptr<CallBase> CallByIndex(int32_t index)
{
    sptr<CallBase> call;
    OnLoop([&call, index]() { call = CallObjectManager::GetOneCallObjectByIndexAndSlotId(index, 0); });
    return call;
}

int32_t CallIdOf(int32_t index)
{
    sptr<CallBase> call = CallByIndex(index);
    return (call == nullptr) ? -1 : call->GetCallID();
}

TelCallState StateOf(int32_t index)
{
    TelCallState state = TelCallState::CALL_STATUS_UNKNOWN;
    OnLoop([&state, index]() {
        sptr<CallBase> call = CallObjectManager::GetOneCallObjectByIndexAndSlotId(index, 0);
        if (call != nullptr) {
            state = call->GetTelCallState();
        }
    });
    return state;
}

// Dials and lets the network take the call to ACTIVE on index `index`.
int32_t EstablishOutgoing(const std::u16string &number, int32_t index)
{
    int32_t ret = Dial(number);
    if (ret != TELEPHONY_SUCCESS) {
        return ret;
    }
    std::string narrow = Str16ToStr8(number);
    g_stack->Set(index, TelCallState::CALL_STATUS_DIALING, narrow.c_str());
    g_stack->Set(index, TelCallState::CALL_STATUS_ALERTING, narrow.c_str());
    g_stack->Set(index, TelCallState::CALL_STATUS_ACTIVE, narrow.c_str());
    Settle();
    return TELEPHONY_SUCCESS;
}

bool Contains(const std::vector<TelCallState> &states, const std::vector<TelCallState> &expected)
{
    size_t next = 0;
    for (TelCallState state : states) {
        if (next < expected.size() && state == expected[next]) {
            next++;
        }
    }
    return next == expected.size();
}
} // namespace CallFixture
