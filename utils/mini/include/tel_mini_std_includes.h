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

#ifndef TELEPHONY_MINI_TEL_MINI_STD_INCLUDES_H
#define TELEPHONY_MINI_TEL_MINI_STD_INCLUDES_H

// The reused standard headers (call_manager_base.h, call_object_manager.h, ...) rely on these
// being included already; in the standard build other system headers bring them in. Every Mini
// stand-in that such a header includes first must include this file first.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#endif // TELEPHONY_MINI_TEL_MINI_STD_INCLUDES_H
