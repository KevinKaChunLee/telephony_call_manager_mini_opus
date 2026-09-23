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

// Uplink, policies and downlink through the client, the service and the stack double (tasks 4.5-4.8).
// Each case names the specs/mini-voice-call-control, mini-call-state-reporting or mini-call-runtime
// scenario it covers; "高危序列" marks the sequences task 4.8 lists.

namespace FlowCases {
using namespace OHOS;
using namespace OHOS::Telephony;
using CallFixture::g_recorder;
using CallFixture::g_stack;

const std::vector<TelCallState> DIAL_TO_END = { TelCallState::CALL_STATUS_DIALING,
    TelCallState::CALL_STATUS_ALERTING, TelCallState::CALL_STATUS_ACTIVE, TelCallState::CALL_STATUS_DISCONNECTED };

// Brings `count` calls in as the network would: the first rings and is taken active, the rest wait.
void BringInCalls(int32_t count)
{
    for (int32_t index = 1; index <= count; index++) {
        std::string number = "1000" + std::to_string(index);
        g_stack->Set(index, (index == 1) ? TelCallState::CALL_STATUS_INCOMING : TelCallState::CALL_STATUS_WAITING,
            number.c_str());
        if (index == 1) {
            g_stack->Set(index, TelCallState::CALL_STATUS_ACTIVE, number.c_str());
        }
    }
    CallFixture::Settle();
}

bool IsIdle()
{
    bool allowed = false;
    CallFixture::Client()->IsNewCallAllowed(allowed);
    return !CallFixture::Client()->HasCall() && CallFixture::CallCount() == 0 && allowed &&
        CallFixture::Client()->GetCallState() == static_cast<int32_t>(CallStateToApp::CALL_STATE_IDLE);
}
} // namespace FlowCases

// 高危序列：正常拨号；最后一路结束后状态归零 (mini-call-state-reporting 状态顺序)
MINI_TEST(Uplink, DialAlertActiveDisconnectInOrder)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    EXPECT_TRUE(g_stack->NumberOf(0) == "10086");
    int32_t callId = CallIdOf(1);
    EXPECT_TRUE(callId > 0);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    EXPECT_TRUE(Client()->HasCall());
    g_stack->Drop(1);
    Settle();
    EXPECT_TRUE(Contains(g_recorder.StatesOf(callId), DIAL_TO_END));
    EXPECT_EQ(g_recorder.LastStateOf(callId), TelCallState::CALL_STATUS_DISCONNECTED);
    EXPECT_TRUE(IsIdle());
    Stop();
}

// 对象表已满时来电: the fifth normal call is refused through the stack and never reported.
MINI_TEST(Uplink, IncomingWhenTableFullIsRejectedByTheStack)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    BringInCalls(4);
    EXPECT_EQ(CallCount(), MINI_MAX_CALL_OBJECTS - MINI_ECC_RESERVED_CALL_OBJECTS);
    size_t reportsBefore = g_recorder.states.size();
    g_stack->Set(5, TelCallState::CALL_STATUS_WAITING, "10005");
    Settle();
    EXPECT_EQ(CallCount(), MINI_MAX_CALL_OBJECTS - MINI_ECC_RESERVED_CALL_OBJECTS);
    EXPECT_TRUE(CallByIndex(5) == nullptr);
    EXPECT_EQ(g_stack->CountOf("HangUp"), 1);
    EXPECT_TRUE(g_stack->LastNumber() == "10005");
    EXPECT_EQ(g_recorder.states.size(), reportsBefore);
    g_stack->calls.clear();
    g_stack->Report();
    Settle();
    EXPECT_TRUE(IsIdle());
    Stop();
}

// 延迟任务执行前通话已移除 (the pending-hangup protect task of a call that is gone by then)
MINI_TEST(Uplink, DelayedTaskFindsItsCallGone)
{
    using namespace OHOS;
    using namespace CallFixture;
    using namespace FlowCases;
    CallControlManager::SetPendingHangupDelayForTest(50);
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_stack->dialRet = TELEPHONY_SUCCESS;
    ASSERT_TRUE(Dial(u"10086") == TELEPHONY_SUCCESS);
    int32_t callId = CallIdOf(0);
    ASSERT_TRUE(callId > 0);
    EXPECT_EQ(Client()->HangUpCall(callId), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("HangUp"), 0);
    OnLoop([callId]() { CallObjectManager::DeleteOneCallObject(callId); });
    OHOS::Telephony::TelOsTaskDelayMs(200);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_EQ(g_stack->CountOf("HangUp"), 0);
    bool pending = true;
    OnLoop([&pending, callId]() {
        pending = DelayedSingleton<CallRequestEventHandlerHelper>::GetInstance()->HasPendingHangup(callId);
    });
    EXPECT_FALSE(pending);
    Stop();
    CallControlManager::SetPendingHangupDelayForTest(0);
}

