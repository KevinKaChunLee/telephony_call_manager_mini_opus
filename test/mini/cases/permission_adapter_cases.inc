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

// CallPermissionAdapter cases. The gates build with no trust-model defines, which is the product
// default; the build-time injection path is covered by test/mini/probe/permission_macro_probe.cpp.

#include "call_permission_adapter.h"

MINI_TEST(PermissionAdapter, DefaultConfigDeniesEverything)
{
    using namespace OHOS::Telephony;
    CallPermissionAdapter::OverrideForTest(nullptr, false, nullptr);
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE_CALL"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.ANSWER_CALL"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.SET_TELEPHONY_STATE"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission(""));
    EXPECT_FALSE(CallPermissionAdapter::IsSystemCaller());
    EXPECT_TRUE(CallPermissionAdapter::GetCallerBundleName().empty());
}

MINI_TEST(PermissionAdapter, GrantedListMatchesExactly)
{
    using namespace OHOS::Telephony;
    CallPermissionAdapter::OverrideForTest("ohos.permission.PLACE_CALL;;ohos.permission.ANSWER_CALL;", true,
        "com.example.dialer");
    EXPECT_TRUE(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE_CALL"));
    EXPECT_TRUE(CallPermissionAdapter::CheckPermission("ohos.permission.ANSWER_CALL"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE_CALLS"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.SET_TELEPHONY_STATE"));
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission(""));
    EXPECT_TRUE(CallPermissionAdapter::IsSystemCaller());
    EXPECT_TRUE(CallPermissionAdapter::GetCallerBundleName() == "com.example.dialer");
    CallPermissionAdapter::OverrideForTest(nullptr, false, nullptr);
    EXPECT_FALSE(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE_CALL"));
    EXPECT_FALSE(CallPermissionAdapter::IsSystemCaller());
}
