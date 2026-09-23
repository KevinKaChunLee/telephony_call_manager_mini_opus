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

#ifndef OHOS_IPC_IREMOTE_BROKER_H
#define OHOS_IPC_IREMOTE_BROKER_H

// Mini stand-in for the ipc component's iremote_broker.h. The reused innerkits interfaces
// (i_call_status_callback.h, transfer_control.h) are included from call_manager_client.h in the same
// directory, which a quoted include always resolves first, so they cannot be replaced by Mini copies;
// this header lets them compile unchanged. There is no IPC on the mini system: the broker is a plain
// local interface base and deliberately has no AsObject(), so nothing can be sent as a remote object.

#include "refbase.h"

namespace OHOS {
class IRemoteBroker : public virtual RefBase {
public:
    IRemoteBroker() = default;
    ~IRemoteBroker() override = default;
};

#define DECLARE_INTERFACE_DESCRIPTOR(DESCRIPTOR)                   \
    static constexpr const char16_t *metaDescriptor_ = DESCRIPTOR; \
    static inline const std::u16string GetDescriptor()             \
    {                                                              \
        return metaDescriptor_;                                    \
    }
} // namespace OHOS
#endif // OHOS_IPC_IREMOTE_BROKER_H
