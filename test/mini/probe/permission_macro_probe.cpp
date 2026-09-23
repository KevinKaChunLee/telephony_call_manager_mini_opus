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

// Checks CallPermissionAdapter against the defines callmanager_mini.gni produces. mini_selftest.sh
// builds it with call_permission_adapter.cpp once per trust model and passes the expectation as
// PROBE_EXPECT_SYSTEM_APP / PROBE_EXPECT_BUNDLE.

#include <cstdio>
#include <cstring>

#include "call_permission_adapter.h"
#include "telephony_log_wrapper.h"

#ifndef PROBE_EXPECT_BUNDLE
#define PROBE_EXPECT_BUNDLE ""
#endif

int main()
{
    using OHOS::Telephony::CallPermissionAdapter;
    OHOS::Telephony::TelMiniLogSetLevel(OHOS::Telephony::TelMiniLogLevel::FATAL);
#ifdef PROBE_EXPECT_SYSTEM_APP
    const bool expectSystemApp = true;
#else
    const bool expectSystemApp = false;
#endif
    int failures = 0;
    auto check = [&failures](bool ok, const char *what) {
        if (!ok) {
            std::printf("permission probe: %s\n", what);
            failures++;
        }
    };
    check(CallPermissionAdapter::CheckPermission("ohos.permission.PLACE_CALL"), "PLACE_CALL not granted");
    check(!CallPermissionAdapter::CheckPermission("ohos.permission.ANSWER_CALL"), "ANSWER_CALL granted");
    check(!CallPermissionAdapter::CheckPermission("ohos.permission.SET_TELEPHONY_STATE"),
        "SET_TELEPHONY_STATE granted");
    check(!CallPermissionAdapter::CheckPermission("ohos.permission.PLACE"), "prefix of PLACE_CALL granted");
    check(CallPermissionAdapter::IsSystemCaller() == expectSystemApp, "system app declaration not applied");
    check(CallPermissionAdapter::GetCallerBundleName() == PROBE_EXPECT_BUNDLE, "bundle name not applied");
    return (failures == 0) ? 0 : 1;
}
