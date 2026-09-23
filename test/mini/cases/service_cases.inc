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

// Report proxy, service entries and lifecycle, client proxy, end-to-end runs and audio (tasks 5.1-5.3,
// 5.5, 6.1). Scenario names come from specs/mini-call-client-api, mini-call-runtime,
// mini-call-state-reporting and mini-platform-adaptation.

#include "audio_control_manager.h"
#include "call_ability_report_proxy.h"

namespace ServiceCases {
using namespace OHOS;
using namespace OHOS::Telephony;

std::atomic<int> g_warnings { 0 };

void CountWarnings(TelMiniLogLevel level, const char *message)
{
    if (std::strstr(message, "no call audio implementation") != nullptr) {
        g_warnings.fetch_add(1);
    }
}

struct DialerTask {
    TelOsHandle done = nullptr;
    std::atomic<int32_t> result { -1 };
};

DialerTask g_dialer;

void DialFromOtherTask(void *)
{
    g_dialer.result.store(CallFixture::Dial(u"10086"));
    TelOsSemPost(g_dialer.done);
}

bool SameObjects(const TelOsObjectCount &a, const TelOsObjectCount &b)
{
    return a.mutexCount == b.mutexCount && a.semaphoreCount == b.semaphoreCount && a.timerCount == b.timerCount &&
        a.taskCount == b.taskCount;
}
} // namespace ServiceCases

// 来电上报 + 同一通话的回调顺序 (mini-call-state-reporting)
MINI_TEST(ReportProxy, IncomingCallIsReportedInOrder)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    int32_t callId = CallIdOf(1);
    ASSERT_TRUE(callId > 0);
    ASSERT_TRUE(!g_recorder.states.empty());
    EXPECT_EQ(g_recorder.FirstStateOf(callId), TelCallState::CALL_STATUS_INCOMING);
    g_stack->Set(1, TelCallState::CALL_STATUS_ACTIVE, "10086");
    g_stack->Drop(1);
    Settle();
    std::vector<TelCallState> states = g_recorder.StatesOf(callId);
    EXPECT_TRUE(Contains(states, { TelCallState::CALL_STATUS_INCOMING, TelCallState::CALL_STATUS_ACTIVE,
        TelCallState::CALL_STATUS_DISCONNECTED }));
    EXPECT_EQ(g_recorder.LastStateOf(callId), TelCallState::CALL_STATUS_DISCONNECTED);
    Stop();
}

// 回调返回错误: the state machine carries on.
MINI_TEST(ReportProxy, FailingCallbackDoesNotStopTheStateMachine)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    g_recorder.detailsResult = TELEPHONY_ERR_FAIL;
    ASSERT_TRUE(EstablishOutgoing(u"10086", 1) == TELEPHONY_SUCCESS);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_ACTIVE);
    g_stack->Drop(1);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_TRUE(g_recorder.states.size() >= FlowCases::DIAL_TO_END.size());
    Stop();
}

// 调用方注入非白名单字段 + 调用方伪造 bundleName
MINI_TEST(Service, DialExtrasAreRebuiltFromTheWhitelist)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    AppExecFwk::PacMap extras = VoiceExtras();
    extras.PutStringValue("token", "forged-token");
    extras.PutBooleanValue("isCustomAccessibility", true);
    extras.PutStringValue("bundleName", "com.attacker");
    ASSERT_EQ(Client()->DialCall(u"10086", extras), TELEPHONY_SUCCESS);
    DialParaInfo info;
    OnLoop([&info]() { DelayedSingleton<CallControlManager>::GetInstance()->GetDialParaInfo(info); });
    EXPECT_TRUE(info.token.empty());
    EXPECT_FALSE(info.isCustomAccessibility);
    EXPECT_TRUE(info.bundleName == TRUSTED_BUNDLE);
    Stop();
}

// 号码超长
MINI_TEST(Service, OverlongNumberIsRefused)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    std::u16string number(256, u'1');
    EXPECT_EQ(Dial(number), CALL_ERR_NUMBER_OUT_OF_RANGE);
    EXPECT_EQ(CallCount(), 0U);
    Stop();
}

