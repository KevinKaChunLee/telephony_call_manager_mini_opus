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

// Adapter registries, the uplink guard and the stack connection (tasks 3.1-3.3, 4.4). Scenario names
// in the comments are those of specs/mini-call-runtime, mini-call-state-reporting and
// mini-platform-adaptation.

#include "call_status_callback_guard.h"
#include "cellular_call_connection.h"
#include "core_service_client.h"

namespace AdapterCases {
using namespace OHOS;
using namespace OHOS::Telephony;

std::atomic<int> g_readyCount { 0 };

void CountReady()
{
    g_readyCount.fetch_add(1);
}

std::string g_capturedLog;

void CaptureLog(TelMiniLogLevel level, const char *message)
{
    g_capturedLog += message;
    g_capturedLog += "\n";
}
} // namespace AdapterCases

MINI_TEST(Registry, ReadyListenerHearsEachRegistration)
{
    using namespace OHOS::Telephony;
    using namespace AdapterCases;
    CellularCallLiteRegistry::Register(nullptr);
    EXPECT_TRUE(CellularCallLiteRegistry::Get() == nullptr);
    g_readyCount.store(0);
    ASSERT_TRUE(CellularCallLiteRegistry::AddReadyListener(CountReady));
    EXPECT_EQ(g_readyCount.load(), 0);
    sptr<CallFixture::FakeStack> stack = new CallFixture::FakeStack();
    CellularCallLiteRegistry::Register(stack);
    EXPECT_EQ(g_readyCount.load(), 1);
    EXPECT_TRUE(CellularCallLiteRegistry::Get().GetRefPtr() == stack.GetRefPtr());
    CellularCallLiteRegistry::Register(nullptr);
    EXPECT_EQ(g_readyCount.load(), 1);
    EXPECT_TRUE(CellularCallLiteRegistry::Get() == nullptr);
    CellularCallLiteRegistry::RemoveReadyListener(CountReady);
    CellularCallLiteRegistry::Register(stack);
    EXPECT_EQ(g_readyCount.load(), 1);
    CellularCallLiteRegistry::Register(nullptr);
}

MINI_TEST(CoreServiceClient, ForwardsOnlyWhenRegistered)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    CoreServiceLiteRegistry::Register(nullptr);
    bool hasSim = true;
    bool inService = true;
    EXPECT_EQ(DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(0, hasSim),
        TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    EXPECT_EQ(DelayedRefSingleton<CoreServiceClient>::GetInstance().IsNetworkInService(0, inService),
        TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    EXPECT_TRUE(hasSim && inService);
    sptr<CallFixture::FakeCoreService> core = new CallFixture::FakeCoreService();
    core->hasSim = false;
    CoreServiceLiteRegistry::Register(core);
    EXPECT_EQ(DelayedRefSingleton<CoreServiceClient>::GetInstance().HasSimCard(0, hasSim), TELEPHONY_SUCCESS);
    EXPECT_FALSE(hasSim);
    EXPECT_EQ(DelayedRefSingleton<CoreServiceClient>::GetInstance().IsNetworkInService(0, inService),
        TELEPHONY_SUCCESS);
    EXPECT_TRUE(inService);
    CoreServiceLiteRegistry::Register(nullptr);
}

// 批量上报条目超限
MINI_TEST(Guard, OversizedBatchIsRejected)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    CallsReportInfo report;
    report.slotId = 0;
    for (int32_t i = 1; i <= static_cast<int32_t>(MINI_MAX_REPORTED_CALLS) + 1; i++) {
        CallReportInfo info;
        info.index = i;
        info.state = TelCallState::CALL_STATUS_INCOMING;
        info.callType = CallType::TYPE_CS;
        (void)std::strncpy(info.accountNum, "10086", kMaxNumberLen);
        report.callVec.push_back(info);
    }
    EXPECT_EQ(g_stack->callback->UpdateCallsReportInfo(report), TELEPHONY_ERR_ARGUMENT_INVALID);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    report.callVec.pop_back();
    report.callVec.back().accountId = 1;
    EXPECT_EQ(g_stack->callback->UpdateCallsReportInfo(report), TELEPHONY_ERR_ARGUMENT_INVALID);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    Stop();
}