// 两次通话之间无状态泄漏
MINI_TEST(Uplink, NoStateLeaksBetweenCalls)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    int32_t firstId = CallIdOf(1);
    g_stack->Drop(1);
    Settle();
    EXPECT_TRUE(IsIdle());
    ASSERT_TRUE(EstablishOutgoing(u"10010", 1) == TELEPHONY_SUCCESS);
    int32_t secondId = CallIdOf(1);
    EXPECT_TRUE(secondId > firstId);
    sptr<CallBase> second = CallByIndex(1);
    ASSERT_TRUE(second != nullptr);
    EXPECT_EQ(second->GetPolicyFlag(), 0U);
    EXPECT_EQ(second->GetTelConferenceState(), TelConferenceState::TEL_CONFERENCE_IDLE);
    EXPECT_TRUE(second->GetAccountNumber() == "10010");
    g_stack->Drop(1);
    Settle();
    EXPECT_TRUE(IsIdle());
    EXPECT_FALSE(DelayedSingleton<AudioControlManager>::GetInstance()->IsCallAudioActive());
    Stop();
}

// 空号码 + 错误码与标准侧一致
MINI_TEST(Policy, EmptyNumberFailsBeforeAnySideEffect)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    EXPECT_EQ(Dial(u""), CALL_ERR_PHONE_NUMBER_EMPTY);
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_EQ(g_stack->commands.size(), 0U);
    Stop();
}

// 策略拒绝: two carrier calls already, a third normal dial is refused and nothing changes.
MINI_TEST(Policy, CallLimitRejectsWithoutSideEffects)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    BringInCalls(2);
    g_stack->Set(1, TelCallState::CALL_STATUS_HOLDING, "10001");
    g_stack->Set(2, TelCallState::CALL_STATUS_ACTIVE, "10002");
    Settle();
    size_t reportsBefore = g_recorder.states.size();
    size_t commandsBefore = g_stack->commands.size();
    EXPECT_EQ(Dial(u"10086"), CALL_ERR_CALL_COUNTS_EXCEED_LIMIT);
    Settle();
    EXPECT_EQ(CallCount(), 2U);
    EXPECT_EQ(g_stack->commands.size(), commandsBefore);
    EXPECT_EQ(g_recorder.states.size(), reportsBefore);
    Stop();
}

// 裁剪能力被省略时拨打紧急号码: no SIM, no service and nothing else on the Mini system blocks it.
MINI_TEST(Policy, EmergencyDialIgnoresTheOmittedChecks)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    g_core->hasSim = false;
    g_core->inService = false;
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_NO_SIM_CARD);
    EXPECT_EQ(Dial(u"112"), TELEPHONY_SUCCESS);
    ASSERT_EQ(g_stack->CountOf("Dial"), 1);
    EXPECT_TRUE(g_stack->LastNumber() == "112");
    sptr<CallBase> call = CallByIndex(0);
    ASSERT_TRUE(call != nullptr);
    EXPECT_TRUE(call->GetEmergencyState());
    CallAttributeInfo info;
    OnLoop([&call, &info]() { call->GetCallAttributeInfo(info); });
    EXPECT_TRUE(info.isEcc);
    EXPECT_TRUE(info.isEccContact);
    Stop();
}

// 通话数已达上限时拨打紧急号码 + 普通槽位已满时拨打紧急号码 / 普通槽位已满 (高危序列：紧急呼叫（含普通槽位满）)
MINI_TEST(Policy, EmergencyDialUsesTheReservedSlot)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    FlowCases::BringInCalls(4);
    ASSERT_EQ(CallCount(), 4U);
    EXPECT_EQ(Dial(u"10086"), CALL_ERR_CALL_COUNTS_EXCEED_LIMIT);
    EXPECT_EQ(g_stack->CountOf("Dial"), 0);
    EXPECT_EQ(Dial(u"112"), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    EXPECT_EQ(g_stack->CountOf("HangUp") + g_stack->CountOf("Reject"), 4);
    EXPECT_EQ(CallCount(), 5U);
    Stop();
}