// 拒接并发送短信 + 以视频方式接听
MINI_TEST(Service, UnsupportedAnswerAndRejectOptionsFailFirst)
{
    using namespace CallFixture;
    CallPermissionAdapter::OverrideForTest(nullptr, false, nullptr);
    ASSERT_TRUE(Start());
    CallPermissionAdapter::OverrideForTest("ohos.permission.ANSWER_CALL;ohos.permission.SEND_MESSAGES;"
        "ohos.permission.SET_TELEPHONY_STATE;", true, TRUSTED_BUNDLE);
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    int32_t callId = CallIdOf(1);
    EXPECT_EQ(Client()->RejectCall(callId, true, u"busy"), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(Client()->AnswerCall(callId, static_cast<int32_t>(VideoStateType::TYPE_VIDEO)),
        CALL_ERR_VIDEO_NOT_SUPPORTED);
    Settle();
    EXPECT_EQ(g_stack->commands.size(), 0U);
    EXPECT_EQ(StateOf(1), TelCallState::CALL_STATUS_INCOMING);
    Stop();
}

// 未配置信任模型 + 已配置信任模型
MINI_TEST(Service, TrustModelGatesDialing)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    CallPermissionAdapter::OverrideForTest(nullptr, false, nullptr);
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API);
    CallPermissionAdapter::OverrideForTest("", true, "");
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_PERMISSION_ERR);
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_EQ(g_stack->commands.size(), 0U);
    CallPermissionAdapter::OverrideForTest("ohos.permission.PLACE_CALL;", true, TRUSTED_BUNDLE);
    g_core->hasSim = false;
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_NO_SIM_CARD);
    Stop();
}

// 启动早期调用 + 重复初始化 + 中途失败 (mini-call-runtime)
MINI_TEST(Service, LifecycleBeforeAndDuringInit)
{
    using namespace OHOS;
    using namespace CallFixture;
    UseTrustedCaller();
    Client()->Init(SA_ID);
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_UNINIT);
    Client()->UnInit();
    for (int32_t step = 1; step <= 3; step++) {
        TelOsObjectCount before = TelOsGetObjectCount();
        CallManagerService::FailInitStepForTest(step);
        EXPECT_EQ(CallManagerServiceMiniStart(), TELEPHONY_ERR_FAIL);
        EXPECT_FALSE(DelayedSingleton<CallManagerService>::GetInstance()->IsReady());
        EXPECT_FALSE(TelMiniMainLoop().IsRunning());
        Client()->Init(SA_ID);
        EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_UNINIT);
        Client()->UnInit();
        OHOS::Telephony::TelOsTaskDelayMs(20);
        EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), before));
    }
    CallManagerService::FailInitStepForTest(0);
    ASSERT_TRUE(Start());
    TelOsObjectCount running = TelOsGetObjectCount();
    int32_t again = TELEPHONY_ERR_FAIL;
    OnLoop([&again]() { again = DelayedSingleton<CallManagerService>::GetInstance()->Init(); });
    EXPECT_EQ(again, TELEPHONY_SUCCESS);
    EXPECT_EQ(CallManagerServiceMiniStart(), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->registrations.load(), 1);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), running));
    Stop();
}

// 退出期间有请求到达 + 重复退出 (mini-call-runtime)
MINI_TEST(Service, ExitRejectsLateRequestsAndIsRepeatable)
{
    using namespace CallFixture;
    TelOsObjectCount before = TelOsGetObjectCount();
    ASSERT_TRUE(Start());
    int32_t duringExit = TELEPHONY_ERR_FAIL;
    OnLoop([&duringExit]() {
        DelayedSingleton<CallManagerService>::GetInstance()->UnInit();
        duringExit = CallFixture::Dial(u"10086");
    });
    EXPECT_EQ(duringExit, TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(g_stack->commands.size(), 0U);
    Stop();
    CallManagerServiceMiniStop();
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_UNINIT);
    OHOS::Telephony::TelOsTaskDelayMs(20);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), before));
}