// 号码未以 NUL 结尾
MINI_TEST(Guard, UnterminatedNumberIsRejectedWithoutLoggingIt)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    CallReportInfo info;
    info.index = 1;
    info.state = TelCallState::CALL_STATUS_INCOMING;
    info.callType = CallType::TYPE_CS;
    (void)std::memset(info.accountNum, '7', sizeof(info.accountNum));
    AdapterCases::g_capturedLog.clear();
    TelMiniLogSetSink(AdapterCases::CaptureLog);
    TelMiniLogSetLevel(TelMiniLogLevel::DEBUG);
    EXPECT_EQ(g_stack->callback->UpdateCallReportInfo(info), TELEPHONY_ERR_ARGUMENT_INVALID);
    CallsReportInfo report;
    report.slotId = 0;
    report.callVec.push_back(info);
    EXPECT_EQ(g_stack->callback->UpdateCallsReportInfo(report), TELEPHONY_ERR_ARGUMENT_INVALID);
    TelMiniLogSetLevel(TelMiniLogLevel::FATAL);
    TelMiniLogSetSink(nullptr);
    EXPECT_FALSE(AdapterCases::g_capturedLog.empty());
    EXPECT_TRUE(AdapterCases::g_capturedLog.find("7777777") == std::string::npos);
    info.state = TelCallState::CALL_STATUS_IDLE;
    (void)std::strncpy(info.accountNum, "10086", kMaxNumberLen);
    info.accountNum[kMaxNumberLen] = '\0';
    EXPECT_EQ(g_stack->callback->UpdateCallReportInfo(info), TELEPHONY_ERR_ARGUMENT_INVALID);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_FALSE(Client()->HasCall());
    Stop();
}

MINI_TEST(Guard, OutOfScopeReportsFailExplicitly)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    sptr<CallStatusCallbackGuard> guard = new CallStatusCallbackGuard();
    EXPECT_EQ(guard->StartDtmfResult(0), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(guard->SendMmiCodeResult(MmiCodeInfo()), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(guard->UpdateVoipEventInfo(VoipCallEventInfo()), CALL_ERR_FUNCTION_NOT_SUPPORTED);
}

// 协议栈晚于通话管理就绪 + 协议栈已就绪时重复触发注册
MINI_TEST(Connection, LateStackGetsTheUplinkExactlyOnce)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start(false));
    RegisterRecorder();
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    Settle();
    EXPECT_EQ(CallCount(), 0U);
    EXPECT_FALSE(Client()->HasCall());
    CellularCallLiteRegistry::Register(g_stack);
    Settle();
    EXPECT_EQ(g_stack->registrations.load(), 1);
    ASSERT_TRUE(g_stack->callback != nullptr);
    sptr<ICallStatusCallback> first = g_stack->callback;
    CellularCallLiteRegistry::Register(g_stack);
    Settle();
    EXPECT_EQ(g_stack->registrations.load(), 1);
    EXPECT_TRUE(g_stack->callback == first);
    EXPECT_EQ(Dial(u"10086"), TELEPHONY_SUCCESS);
    EXPECT_EQ(g_stack->CountOf("Dial"), 1);
    Stop();
    EXPECT_EQ(first->GetSptrRefCount(), 1);
}

// 底层返回错误 + 实现缺失 (mini-platform-adaptation)
MINI_TEST(Connection, ForwardsStackErrorsAndFailsWithoutStack)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    std::shared_ptr<CellularCallConnection> connection = DelayedSingleton<CellularCallConnection>::GetInstance();
    CellularCallInfo info;
    int32_t ret = TELEPHONY_ERR_FAIL;
    g_stack->hangUpRet = CALL_ERR_HANGUP_FAILED + 7;
    OnLoop([&]() { ret = connection->HangUp(info, CallSupplementType::TYPE_DEFAULT); });
    EXPECT_EQ(ret, CALL_ERR_HANGUP_FAILED + 7);
    OnLoop([&]() { ret = connection->StartDtmf('1', info); });
    EXPECT_EQ(ret, CALL_ERR_FUNCTION_NOT_SUPPORTED);
    CellularCallLiteRegistry::Register(nullptr);
    OnLoop([&]() { ret = connection->HoldCall(info); });
    EXPECT_EQ(ret, TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    bool isEcc = true;
    OnLoop([&]() { ret = connection->IsEmergencyPhoneNumber("112", 0, isEcc); });
    EXPECT_EQ(ret, TELEPHONY_ERR_IPC_CONNECT_STUB_FAIL);
    Stop();
}

// 退出时注销 (mini-platform-adaptation): the uplink is unregistered before it is dropped.
MINI_TEST(Connection, StopUnregistersTheUplinkFirst)
{
    using namespace OHOS::Telephony;
    using namespace CallFixture;
    ASSERT_TRUE(Start());
    sptr<FakeStack> stack = g_stack;
    sptr<ICallStatusCallback> uplink = stack->callback;
    Stop();
    EXPECT_EQ(stack->unregistrations.load(), 1);
    EXPECT_TRUE(stack->callback == nullptr);
    EXPECT_EQ(uplink->GetSptrRefCount(), 1);
    CallReportInfo info;
    info.index = 1;
    info.state = TelCallState::CALL_STATUS_INCOMING;
    info.callType = CallType::TYPE_CS;
    (void)std::strncpy(info.accountNum, "10086", kMaxNumberLen);
    EXPECT_EQ(uplink->UpdateCallReportInfo(info), TELEPHONY_ERR_UNINIT);
}
