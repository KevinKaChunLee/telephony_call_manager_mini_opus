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

#include <atomic>
#include <string>

#include "call_manager_mini_config.h"
#include "ffrt.h"
#include "pac_map.h"
#include "refbase.h"
#include "singleton.h"
#include "string_ex.h"
#include "tel_os_adapter.h"
#include "telephony_log_wrapper.h"

namespace BaseShimCases {
using namespace OHOS;
using namespace OHOS::Telephony;

constexpr int WORKER_COUNT = 4;

class Tracked : public virtual RefBase {
public:
    Tracked()
    {
        alive.fetch_add(1);
    }
    ~Tracked() override
    {
        alive.fetch_sub(1);
        destroyed.fetch_add(1);
    }
    static std::atomic<int> alive;
    static std::atomic<int> destroyed;
};
std::atomic<int> Tracked::alive { 0 };
std::atomic<int> Tracked::destroyed { 0 };

struct RefWorkers {
    sptr<Tracked> shared;
    TelOsHandle done = nullptr;
};
RefWorkers g_refWorkers;

void CopyAndDropRefs(void *)
{
    constexpr int rounds = 20000;
    for (int i = 0; i < rounds; i++) {
        sptr<Tracked> copy = g_refWorkers.shared;
        wptr<Tracked> weak = copy;
        sptr<Tracked> promoted = weak.promote();
        if (promoted == nullptr) {
            break;
        }
    }
    TelOsSemPost(g_refWorkers.done);
}

class SlowSingleton {
    DECLARE_DELAYED_SINGLETON(SlowSingleton)
public:
    static std::atomic<int> constructions;
};
std::atomic<int> SlowSingleton::constructions { 0 };
SlowSingleton::SlowSingleton()
{
    constructions.fetch_add(1);
    TelOsTaskDelayMs(30);
}
SlowSingleton::~SlowSingleton() {}

struct SingletonWorkers {
    std::atomic<SlowSingleton *> seen[WORKER_COUNT];
    std::atomic<int> nextSlot { 0 };
    TelOsHandle done = nullptr;
};
SingletonWorkers g_singletonWorkers;

void FetchSingleton(void *)
{
    int slot = g_singletonWorkers.nextSlot.fetch_add(1);
    g_singletonWorkers.seen[slot].store(DelayedSingleton<SlowSingleton>::GetInstance().get());
    TelOsSemPost(g_singletonWorkers.done);
}

bool StartWorkers(TelOsTaskEntry entry, TelOsHandle *done)
{
    if (TelOsSemCreate(0, WORKER_COUNT, done) != TelOsResult::OK) {
        return false;
    }
    for (int i = 0; i < WORKER_COUNT; i++) {
        TelOsTaskParam param;
        param.name = "mini_test_worker";
        param.entry = entry;
        param.stackSize = 16384;
        param.priority = 20;
        if (TelOsTaskCreate(param) != TelOsResult::OK) {
            return false;
        }
    }
    for (int i = 0; i < WORKER_COUNT; i++) {
        if (TelOsSemWait(*done, 10000) != TelOsResult::OK) {
            return false;
        }
    }
    TelOsSemDelete(*done);
    *done = nullptr;
    return true;
}

char g_lastLog[512];

void CaptureLog(TelMiniLogLevel, const char *message)
{
    std::snprintf(g_lastLog, sizeof(g_lastLog), "%s", message);
}

bool EndsWith(const char *text, const char *suffix)
{
    size_t textLen = std::strlen(text);
    size_t suffixLen = std::strlen(suffix);
    return textLen >= suffixLen && std::strcmp(text + textLen - suffixLen, suffix) == 0;
}

bool OffLoop()
{
    return false;
}
} // namespace BaseShimCases

