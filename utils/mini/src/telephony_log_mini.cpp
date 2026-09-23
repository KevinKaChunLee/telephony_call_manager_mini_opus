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

#include "telephony_log_wrapper.h"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace OHOS {
namespace Telephony {
namespace {
constexpr size_t LOG_FORMAT_MAX = 256;
constexpr size_t LOG_MESSAGE_MAX = 320;
constexpr char PUBLIC_TAG[] = "{public}";
constexpr char PRIVATE_TAG[] = "{private}";

std::atomic<int32_t> g_minLevel { static_cast<int32_t>(TelMiniLogLevel::INFO) };
std::atomic<TelMiniLogSink> g_sink { nullptr };

#ifdef TELEPHONY_MINI_LOG_PRINTF
const char *LevelName(TelMiniLogLevel level)
{
    switch (level) {
        case TelMiniLogLevel::DEBUG:
            return "D";
        case TelMiniLogLevel::INFO:
            return "I";
        case TelMiniLogLevel::WARN:
            return "W";
        case TelMiniLogLevel::ERROR:
            return "E";
        default:
            return "F";
    }
}
#endif

bool StripPrivacyTags(const char *fmt, char *out, size_t outSize)
{
    size_t outPos = 0;
    size_t pos = 0;
    while (fmt[pos] != '\0') {
        size_t copyLen = 1;
        size_t skipLen = 0;
        if (fmt[pos] == '%' && fmt[pos + 1] == '%') {
            copyLen = 2;
        } else if (fmt[pos] == '%' && std::strncmp(&fmt[pos + 1], PUBLIC_TAG, sizeof(PUBLIC_TAG) - 1) == 0) {
            skipLen = sizeof(PUBLIC_TAG) - 1;
        } else if (fmt[pos] == '%' && std::strncmp(&fmt[pos + 1], PRIVATE_TAG, sizeof(PRIVATE_TAG) - 1) == 0) {
            skipLen = sizeof(PRIVATE_TAG) - 1;
        }
        if (outPos + copyLen >= outSize) {
            return false;
        }
        std::memcpy(&out[outPos], &fmt[pos], copyLen);
        outPos += copyLen;
        pos += copyLen + skipLen;
    }
    out[outPos] = '\0';
    return true;
}

void ConsoleSink(TelMiniLogLevel level, const char *message)
{
#ifdef TELEPHONY_MINI_LOG_PRINTF
    std::printf("[CallManager][%s]%s\n", LevelName(level), message);
#endif
}
} // namespace

void TelMiniLog(TelMiniLogLevel level, const char *func, int32_t line, const char *fmt, ...)
{
    if (static_cast<int32_t>(level) < g_minLevel.load(std::memory_order_relaxed) || fmt == nullptr) {
        return;
    }
    char message[LOG_MESSAGE_MAX];
    int prefixLen = std::snprintf(message, sizeof(message), "[%s:%d] ", (func == nullptr) ? "?" : func,
        static_cast<int>(line));
    if (prefixLen < 0 || static_cast<size_t>(prefixLen) >= sizeof(message)) {
        prefixLen = 0;
        message[0] = '\0';
    }
    char format[LOG_FORMAT_MAX];
    if (!StripPrivacyTags(fmt, format, sizeof(format))) {
        std::snprintf(&message[prefixLen], sizeof(message) - prefixLen, "log format too long");
    } else {
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(&message[prefixLen], sizeof(message) - prefixLen, format, args);
        va_end(args);
    }
    TelMiniLogSink sink = g_sink.load(std::memory_order_acquire);
    if (sink != nullptr) {
        sink(level, message);
    } else {
        ConsoleSink(level, message);
    }
}

void TelMiniLogSetLevel(TelMiniLogLevel minLevel)
{
    g_minLevel.store(static_cast<int32_t>(minLevel), std::memory_order_relaxed);
}

void TelMiniLogSetSink(TelMiniLogSink sink)
{
    g_sink.store(sink, std::memory_order_release);
}
} // namespace Telephony
} // namespace OHOS
