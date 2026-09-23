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

// Call objects, the call table and the state listener (tasks 4.1-4.3). The call table is main-loop
// state, so these cases drive it on the loop of a started service.

#include "call_state_listener.h"
#include "cs_call.h"
#include "ims_call.h"

namespace CallObjectCases {
using namespace OHOS;
using namespace OHOS::Telephony;

sptr<CallBase> MakeCall(CallType type, int32_t index, bool isEcc, const char *number)
{
    DialParaInfo info;
    info.callId = CallObjectManager::GetNewCallId();
    info.index = index;
    info.accountId = 0;
    info.number = number;
    info.callType = type;
    info.isEcc = isEcc;
    info.callState = TelCallState::CALL_STATUS_IDLE;
    if (type == CallType::TYPE_IMS) {
        return new IMSCall(info);
    }
    return new CSCall(info);
}

class CountingObserver : public CallStateListenerBase {
public:
    void CallStateUpdated(sptr<CallBase> &call, TelCallState priorState, TelCallState nextState) override
    {
        updates++;
    }
    int updates = 0;
};
} // namespace CallObjectCases

MINI_TEST(CallObject, AttributesAndRunningStateFollowTheCallState)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    using namespace CallObjectCases;
    ASSERT_TRUE(Start());
    OnLoop([]() {
        sptr<CallBase> call = MakeCall(CallType::TYPE_CS, 3, false, "10086");
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_CREATE);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_DIALING), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_DIALING);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_ALERTING), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_DIALING);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_ACTIVE), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_ACTIVE);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_ACTIVE), CALL_ERR_NOT_NEW_STATE);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_HOLDING), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_HOLD);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_DISCONNECTING), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_ENDING);
        EXPECT_EQ(call->SetTelCallState(TelCallState::CALL_STATUS_DISCONNECTED), TELEPHONY_SUCCESS);
        EXPECT_EQ(call->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_ENDED);
        call->SetAccountNumber("10010");
        call->SetPolicyFlag(POLICY_FLAG_HANG_UP_ACTIVE);
        CallAttributeInfo info;
        call->GetCallAttributeInfo(info);
        EXPECT_TRUE(std::string(info.accountNumber) == "10010");
        EXPECT_EQ(info.index, 3);
        EXPECT_EQ(info.callType, CallType::TYPE_CS);
        EXPECT_EQ(info.callState, TelCallState::CALL_STATUS_DISCONNECTED);
        EXPECT_TRUE(call->GetPolicyFlag() & POLICY_FLAG_HANG_UP_ACTIVE);
        sptr<CallBase> incoming = MakeCall(CallType::TYPE_IMS, 4, false, "10086");
        incoming->SetTelCallState(TelCallState::CALL_STATUS_INCOMING);
        EXPECT_EQ(incoming->GetCallRunningState(), CallRunningState::CALL_RUNNING_STATE_RINGING);
        EXPECT_TRUE(incoming->IsCurrentRinging());
    });
    Stop();
}

// 复用头文件中声明的视频方法 (mini-feature-scope-build)
MINI_TEST(CallObject, VideoAndConferenceActionsFailWithoutStateChange)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    using namespace CallObjectCases;
    ASSERT_TRUE(Start());
    OnLoop([]() {
        sptr<CallBase> base = MakeCall(CallType::TYPE_IMS, 2, false, "10086");
        base->SetTelCallState(TelCallState::CALL_STATUS_ACTIVE);
        sptr<IMSCall> ims = static_cast<IMSCall *>(base.GetRefPtr());
        std::string text = "x";
        CallModeReportInfo response;
        EXPECT_EQ(ims->InitVideoCall(), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->UpdateImsCallMode(ImsCallMode::CALL_MODE_SEND_RECEIVE), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->SendUpdateCallMediaModeRequest(ImsCallMode::CALL_MODE_SEND_RECEIVE),
            CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->RecieveUpdateCallMediaModeRequest(response), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->ControlCamera(text, 0, 0), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->SetPausePicture(text), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->SetDeviceDirection(90), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_EQ(ims->CancelCallUpgrade(), CALL_ERR_VIDEO_NOT_SUPPORTED);
        EXPECT_FALSE(ims->IsSupportVideoCall());
        EXPECT_EQ(base->CombineConference(), CALL_ERR_FUNCTION_NOT_SUPPORTED);
        EXPECT_EQ(base->SeparateConference(), CALL_ERR_FUNCTION_NOT_SUPPORTED);
        EXPECT_EQ(base->GetTelCallState(), TelCallState::CALL_STATUS_ACTIVE);
        EXPECT_EQ(base->GetVideoStateType(), VideoStateType::TYPE_VOICE);
        EXPECT_EQ(base->GetTelConferenceState(), TelConferenceState::TEL_CONFERENCE_IDLE);
    });
    EXPECT_EQ(g_stack->commands.size(), 0U);
    Stop();
}

