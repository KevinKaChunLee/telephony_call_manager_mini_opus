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

#ifndef TELEPHONY_LOG_WRAPPER_H
#define TELEPHONY_LOG_WRAPPER_H

// Mini stand-in for core_service utils/log/include/telephony_log_wrapper.h, keeping the macro
// names. Reused standard code passes hilog formats such as "%{public}d"; TelMiniLog strips the
// {public}/{private} tags at run time, so it deliberately carries no printf format attribute.
// Output goes to the console when TELEPHONY_MINI_LOG_PRINTF is defined and is dropped otherwise.

#include <cstdint>

namespace OHOS {
namespace Telephony {
enum class TelMiniLogLevel : int32_t {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

using TelMiniLogSink = void (*)(TelMiniLogLevel level, const char *message);

void TelMiniLog(TelMiniLogLevel level, const char *func, int32_t line, const char *fmt, ...);
void TelMiniLogSetLevel(TelMiniLogLevel minLevel);
// nullptr restores the default console sink.
void TelMiniLogSetSink(TelMiniLogSink sink);
} // namespace Telephony
} // namespace OHOS

#define TELEPHONY_LOGD(fmt, ...) \
    ::OHOS::Telephony::TelMiniLog(::OHOS::Telephony::TelMiniLogLevel::DEBUG, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define TELEPHONY_LOGI(fmt, ...) \
    ::OHOS::Telephony::TelMiniLog(::OHOS::Telephony::TelMiniLogLevel::INFO, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define TELEPHONY_LOGW(fmt, ...) \
    ::OHOS::Telephony::TelMiniLog(::OHOS::Telephony::TelMiniLogLevel::WARN, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define TELEPHONY_LOGE(fmt, ...) \
    ::OHOS::Telephony::TelMiniLog(::OHOS::Telephony::TelMiniLogLevel::ERROR, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define TELEPHONY_LOGF(fmt, ...) \
    ::OHOS::Telephony::TelMiniLog(::OHOS::Telephony::TelMiniLogLevel::FATAL, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

#endif // TELEPHONY_LOG_WRAPPER_H
