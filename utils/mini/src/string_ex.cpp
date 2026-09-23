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

#include "string_ex.h"

#include <cstdint>

namespace OHOS {
namespace {
constexpr char32_t SURROGATE_HIGH_BEGIN = 0xD800;
constexpr char32_t SURROGATE_LOW_BEGIN = 0xDC00;
constexpr char32_t SURROGATE_END = 0xDFFF;
constexpr char32_t SUPPLEMENTARY_BEGIN = 0x10000;
constexpr char32_t CODE_POINT_MAX = 0x10FFFF;
constexpr uint32_t SURROGATE_BITS = 10;
constexpr char32_t SURROGATE_MASK = 0x3FF;

void AppendUtf8(std::string &out, char32_t cp)
{
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < SUPPLEMENTARY_BEGIN) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

// Returns false on a truncated or overlong sequence, a surrogate or a value above U+10FFFF.
bool DecodeUtf8(const std::string &in, size_t &pos, char32_t &cp)
{
    unsigned char lead = static_cast<unsigned char>(in[pos]);
    size_t extra = 0;
    char32_t minValue = 0;
    if (lead < 0x80) {
        cp = lead;
        pos++;
        return true;
    } else if ((lead & 0xE0) == 0xC0) {
        extra = 1;
        cp = lead & 0x1F;
        minValue = 0x80;
    } else if ((lead & 0xF0) == 0xE0) {
        extra = 2;
        cp = lead & 0x0F;
        minValue = 0x800;
    } else if ((lead & 0xF8) == 0xF0) {
        extra = 3;
        cp = lead & 0x07;
        minValue = SUPPLEMENTARY_BEGIN;
    } else {
        return false;
    }
    if (pos + extra >= in.size()) {
        return false;
    }
    for (size_t i = 1; i <= extra; i++) {
        unsigned char next = static_cast<unsigned char>(in[pos + i]);
        if ((next & 0xC0) != 0x80) {
            return false;
        }
        cp = (cp << 6) | (next & 0x3F);
    }
    if (cp < minValue || cp > CODE_POINT_MAX || (cp >= SURROGATE_HIGH_BEGIN && cp <= SURROGATE_END)) {
        return false;
    }
    pos += extra + 1;
    return true;
}
} // namespace

std::string Str16ToStr8(const std::u16string &str16)
{
    std::string out;
    out.reserve(str16.size());
    for (size_t i = 0; i < str16.size(); i++) {
        char32_t unit = str16[i];
        if (unit >= SURROGATE_HIGH_BEGIN && unit < SURROGATE_LOW_BEGIN) {
            if (i + 1 >= str16.size()) {
                return "";
            }
            char32_t low = str16[i + 1];
            if (low < SURROGATE_LOW_BEGIN || low > SURROGATE_END) {
                return "";
            }
            unit = SUPPLEMENTARY_BEGIN + (((unit - SURROGATE_HIGH_BEGIN) << SURROGATE_BITS) |
                (low - SURROGATE_LOW_BEGIN));
            i++;
        } else if (unit >= SURROGATE_LOW_BEGIN && unit <= SURROGATE_END) {
            return "";
        }
        AppendUtf8(out, unit);
    }
    return out;
}

std::u16string Str8ToStr16(const std::string &str)
{
    std::u16string out;
    out.reserve(str.size());
    size_t pos = 0;
    while (pos < str.size()) {
        char32_t cp = 0;
        if (!DecodeUtf8(str, pos, cp)) {
            return u"";
        }
        if (cp >= SUPPLEMENTARY_BEGIN) {
            cp -= SUPPLEMENTARY_BEGIN;
            out.push_back(static_cast<char16_t>(SURROGATE_HIGH_BEGIN + (cp >> SURROGATE_BITS)));
            out.push_back(static_cast<char16_t>(SURROGATE_LOW_BEGIN + (cp & SURROGATE_MASK)));
        } else {
            out.push_back(static_cast<char16_t>(cp));
        }
    }
    return out;
}
} // namespace OHOS
