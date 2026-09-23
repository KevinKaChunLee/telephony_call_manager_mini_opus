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

// samgr_lite binding of the Mini call manager (design D1): one SINGLE_TASK service whose task is the
// call manager's main loop. LosEventHandler runs externally driven: every loop message is handed to
// SAMGR_SendRequest by pointer (len 0, so samgr never copies or frees it) and comes back through
// MessageHandle, which dispatches it. The service registers a default feature API so that it can be
// discovered by name; callers in the image use CallManagerClient, which reaches the service in-process.

#include "call_manager_service.h"

#include "call_manager_mini_config.h"
#include "los_event_handler.h"
#include "ohos_errno.h"
#include "ohos_init.h"
#include "samgr_lite.h"
#include "service.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
constexpr const char *CALL_MANAGER_SERVICE_NAME = "call_manager";
constexpr int16 MSG_DISPATCH = 1;

struct CallManagerLiteService {
    INHERIT_SERVICE;
    Identity identity;
};

struct CallManagerLiteApi {
    INHERIT_IUNKNOWN;
};

struct CallManagerLiteApiEntry {
    INHERIT_IUNKNOWNENTRY(CallManagerLiteApi);
};

CallManagerLiteService g_service = {};
CallManagerLiteApiEntry g_apiEntry = {};

bool PostToServiceTask(LosEventMessage *message, void *context)
{
    Request request = {};
    request.msgId = MSG_DISPATCH;
    request.len = 0;
    request.data = message;
    request.msgValue = 0;
    return SAMGR_SendRequest(&g_service.identity, &request, nullptr) == EC_SUCCESS;
}

const char *GetName(Service *service)
{
    return CALL_MANAGER_SERVICE_NAME;
}

BOOL Initialize(Service *service, Identity identity)
{
    g_service.identity = identity;
    LosEventHandler &loop = TelMiniMainLoop();
    loop.BindLoopTask();
    LosEventHandler::Config config = LosEventHandler::DefaultConfig();
    config.loopName = nullptr;
    config.poster = PostToServiceTask;
    config.posterContext = nullptr;
    int32_t ret = loop.Init(config);
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("call manager main loop init failed: %{public}d", ret);
        return FALSE;
    }
    ret = DelayedSingleton<CallManagerService>::GetInstance()->Init();
    if (ret != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("call manager init failed: %{public}d", ret);
        loop.Stop();
        return FALSE;
    }
    return TRUE;
}

BOOL MessageHandle(Service *service, Request *request)
{
    if (request == nullptr || request->msgId != MSG_DISPATCH) {
        return FALSE;
    }
    TelMiniMainLoop().Dispatch(static_cast<LosEventMessage *>(request->data));
    return TRUE;
}

TaskConfig GetTaskConfig(Service *service)
{
    TaskConfig config = {};
    config.level = LEVEL_HIGH;
    config.priority = static_cast<int16>(MINI_TASK_PRIORITY);
    config.stackSize = static_cast<uint16>(MINI_TASK_STACK_SIZE);
    config.queueSize = static_cast<uint16>(MINI_QUEUE_SIZE);
    config.taskFlags = SINGLE_TASK;
    return config;
}

int QueryInterface(IUnknown *iUnknown, int version, void **target)
{
    return IUNKNOWN_QueryInterface(iUnknown, version, target);
}

void CallManagerServiceLiteInit(void)
{
    g_service.GetName = GetName;
    g_service.Initialize = Initialize;
    g_service.MessageHandle = MessageHandle;
    g_service.GetTaskConfig = GetTaskConfig;
    if (!SAMGR_GetInstance()->RegisterService(reinterpret_cast<Service *>(&g_service))) {
        TELEPHONY_LOGE("service %{public}s is already registered", CALL_MANAGER_SERVICE_NAME);
        return;
    }
    g_apiEntry.ver = DEFAULT_VERSION;
    g_apiEntry.ref = 1;
    g_apiEntry.iUnknown.QueryInterface = QueryInterface;
    g_apiEntry.iUnknown.AddRef = IUNKNOWN_AddRef;
    g_apiEntry.iUnknown.Release = IUNKNOWN_Release;
    if (!SAMGR_GetInstance()->RegisterDefaultFeatureApi(CALL_MANAGER_SERVICE_NAME, GET_IUNKNOWN(g_apiEntry))) {
        TELEPHONY_LOGE("default feature api of %{public}s not registered", CALL_MANAGER_SERVICE_NAME);
    }
}
} // namespace

SYS_SERVICE_INIT(CallManagerServiceLiteInit);
} // namespace Telephony
} // namespace OHOS
