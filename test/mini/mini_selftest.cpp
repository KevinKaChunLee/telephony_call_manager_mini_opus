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

// Self-test for the Mini (LiteOS-M) call manager. The gates build exactly this one test source
// (test/mini/mini_selftest.sh with the POSIX kernel adapter, the LiteOS harness's
// run_mini_cmsis.sh with the CMSIS one), so the cases live in cases/*.inc and are included below.
// Usage: mini_selftest [substring-filter]; the exit status is non-zero when any case fails.
// MINI_TEST_FILTER supplies the filter when no argument is given, for gates that pass none
// (for example SEM_LIMIT=1 MINI_TEST_FILTER=InitFailure bash run_mini_cmsis.sh).

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "telephony_log_wrapper.h"

namespace MiniTest {
using TestFn = void (*)();

struct Case {
    const char *suite;
    const char *name;
    TestFn fn;
};

std::vector<Case> &Registry()
{
    static std::vector<Case> cases;
    return cases;
}

int g_caseFailures = 0;

struct Registrar {
    Registrar(const char *suite, const char *name, TestFn fn)
    {
        Registry().push_back({ suite, name, fn });
    }
};

void ReportFailure(const char *file, int line, const char *expr)
{
    g_caseFailures++;
    std::printf("%s:%d: expectation failed: %s\n", file, line, expr);
}
} // namespace MiniTest

#define MINI_TEST(suite, name)                                                                     \
    static void MiniTest_##suite##_##name();                                                       \
    static MiniTest::Registrar g_miniTestRegistrar_##suite##_##name(#suite, #name, MiniTest_##suite##_##name); \
    static void MiniTest_##suite##_##name()

#define EXPECT_TRUE(cond)                                         \
    do {                                                          \
        if (!(cond)) {                                            \
            MiniTest::ReportFailure(__FILE__, __LINE__, #cond);   \
        }                                                         \
    } while (0)

#define EXPECT_FALSE(cond) EXPECT_TRUE(!(cond))

#define EXPECT_EQ(actual, expected)                                                   \
    do {                                                                              \
        if (!((actual) == (expected))) {                                              \
            MiniTest::ReportFailure(__FILE__, __LINE__, #actual " == " #expected);    \
        }                                                                             \
    } while (0)

#define ASSERT_EQ(actual, expected)                                                   \
    do {                                                                              \
        if (!((actual) == (expected))) {                                              \
            MiniTest::ReportFailure(__FILE__, __LINE__, #actual " == " #expected);    \
            return;                                                                   \
        }                                                                             \
    } while (0)

#define ASSERT_TRUE(cond)                                         \
    do {                                                          \
        if (!(cond)) {                                            \
            MiniTest::ReportFailure(__FILE__, __LINE__, #cond);   \
            return;                                               \
        }                                                         \
    } while (0)

#include "cases/os_adapter_cases.inc"
#include "cases/base_shim_cases.inc"
#include "cases/client_skeleton_cases.inc"
#include "cases/event_handler_cases.inc"
#include "cases/permission_adapter_cases.inc"
#include "cases/call_fixture.inc"
#include "cases/adapter_cases.inc"
#include "cases/call_object_cases.inc"
#include "cases/call_flow_cases.inc"
#include "cases/service_cases.inc"

int main(int argc, char *argv[])
{
    const char *filter = (argc > 1) ? argv[1] : std::getenv("MINI_TEST_FILTER");
    const char *verbose = std::getenv("MINI_TEST_VERBOSE_LOG");
    OHOS::Telephony::TelMiniLogSetLevel((verbose != nullptr) ? OHOS::Telephony::TelMiniLogLevel::DEBUG :
        OHOS::Telephony::TelMiniLogLevel::FATAL);
    int run = 0;
    int failed = 0;
    for (const MiniTest::Case &testCase : MiniTest::Registry()) {
        char fullName[128];
        std::snprintf(fullName, sizeof(fullName), "%s.%s", testCase.suite, testCase.name);
        if (filter != nullptr && std::strstr(fullName, filter) == nullptr) {
            continue;
        }
        std::printf("[ RUN      ] %s\n", fullName);
        MiniTest::g_caseFailures = 0;
        testCase.fn();
        run++;
        if (MiniTest::g_caseFailures == 0) {
            std::printf("[       OK ] %s\n", fullName);
        } else {
            failed++;
            std::printf("[  FAILED  ] %s\n", fullName);
        }
    }
    std::printf("[==========] %d cases run, %d passed, %d failed\n", run, run - failed, failed);
    return (failed == 0 && run > 0) ? 0 : 1;
}