// 正常拨号返回 + 错误码与标准侧一致 + 重复注册 + 注销后无回调 (mini-call-client-api)
MINI_TEST(Proxy, RegistrationAndDialThroughTheClient)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    EXPECT_EQ(Client()->RegisterCallBack(nullptr), TELEPHONY_ERR_LOCAL_PTR_NULL);
    ASSERT_TRUE(RegisterRecorder());
    EXPECT_EQ(Client()->RegisterCallBack(std::make_unique<RecordingCallback>()), TELEPHONY_ERR_REGISTER_CALLBACK_FAIL);
    EXPECT_EQ(Dial(u""), CALL_ERR_PHONE_NUMBER_EMPTY);
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_SUCCESS);
    Settle();
    EXPECT_FALSE(g_recorder.states.empty());
    EXPECT_EQ(g_recorder.LastState(), TelCallState::CALL_STATUS_DIALING);
    EXPECT_EQ(Client()->UnRegisterCallBack(), TELEPHONY_SUCCESS);
    EXPECT_EQ(Client()->UnRegisterCallBack(), TELEPHONY_ERR_REGISTER_CALLBACK_FAIL);
    size_t seen = g_recorder.states.size();
    g_stack->Set(1, TelCallState::CALL_STATUS_DIALING, "10086");
    g_stack->Set(1, TelCallState::CALL_STATUS_ALERTING, "10086");
    Settle();
    EXPECT_EQ(g_recorder.states.size(), seen);
    EXPECT_TRUE(RegisterRecorder());
    Stop();
}

// 服务处理超时 (mini-call-client-api): the caller returns near the limit, the late call is harmless.
MINI_TEST(Proxy, BlockedLoopTimesTheCallerOut)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    TelOsHandle gate = nullptr;
    ASSERT_TRUE(TelOsSemCreate(0, 1, &gate) == TelOsResult::OK);
    EXPECT_EQ(TelMiniMainLoop().PostAsyncTask([gate]() { TelOsSemWait(gate, MINI_SYNC_WAIT_MS * 2); }),
        TELEPHONY_SUCCESS);
    uint32_t begin = TelOsGetTickCount();
    EXPECT_EQ(Client()->HangUpCall(1), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    uint32_t elapsed = TelOsGetTickCount() - begin;
    EXPECT_TRUE(elapsed + 1 >= TelOsMsToTicks(MINI_SYNC_WAIT_MS));
    EXPECT_TRUE(elapsed <= TelOsMsToTicks(MINI_SYNC_WAIT_MS + 500));
    TelOsSemPost(gate);
    Settle();
    EXPECT_EQ(g_stack->commands.size(), 0U);
    TelOsSemDelete(gate);
    Stop();
}

// 在回调中接听 (mini-call-client-api) + 主循环内的同步调用 (mini-call-runtime)
MINI_TEST(EndToEnd, AnswerFromInsideTheCallback)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ASSERT_TRUE(RegisterRecorder());
    std::atomic<int32_t> answerRet { -1 };
    g_recorder.onDetails = [&answerRet](const CallAttributeInfo &info) {
        if (info.callState == TelCallState::CALL_STATUS_INCOMING) {
            answerRet.store(Client()->AnswerCall(info.callId, 0));
        }
    };
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    g_recorder.onDetails = nullptr;
    EXPECT_EQ(answerRet.load(), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("Answer"), 1);
    int32_t callId = CallIdOf(1);
    g_stack->Set(1, TelCallState::CALL_STATUS_ACTIVE, "10086");
    Settle();
    EXPECT_TRUE(Contains(g_recorder.StatesOf(callId), { TelCallState::CALL_STATUS_INCOMING,
        TelCallState::CALL_STATUS_ANSWERED, TelCallState::CALL_STATUS_ACTIVE }));
    Stop();
}

// 其他任务的同步调用 (mini-call-runtime)
MINI_TEST(EndToEnd, DialFromAnotherTaskWaitsForItsResult)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    ServiceCases::g_dialer.result.store(-1);
    ASSERT_TRUE(TelOsSemCreate(0, 1, &ServiceCases::g_dialer.done) == TelOsResult::OK);
    TelOsTaskParam param;
    param.name = "mini_test_dialer";
    param.entry = ServiceCases::DialFromOtherTask;
    param.stackSize = 32768;
    param.priority = 20;
    ASSERT_TRUE(TelOsTaskCreate(param) == TelOsResult::OK);
    EXPECT_TRUE(TelOsSemWait(ServiceCases::g_dialer.done, WAIT_MS) == TelOsResult::OK);
    EXPECT_EQ(ServiceCases::g_dialer.result.load(), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    TelOsSemDelete(ServiceCases::g_dialer.done);
    Stop();
}