// 连续两次拨号
MINI_TEST(Downlink, SecondDialWhileTheFirstIsPending)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_SUCCESS);
    DialParaInfo before;
    OnLoop([&before]() { DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(before); });
    EXPECT_EQ(Dial(u"10010"), CALL_ERR_CALL_COUNTS_EXCEED_LIMIT);
    DialParaInfo after;
    OnLoop([&after]() { DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(after); });
    EXPECT_TRUE(after.number == "10086");
    EXPECT_TRUE(after.number == before.number);
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    EXPECT_EQ(CallCount(), 1U);
    Stop();
}

// 协议栈拒绝拨号 + 失败处理不自等待 (高危序列：拨号失败后再次拨号)
MINI_TEST(Downlink, StackRejectsDialAndLeavesNothingBehind)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_stack->dialRet = CALL_ERR_DIAL_FAILED;
    uint32_t begin = OHOS::Telephony::TelOsGetTickCount();
    int32_t ret = Dial(u"10086");
    uint32_t elapsed = OHOS::Telephony::TelOsGetTickCount() - begin;
    EXPECT_TRUE(ret != TELEPHONY_SUCCESS);
    EXPECT_EQ(ret, CALL_ERR_DIAL_FAILED);
    EXPECT_TRUE(elapsed < OHOS::Telephony::TelOsMsToTicks(500));
    Settle();
    int32_t failedId = g_recorder.LastCallId();
    EXPECT_TRUE(Contains(g_recorder.StatesOf(failedId),
        { TelCallState::CALL_STATUS_DIALING, TelCallState::CALL_STATUS_DISCONNECTED }));
    EXPECT_TRUE(IsIdle());
    g_stack->dialRet = TELEPHONY_SUCCESS;
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    Stop();
}

// 挂断不存在的通话
MINI_TEST(Downlink, HangUpOfUnknownCallSendsNothing)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    EXPECT_EQ(Client()->HangUpCall(4242), TELEPHONY_ERR_ARGUMENT_INVALID);
    Settle();
    EXPECT_EQ(g_stack->commands.size(), 0U);
    Stop();
}

// 接听来电（先 ANSWERED 再 ACTIVE） (高危序列：来电 → 接听)
MINI_TEST(Downlink, AnswerReportsAnsweredThenActive)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    int32_t callId = CallIdOf(1);
    ASSERT_TRUE(callId > 0);
    bool ringing = false;
    EXPECT_EQ(Client()->IsRinging(ringing), TELEPHONY_SUCCESS);
    EXPECT_TRUE(ringing);
    EXPECT_EQ(Client()->AnswerCall(callId, 0), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Answer"), 1);
    g_stack->Set(1, TelCallState::CALL_STATUS_ACTIVE, "10086");
    Settle();
    EXPECT_TRUE(Contains(g_recorder.StatesOf(callId), { TelCallState::CALL_STATUS_INCOMING,
        TelCallState::CALL_STATUS_ANSWERED, TelCallState::CALL_STATUS_ACTIVE }));
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    Stop();
}

// 高危序列：来电 → 拒接
MINI_TEST(Downlink, RejectIncomingCall)
{
    using namespace CallFixture;
    using namespace FlowCases;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    int32_t callId = CallIdOf(1);
    EXPECT_EQ(Client()->RejectCall(callId, false, u""), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("HangUp"), 1);
    g_stack->Drop(1);
    Settle();
    EXPECT_EQ(g_recorder.LastStateOf(callId), TelCallState::CALL_STATUS_DISCONNECTED);
    EXPECT_TRUE(IsIdle());
    Stop();
}

// 保持后切换 (高危序列：保持 → 切换)
MINI_TEST(Downlink, HoldThenSwitchSwapsTheCalls)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    ASSERT_TRUE(EstablishOutgoing(u"10001", 1) == TELEPHONY_SUCCESS);
    int32_t firstId = CallIdOf(1);
    EXPECT_EQ(Client()->HoldCall(firstId), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Hold"), 1);
    g_stack->Set(1, TelCallState::CALL_STATUS_HOLDING, "10001");
    Settle();
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_HOLDING);
    ASSERT_TRUE(EstablishOutgoing(u"10002", 2) == TELEPHONY_SUCCESS);
    int32_t secondId = CallIdOf(2);
    EXPECT_EQ(StateOf(2), TelCallState::CALL_STATUS_ACTIVE);
    EXPECT_EQ(Client()->SwitchCall(firstId), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Switch"), 1);
    g_stack->Set(1, TelCallState::CALL_STATUS_ACTIVE, "10001");
    g_stack->Set(2, TelCallState::CALL_STATUS_HOLDING, "10002");
    Settle();
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    EXPECT_EQ(StateOf(2), TelCallState::CALL_STATUS_HOLDING);
    EXPECT_EQ(g_recorder.LastStateOf(firstId), TelCallState::CALL_STATUS_ACTIVE);
    EXPECT_EQ(g_recorder.LastStateOf(secondId), TelCallState::CALL_STATUS_HOLDING);
    Stop();
}

