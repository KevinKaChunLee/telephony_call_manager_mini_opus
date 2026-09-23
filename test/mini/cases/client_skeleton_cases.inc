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

// CallManagerClient compiled unchanged over the Mini CallManagerProxy. The client keeps its proxy in
// a global once Init has run, so the "before Init" case must stay the first client case.

#include "call_manager_client.h"
#include "call_manager_errors.h"

namespace ClientCases {
constexpr int32_t CALL_MANAGER_SA_ID = 4005;
} // namespace ClientCases

MINI_TEST(Client, InScopeCallsBeforeInitReturnUninit)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    std::shared_ptr<CallManagerClient> client = DelayedSingleton<CallManagerClient>::GetInstance();
    ASSERT_TRUE(client != nullptr);
    AppExecFwk::PacMap extras;
    EXPECT_EQ(client->HangUpCall(1), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(client->DialCall(u"10086", extras), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(client->AnswerCall(1, 0), TELEPHONY_ERR_UNINIT);
}

MINI_TEST(Client, OutOfScopeCallsFailExplicitly)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    std::shared_ptr<CallManagerClient> client = DelayedSingleton<CallManagerClient>::GetInstance();
    ASSERT_TRUE(client != nullptr);
    client->Init(ClientCases::CALL_MANAGER_SA_ID);
    EXPECT_EQ(client->CombineConference(1), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(client->StartDtmf(1, '1'), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(client->SetMuted(true), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    EXPECT_EQ(client->GetCallWaiting(0), CALL_ERR_FUNCTION_NOT_SUPPORTED);
    std::string mac = "00:00:00:00:00:00";
    EXPECT_TRUE(client->RegisterBluetoothCallManagerCallbackPtr(mac) == nullptr);
    EXPECT_FALSE(client->CheckCallRecordingPermission("10086", "token"));
    EXPECT_TRUE(client->HasVoiceCapability());
    client->UnInit();
}

MINI_TEST(Client, InScopeCallsWithoutServiceReturnUninit)
{
    using namespace OHOS;
    using namespace OHOS::Telephony;
    std::shared_ptr<CallManagerClient> client = DelayedSingleton<CallManagerClient>::GetInstance();
    ASSERT_TRUE(client != nullptr);
    client->Init(ClientCases::CALL_MANAGER_SA_ID);
    EXPECT_EQ(client->HangUpCall(1), TELEPHONY_ERR_UNINIT);
    EXPECT_EQ(client->HoldCall(1), TELEPHONY_ERR_UNINIT);
    EXPECT_FALSE(client->HasCall());
    client->UnInit();
}