// 多路通话不增加内核对象 (mini-call-runtime): 1, 3 and 5 calls, the fifth being an emergency call.
MINI_TEST(EndToEnd, KernelObjectsStayConstantAcrossCalls)
{
    using namespace CallFixture;
    TelOsObjectCount idle = TelOsGetObjectCount();
    ASSERT_TRUE(Start());
    TelOsObjectCount running = TelOsGetObjectCount();
    EXPECT_EQ(running.mutexCount - idle.mutexCount, 1U);
    EXPECT_EQ(running.semaphoreCount - idle.semaphoreCount, MINI_MAX_SYNC_WAITERS + 1);
    EXPECT_EQ(running.timerCount - idle.timerCount, 1U);
    EXPECT_EQ(running.taskCount - idle.taskCount, 1U);
    FlowCases::BringInCalls(1);
    EXPECT_EQ(CallCount(), 1U);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), running));
    g_stack->Set(2, TelCallState::CALL_STATUS_WAITING, "10002");
    g_stack->Set(3, TelCallState::CALL_STATUS_WAITING, "10003");
    Settle();
    EXPECT_EQ(CallCount(), 3U);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), running));
    g_stack->Set(4, TelCallState::CALL_STATUS_WAITING, "10004");
    Settle();
    EXPECT_EQ(Dial(u"112"), TELEPHONY_SUCCESS);
    EXPECT_EQ(CallCount(), 5U);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), running));
    Stop();
    OHOS::Telephony::TelOsTaskDelayMs(20);
    EXPECT_TRUE(ServiceCases::SameObjects(TelOsGetObjectCount(), idle));
}

// 无音频实现时来电 (mini-platform-adaptation)
MINI_TEST(Audio, IncomingWithoutAudioIsReportedAndLoggedOnce)
{
    using namespace CallFixture;
    ServiceCases::g_warnings.store(0);
    ASSERT_TRUE(Start(true, false));
    ASSERT_TRUE(RegisterRecorder());
    TelMiniLogSetSink(ServiceCases::CountWarnings);
    TelMiniLogSetLevel(TelMiniLogLevel::DEBUG);
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    int32_t ret = TELEPHONY_SUCCESS;
    OnLoop([&ret]() {
        sptr<CallBase> call = CallObjectManager::GetOneCallObjectByIndexAndSlotId(1, 0);
        DelayedSingleton<AudioControlManager>::GetInstance()->CallStateUpdated(
            call, TelCallState::CALL_STATUS_IDLE, TelCallState::CALL_STATUS_INCOMING);
        ret = DelayedSingleton<AudioControlManager>::GetInstance()->SetMute(true);
    });
    g_stack->Set(2, TelCallState::CALL_STATUS_WAITING, "10002");
    Settle();
    TelMiniLogSetLevel(TelMiniLogLevel::FATAL);
    TelMiniLogSetSink(nullptr);
    EXPECT_EQ(ret, CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(g_recorder.FirstStateOf(CallIdOf(1)), TelCallState::CALL_STATUS_INCOMING);
    EXPECT_EQ(ServiceCases::g_warnings.load(), 1);
    EXPECT_FALSE(DelayedSingleton<AudioControlManager>::GetInstance()->IsRingtonePlaying());
    Stop();
}

// 通话接通 (mini-platform-adaptation) + 最后一路结束后释放通话音频通路
MINI_TEST(Audio, CallAudioFollowsTheCall)
{
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    sptr<FakeAudio> audio = g_audio;
    g_stack->Set(1, TelCallState::CALL_STATUS_INCOMING, "10086");
    Settle();
    EXPECT_TRUE(DelayedSingleton<AudioControlManager>::GetInstance()->IsRingtonePlaying());
    EXPECT_EQ(Client()->AnswerCall(CallIdOf(1), 0), TELEPHONY_SUCCESS);
    g_stack->Set(1, TelCallState::CALL_STATUS_ACTIVE, "10086");
    Settle();
    EXPECT_TRUE(DelayedSingleton<AudioControlManager>::GetInstance()->IsCallAudioActive());
    EXPECT_FALSE(DelayedSingleton<AudioControlManager>::GetInstance()->IsRingtonePlaying());
    g_stack->Drop(1);
    Settle();
    EXPECT_FALSE(DelayedSingleton<AudioControlManager>::GetInstance()->IsCallAudioActive());
    std::vector<std::string> expected = { "ring-start", "ring-stop", "audio-on", "audio-off" };
    EXPECT_TRUE(audio->log == expected);
    Stop();
}
