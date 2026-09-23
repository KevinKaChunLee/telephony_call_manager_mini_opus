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

#ifndef TELEPHONY_MINI_TEST_STUB_SECUREC_H
#define TELEPHONY_MINI_TEST_STUB_SECUREC_H

// Host-only stand-in for third_party/bounds_checking_function (which product images link
// for real). Signatures and error codes follow its include/securec.h; only the functions the
// Mini sources call are provided, and on every failure the destination is reset the way the
// upstream library resets it.

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>

typedef int errno_t;

#ifndef EOK
#define EOK 0
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef ERANGE
#define ERANGE 34
#endif
#ifndef EINVAL_AND_RESET
#define EINVAL_AND_RESET 150
#endif
#ifndef ERANGE_AND_RESET
#define ERANGE_AND_RESET 162
#endif
#ifndef EOVERLAP_AND_RESET
#define EOVERLAP_AND_RESET 182
#endif

inline errno_t memset_s(void *dest, size_t destMax, int c, size_t count)
{
    if (dest == nullptr) {
        return EINVAL;
    }
    if (destMax == 0) {
        return ERANGE;
    }
    if (count > destMax) {
        std::memset(dest, c, destMax);
        return ERANGE_AND_RESET;
    }
    std::memset(dest, c, count);
    return EOK;
}

inline errno_t memcpy_s(void *dest, size_t destMax, const void *src, size_t count)
{
    if (dest == nullptr) {
        return EINVAL;
    }
    if (destMax == 0) {
        return ERANGE;
    }
    if (src == nullptr) {
        std::memset(dest, 0, destMax);
        return EINVAL_AND_RESET;
    }
    if (count > destMax) {
        std::memset(dest, 0, destMax);
        return ERANGE_AND_RESET;
    }
    const char *d = static_cast<const char *>(dest);
    const char *s = static_cast<const char *>(src);
    if (count > 0 && ((d < s + count && s < d + count))) {
        std::memset(dest, 0, destMax);
        return EOVERLAP_AND_RESET;
    }
    std::memcpy(dest, src, count);
    return EOK;
}

inline errno_t memmove_s(void *dest, size_t destMax, const void *src, size_t count)
{
    if (dest == nullptr) {
        return EINVAL;
    }
    if (destMax == 0) {
        return ERANGE;
    }
    if (src == nullptr) {
        std::memset(dest, 0, destMax);
        return EINVAL_AND_RESET;
    }
    if (count > destMax) {
        std::memset(dest, 0, destMax);
        return ERANGE_AND_RESET;
    }
    std::memmove(dest, src, count);
    return EOK;
}

inline errno_t strcpy_s(char *strDest, size_t destMax, const char *strSrc)
{
    if (strDest == nullptr) {
        return EINVAL;
    }
    if (destMax == 0) {
        return ERANGE;
    }
    if (strSrc == nullptr) {
        strDest[0] = '\0';
        return EINVAL_AND_RESET;
    }
    size_t len = std::strlen(strSrc);
    if (len >= destMax) {
        strDest[0] = '\0';
        return ERANGE_AND_RESET;
    }
    std::memcpy(strDest, strSrc, len + 1);
    return EOK;
}

inline errno_t strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
    if (strDest == nullptr) {
        return EINVAL;
    }
    if (destMax == 0) {
        return ERANGE;
    }
    if (strSrc == nullptr) {
        strDest[0] = '\0';
        return EINVAL_AND_RESET;
    }
    size_t len = 0;
    while (len < count && strSrc[len] != '\0') {
        len++;
    }
    if (len >= destMax) {
        strDest[0] = '\0';
        return ERANGE_AND_RESET;
    }
    std::memcpy(strDest, strSrc, len);
    strDest[len] = '\0';
    return EOK;
}

__attribute__((format(printf, 4, 5))) inline int snprintf_s(
    char *strDest, size_t destMax, size_t count, const char *format, ...)
{
    if (strDest == nullptr || format == nullptr || destMax == 0) {
        return -1;
    }
    size_t limit = (count < destMax) ? (count + 1) : destMax;
    va_list args;
    va_start(args, format);
    int ret = std::vsnprintf(strDest, limit, format, args);
    va_end(args);
    if (ret < 0) {
        strDest[0] = '\0';
        return -1;
    }
    if (static_cast<size_t>(ret) >= limit) {
        if (count >= destMax) {
            strDest[0] = '\0';
        }
        return -1;
    }
    return ret;
}

#endif // TELEPHONY_MINI_TEST_STUB_SECUREC_H