MINI_TEST(RefBase, LastSptrDestroysAndWptrStopsPromoting)
{
    using namespace BaseShimCases;
    Tracked::alive.store(0);
    Tracked::destroyed.store(0);
    sptr<Tracked> first = new Tracked();
    sptr<Tracked> second = first;
    wptr<Tracked> weak = first;
    EXPECT_EQ(first->GetSptrRefCount(), 2);
    EXPECT_TRUE(weak.promote() != nullptr);
    first = nullptr;
    EXPECT_EQ(Tracked::alive.load(), 1);
    second.clear();
    EXPECT_EQ(Tracked::alive.load(), 0);
    EXPECT_EQ(Tracked::destroyed.load(), 1);
    EXPECT_TRUE(weak.promote() == nullptr);
    sptr<Tracked> made = sptr<Tracked>::MakeSptr();
    EXPECT_TRUE(made != nullptr);
    made = nullptr;
    EXPECT_EQ(Tracked::destroyed.load(), 2);
}

MINI_TEST(RefBase, ConcurrentCopiesDestroyExactlyOnce)
{
    using namespace BaseShimCases;
    Tracked::alive.store(0);
    Tracked::destroyed.store(0);
    g_refWorkers.shared = new Tracked();
    EXPECT_TRUE(StartWorkers(CopyAndDropRefs, &g_refWorkers.done));
    EXPECT_EQ(g_refWorkers.shared->GetSptrRefCount(), 1);
    EXPECT_EQ(Tracked::alive.load(), 1);
    g_refWorkers.shared = nullptr;
    EXPECT_EQ(Tracked::alive.load(), 0);
    EXPECT_EQ(Tracked::destroyed.load(), 1);
}

MINI_TEST(Singleton, ConcurrentFirstAccessConstructsOnce)
{
    using namespace BaseShimCases;
    for (auto &slot : g_singletonWorkers.seen) {
        slot.store(nullptr);
    }
    EXPECT_TRUE(StartWorkers(FetchSingleton, &g_singletonWorkers.done));
    EXPECT_EQ(SlowSingleton::constructions.load(), 1);
    SlowSingleton *expected = DelayedSingleton<SlowSingleton>::GetInstance().get();
    EXPECT_TRUE(expected != nullptr);
    for (auto &slot : g_singletonWorkers.seen) {
        EXPECT_TRUE(slot.load() == expected);
    }
    DelayedSingleton<SlowSingleton>::DestroyInstance();
    EXPECT_TRUE(DelayedSingleton<SlowSingleton>::GetInstance() != nullptr);
    EXPECT_EQ(SlowSingleton::constructions.load(), 2);
    DelayedSingleton<SlowSingleton>::DestroyInstance();
}

MINI_TEST(PacMap, DefaultsAndTypedAccess)
{
    OHOS::AppExecFwk::PacMap map;
    EXPECT_TRUE(map.IsEmpty());
    EXPECT_EQ(map.GetIntValue("missing", 7), 7);
    EXPECT_EQ(map.GetIntValue("missing"), 0);
    EXPECT_TRUE(map.GetStringValue("missing", "fallback") == "fallback");
    map.PutIntValue("accountId", 1);
    map.PutStringValue("bundleName", "mini");
    map.PutBooleanValue("isRTT", true);
    map.PutLongValue("time", 1234567L);
    EXPECT_EQ(map.GetIntValue("accountId"), 1);
    EXPECT_TRUE(map.GetStringValue("bundleName") == "mini");
    EXPECT_TRUE(map.GetBooleanValue("isRTT"));
    EXPECT_EQ(map.GetLongValue("time"), 1234567L);
    EXPECT_TRUE(map.GetStringValue("accountId", "wrong type") == "wrong type");
    EXPECT_EQ(map.GetIntValue("bundleName", -1), -1);
    OHOS::AppExecFwk::PacMap copy = map;
    map.Remove("accountId");
    EXPECT_FALSE(map.HasKey("accountId"));
    EXPECT_EQ(copy.GetIntValue("accountId"), 1);
    map.Clear();
    EXPECT_EQ(map.GetSize(), 0);
    EXPECT_EQ(copy.GetSize(), 4);
}