// 普通槽位已满 + 普通槽位已满时拨打紧急号码 (at the table level; the dial path is in call_flow_cases)
MINI_TEST(CallObjectManager, LookupRemovalAndCapacity)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    using namespace CallObjectCases;
    ASSERT_TRUE(Start());
    OnLoop([]() {
        std::vector<sptr<CallBase>> calls;
        for (int32_t index = 1; index < static_cast<int32_t>(MINI_MAX_CALL_OBJECTS); index++) {
            sptr<CallBase> call = MakeCall(CallType::TYPE_CS, index, false, "10086");
            EXPECT_EQ(CallObjectManager::AddOneCallObject(call), TELEPHONY_SUCCESS);
            calls.push_back(call);
        }
        EXPECT_EQ(CallObjectManager::AddOneCallObject(calls[0]), CALL_ERR_PHONE_CALL_ALREADY_EXISTS);
        sptr<CallBase> normal = MakeCall(CallType::TYPE_CS, 7, false, "10000");
        EXPECT_EQ(CallObjectManager::AddOneCallObject(normal), CALL_ERR_CALL_COUNTS_EXCEED_LIMIT);
        sptr<CallBase> ecc = MakeCall(CallType::TYPE_CS, 8, true, "112");
        EXPECT_EQ(CallObjectManager::AddOneCallObject(ecc), TELEPHONY_SUCCESS);
        sptr<CallBase> another = MakeCall(CallType::TYPE_CS, 9, true, "911");
        EXPECT_EQ(CallObjectManager::AddOneCallObject(another), CALL_ERR_CALL_COUNTS_EXCEED_LIMIT);
        int32_t secondId = calls[1]->GetCallID();
        EXPECT_TRUE(CallObjectManager::GetOneCallObject(secondId) == calls[1]);
        EXPECT_TRUE(CallObjectManager::GetOneCallObjectByIndexAndSlotId(2, 0) == calls[1]);
        EXPECT_TRUE(CallObjectManager::GetOneCallObjectByIndexAndSlotId(2, 1) == nullptr);
        EXPECT_TRUE(CallObjectManager::GetOneCallObjectByIndexSlotIdAndCallType(2, 0, CallType::TYPE_CS, 0) ==
            calls[1]);
        CallObjectManager::DeleteOneCallObject(secondId);
        EXPECT_TRUE(CallObjectManager::GetOneCallObject(secondId) == nullptr);
        EXPECT_TRUE(CallObjectManager::GetOneCallObjectByIndexAndSlotId(2, 0) == nullptr);
        for (sptr<CallBase> call : CallObjectManager::GetAllCallList()) {
            CallObjectManager::DeleteOneCallObject(call);
        }
        EXPECT_FALSE(CallObjectManager::HasCallExist());
    });
    Stop();
}

// 每次迁移向每个订阅者广播恰好一次 + 主循环限定断言
MINI_TEST(CallStateListener, EachObserverHearsEachTransitionOnce)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    using namespace CallObjectCases;
    ASSERT_TRUE(Start());
    uint32_t violationsBefore = TelMiniConfinementViolations();
    OnLoop([]() {
        CallStateListener listener;
        std::shared_ptr<CountingObserver> first = std::make_shared<CountingObserver>();
        std::shared_ptr<CountingObserver> second = std::make_shared<CountingObserver>();
        EXPECT_TRUE(listener.AddOneObserver(first));
        EXPECT_TRUE(listener.AddOneObserver(first));
        EXPECT_TRUE(listener.AddOneObserver(second));
        EXPECT_FALSE(listener.AddOneObserver(nullptr));
        sptr<CallBase> call = MakeCall(CallType::TYPE_CS, 1, false, "10086");
        listener.CallStateUpdated(call, TelCallState::CALL_STATUS_IDLE, TelCallState::CALL_STATUS_DIALING);
        listener.CallStateUpdated(call, TelCallState::CALL_STATUS_DIALING, TelCallState::CALL_STATUS_ALERTING);
        EXPECT_EQ(first->updates, 2);
        EXPECT_EQ(second->updates, 2);
        listener.RemoveOneObserver(second);
        listener.CallStateUpdated(call, TelCallState::CALL_STATUS_ALERTING, TelCallState::CALL_STATUS_ACTIVE);
        EXPECT_EQ(first->updates, 3);
        EXPECT_EQ(second->updates, 2);
    });
    EXPECT_EQ(TelMiniConfinementViolations(), violationsBefore);
    CallObjectManager::HasCallExist();
    EXPECT_EQ(TelMiniConfinementViolations(), violationsBefore + 1);
    Stop();
}