// 请求卫星通话
MINI_TEST(Downlink, SatelliteDialIsRefused)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    AppExecFwk::PacMap extras = VoiceExtras();
    extras.PutIntValue("callType", static_cast<int32_t>(CallType::TYPE_SATELLITE));
    EXPECT_EQ(Client()->DialCall(u"10086", extras), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_EQ(g_stack->commands.size(), 0U);
    Stop();
}

// 协议栈适配未注册 (HoldCall is accepted, the command fails at the adapter, the state stays)
MINI_TEST(Downlink, HoldWithoutStackKeepsTheCallActive)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    int32_t callId = CallIdOf(1);
    CellularCallLiteRegistry::Register(nullptr);
    EXPECT_EQ(Client()->HoldCall(callId), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Hold"), 0);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    int32_t ret = TELEPHONY_SUCCESS;
    OnLoop([&ret]() {
        ret = DelayedSingleton<CellularCallConnection>::GetInstance()->HoldCall(CellularCallInfo());
    });
    EXPECT_EQ(ret, TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    Stop();
}

// 队列满时 HangUpCall 返回错误且状态不变 (队列满, mini-call-runtime)
MINI_TEST(Downlink, HangUpWithFullQueueFailsAndChangesNothing)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    int32_t callId = CallIdOf(1);
    int32_t ret = TELEPHONY_SUCCESS;
    OnLoop([&ret, callId]() {
        while (TelMiniMainLoop().PostAsyncTask([]() {}) == TELEPHONY_SUCCESS) {
        }
        ret = DelayedSingleton<CallManagerService>::GetInstance()->HangUpCall(callId);
    });
    EXPECT_EQ(ret, TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    Settle();
    EXPECT_EQ(g_stack->CountOf("HangUp"), 0);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    Stop();
}

// 高危序列：拨号时队列中已有他路上报 (an incoming report is queued while the dial runs)
MINI_TEST(Sequence, DialWithAnotherReportQueued)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    int32_t dialRet = TELEPHONY_ERR_FAIL;
    OnLoop([&dialRet]() {
        g_stack->Set(2, TelCallState::CALL_STATUS_INCOMING, "10002");
        AppExecFwk::PacMap extras = VoiceExtras();
        dialRet = DelayedSingleton<CallManagerService>::GetInstance()->DialCall(u"10086", extras);
    });
    EXPECT_EQ(dialRet, TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    EXPECT_TRUE(CallByIndex(2) != nullptr);
    g_stack->Set(1, TelCallState::CALL_STATUS_DIALING, "10086");
    Settle();
    EXPECT_TRUE(CallByIndex(1) != nullptr);
    EXPECT_TRUE(CallByIndex(0) == nullptr);
    EXPECT_EQ(CallCount(), 2U);
    Stop();
}

// 高危序列：通话中第二路来电 (WAITING plays the waiting tone and is answered like an incoming call)
MINI_TEST(Sequence, SecondIncomingCallWhileActive)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    ASSERT_TRUE(EstablishOutgoing(u"10001", 1) == TELEPHONY_SUCCESS);
    g_stack->Set(2, TelCallState::CALL_STATUS_WAITING, "10002");
    Settle();
    int32_t waitingId = CallIdOf(2);
    ASSERT_TRUE(waitingId > 0);
    EXPECT_EQ(StateOf(2), TelCallState::CALL_STATUS_WAITING);
    EXPECT_TRUE(DelayedSingleton<AudioControlManager>::GetInstance()->IsWaitingTonePlaying());
    EXPECT_EQ(Client()->AnswerCall(waitingId, 0), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_EQ(g_stack->CountOf("Answer"), 1);
    g_stack->Set(1, TelCallState::CALL_STATUS_HOLDING, "10001");
    g_stack->Set(2, TelCallState::CALL_STATUS_ACTIVE, "10002");
    Settle();
    EXPECT_FALSE(DelayedSingleton<AudioControlManager>::GetInstance()->IsWaitingTonePlaying());
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_HOLDING);
    EXPECT_EQ(StateOf(2), TelCallState::CALL_STATUS_ACTIVE);
    g_stack->calls.clear();
    g_stack->Report();
    Settle();
    EXPECT_TRUE(FlowCases::IsIdle());
    Stop();
}