MINI_TEST(PacMap, PutBeyondEntryLimitIsDropped)
{
    OHOS::AppExecFwk::PacMap map;
    const int limit = static_cast<int>(OHOS::Telephony::MINI_PACMAP_MAX_ENTRIES);
    for (int i = 0; i < limit; i++) {
        map.PutIntValue("key" + std::to_string(i), i);
    }
    EXPECT_EQ(map.GetSize(), limit);
    map.PutIntValue("oneTooMany", 1);
    EXPECT_FALSE(map.HasKey("oneTooMany"));
    EXPECT_EQ(map.GetSize(), limit);
    map.PutIntValue("key0", 100);
    EXPECT_EQ(map.GetIntValue("key0"), 100);
}

MINI_TEST(Log, PrivacyTagsAreStripped)
{
    using namespace BaseShimCases;
    TelMiniLogSetLevel(TelMiniLogLevel::DEBUG);
    TelMiniLogSetSink(CaptureLog);
    g_lastLog[0] = '\0';
    TELEPHONY_LOGE("slot=%{public}d name=%{private}s rate=%%d", 42, "x");
    char expected[64];
    std::snprintf(expected, sizeof(expected), "slot=%d name=%s rate=%%d", 42, "x");
    EXPECT_TRUE(EndsWith(g_lastLog, expected));
    TELEPHONY_LOGI("init first please!");
    EXPECT_TRUE(EndsWith(g_lastLog, "init first please!"));
    std::string longFormat(300, 'a');
    TELEPHONY_LOGE(longFormat.c_str());
    EXPECT_TRUE(EndsWith(g_lastLog, "log format too long"));
    TelMiniLogSetLevel(TelMiniLogLevel::ERROR);
    g_lastLog[0] = '\0';
    TELEPHONY_LOGI("filtered");
    EXPECT_EQ(g_lastLog[0], '\0');
    TelMiniLogSetSink(nullptr);
    TelMiniLogSetLevel(TelMiniLogLevel::FATAL);
}

MINI_TEST(StringEx, Utf8Utf16RoundTrip)
{
    EXPECT_TRUE(OHOS::Str16ToStr8(u"+86 138-0000-0000") == "+86 138-0000-0000");
    EXPECT_TRUE(OHOS::Str8ToStr16("112") == u"112");
    std::string mixed = "\xE4\xB8\xAD\xF0\x9F\x93\x9E";
    EXPECT_TRUE(OHOS::Str16ToStr8(OHOS::Str8ToStr16(mixed)) == mixed);
    EXPECT_EQ(OHOS::Str8ToStr16(mixed).size(), 3U);
    EXPECT_TRUE(OHOS::Str8ToStr16("\xFF").empty());
    EXPECT_TRUE(OHOS::Str8ToStr16("\xE4\xB8").empty());
    std::u16string loneSurrogate(1, static_cast<char16_t>(0xD800));
    EXPECT_TRUE(OHOS::Str16ToStr8(loneSurrogate).empty());
}

MINI_TEST(Ffrt, LockOffTheMainLoopIsCounted)
{
    using namespace BaseShimCases;
    uint32_t before = TelMiniConfinementViolations();
    ffrt::mutex mutex;
    {
        std::lock_guard<ffrt::mutex> unchecked(mutex);
    }
    EXPECT_EQ(TelMiniConfinementViolations(), before);
    TelMiniSetConfinementCheck(OffLoop);
    {
        std::lock_guard<ffrt::mutex> checked(mutex);
    }
    TelMiniSetConfinementCheck(nullptr);
    EXPECT_EQ(TelMiniConfinementViolations(), before + 1);
    ffrt::condition_variable cv;
    std::unique_lock<ffrt::mutex> lock(mutex);
    EXPECT_TRUE(cv.wait_for(lock, std::chrono::seconds(1)) == ffrt::cv_status::timeout);
}
