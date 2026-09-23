/*
 * Copyright (C) 2021-2026 Huawei Device Co., Ltd.
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

// The system-app checks and permission combinations of each entry must stay those of the standard
// entry; test/mini/tools/check_auth_parity.py compares the two files. DialCall also applies the checks
// the standard stub makes on the parcel (number length, whitelisted extras, bundle name from the
// identity adapter). RejectCall with a message and AnswerCall with video or RTT fail before any side
// effect.

#include "call_manager_service.h"

#include "call_ability_report_proxy.h"
#include "call_manager_errors.h"
#include "call_manager_mini_config.h"
#include "call_object_manager.h"
#include "call_permission_adapter.h"
#include "cellular_call_connection.h"
#include "ffrt.h"
#include "los_event_handler.h"
#include "report_call_info_handler.h"
#include "telephony_log_wrapper.h"

namespace OHOS {
namespace Telephony {
namespace {
static constexpr const char *OHOS_PERMISSION_SET_TELEPHONY_STATE = "ohos.permission.SET_TELEPHONY_STATE";
static constexpr const char *OHOS_PERMISSION_GET_TELEPHONY_STATE = "ohos.permission.GET_TELEPHONY_STATE";
static constexpr const char *OHOS_PERMISSION_PLACE_CALL = "ohos.permission.PLACE_CALL";
static constexpr const char *OHOS_PERMISSION_ANSWER_CALL = "ohos.permission.ANSWER_CALL";
static constexpr const char *OHOS_PERMISSION_MANAGE_CALL_FOR_DEVICES = "ohos.permission.MANAGE_CALL_FOR_DEVICES";
static constexpr const char *OHOS_PERMISSION_GET_CALL_TRANSFER_INFO = "ohos.permission.GET_CALL_TRANSFER_INFO";
static constexpr const char *OHOS_PERMISSION_SEND_MESSAGES = "ohos.permission.SEND_MESSAGES";
constexpr int32_t TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID = 4005;
constexpr int32_t TELEPHONY_CELLULAR_CALL_SYS_ABILITY_ID = 4004;

// The standard TelephonyPermission API, answered by the build-time trust model (design D9).
class TelephonyPermission {
public:
    static bool CheckPermission(const std::string &permissionName)
    {
        return CallPermissionAdapter::CheckPermission(permissionName);
    }
    static bool CheckCallerIsSystemApp()
    {
        return CallPermissionAdapter::IsSystemCaller();
    }
};

#ifdef TELEPHONY_MINI_HOST_TEST
int32_t g_failInitStep = 0;
#endif

bool InitStepFails(int32_t step)
{
#ifdef TELEPHONY_MINI_HOST_TEST
    return g_failInitStep == step;
#else
    return false;
#endif
}

enum InitStep : int32_t {
    INIT_STEP_CALL_CONTROL = 1,
    INIT_STEP_REPORT_HANDLER,
    INIT_STEP_CELLULAR_CONNECTION,
};

// Copies the fields CallManagerServiceProxy::DialCall writes and CallManagerServiceStub::OnDialCall
// reads, with the same defaults, and nothing else.
AppExecFwk::PacMap BuildDialInfo(AppExecFwk::PacMap &extras, const std::string &bundleName)
{
    AppExecFwk::PacMap dialInfo;
    dialInfo.PutIntValue("accountId", extras.GetIntValue("accountId"));
    dialInfo.PutIntValue("videoState", extras.GetIntValue("videoState"));
    dialInfo.PutIntValue("dialScene", extras.GetIntValue("dialScene"));
    dialInfo.PutIntValue("dialType", extras.GetIntValue("dialType"));
    dialInfo.PutIntValue("callType", extras.GetIntValue("callType"));
    dialInfo.PutBooleanValue("isRTT", extras.GetBooleanValue("isRTT"));
    dialInfo.PutIntValue("phoneIndex", extras.GetIntValue("phoneIndex"));
    dialInfo.PutStringValue("extraParams", extras.GetStringValue("extraParams"));
    dialInfo.PutStringValue("bundleName", bundleName);
    dialInfo.PutBooleanValue("btSlotIdUnknown", extras.GetBooleanValue("btSlotIdUnknown", false));
    return dialInfo;
}
} // namespace

CallManagerService::CallManagerService() {}

CallManagerService::~CallManagerService() {}

#ifdef TELEPHONY_MINI_HOST_TEST
void CallManagerService::FailInitStepForTest(int32_t step)
{
    g_failInitStep = step;
}
#endif

int32_t CallManagerService::Init()
{
    if (ready_.load()) {
        return TELEPHONY_SUCCESS;
    }
    TelMiniSetConfinementCheck(TelMiniIsOnMainLoop);
    std::shared_ptr<CallControlManager> callControlManager = DelayedSingleton<CallControlManager>::GetInstance();
    if (callControlManager == nullptr || InitStepFails(INIT_STEP_CALL_CONTROL) || !callControlManager->Init()) {
        TELEPHONY_LOGE("CallControlManager init failed!");
        return TELEPHONY_ERR_FAIL;
    }
    if (InitStepFails(INIT_STEP_REPORT_HANDLER)) {
        TELEPHONY_LOGE("ReportCallInfoHandler init failed!");
        callControlManager->UnInit();
        return TELEPHONY_ERR_FAIL;
    }
    DelayedSingleton<ReportCallInfoHandler>::GetInstance()->Init();
    if (InitStepFails(INIT_STEP_CELLULAR_CONNECTION)) {
        TELEPHONY_LOGE("CellularCallConnection init failed!");
        DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UnInit();
        callControlManager->UnInit();
        return TELEPHONY_ERR_FAIL;
    }
    DelayedSingleton<CellularCallConnection>::GetInstance()->Init(TELEPHONY_CELLULAR_CALL_SYS_ABILITY_ID);
    callControlManagerPtr_ = callControlManager;
    ready_.store(true);
    TELEPHONY_LOGI("call manager %{public}d ready", TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID);
    return TELEPHONY_SUCCESS;
}

void CallManagerService::UnInit()
{
    if (!ready_.exchange(false)) {
        return;
    }
    DelayedSingleton<CellularCallConnection>::GetInstance()->UnInit();
    DelayedSingleton<ReportCallInfoHandler>::GetInstance()->UnInit();
    if (callControlManagerPtr_ != nullptr) {
        callControlManagerPtr_->UnInit();
    }
    for (sptr<CallBase> call : CallObjectManager::GetAllCallList()) {
        CallObjectManager::DeleteOneCallObject(call);
    }
    DelayedSingleton<CallAbilityReportProxy>::GetInstance()->UnRegisterCallBack(GetBundleInfo());
    callControlManagerPtr_ = nullptr;
}

bool CallManagerService::IsReady() const
{
    return ready_.load();
}

int32_t CallManagerService::RegisterCallBack(const sptr<ICallAbilityCallback> &callback)
{
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_GET_TELEPHONY_STATE) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_GET_CALL_TRANSFER_INFO)) {
        TELEPHONY_LOGE("Permission denied.");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    return DelayedSingleton<CallAbilityReportProxy>::GetInstance()->RegisterCallBack(callback, GetBundleInfo());
}

int32_t CallManagerService::UnRegisterCallBack()
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!CheckSetTelephonyStatePermission()) {
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    return DelayedSingleton<CallAbilityReportProxy>::GetInstance()->UnRegisterCallBack(GetBundleInfo());
}

int32_t CallManagerService::ObserverOnCallDetailsChange()
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_GET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    std::vector<CallAttributeInfo> callAttributeInfo = CallObjectManager::GetAllCallInfoList();
    for (auto info : callAttributeInfo) {
        DelayedSingleton<CallAbilityReportProxy>::GetInstance()->ReportCallStateInfo(info, GetBundleInfo());
    }
    return TELEPHONY_SUCCESS;
}

int32_t CallManagerService::DialCall(std::u16string number, AppExecFwk::PacMap &extras)
{
    if (number.length() > ACCOUNT_NUMBER_MAX_LENGTH) {
        TELEPHONY_LOGE("the account number length exceeds the limit");
        return CALL_ERR_NUMBER_OUT_OF_RANGE;
    }
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    AppExecFwk::PacMap dialInfo = BuildDialInfo(extras, CallPermissionAdapter::GetCallerBundleName());
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_PLACE_CALL)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->DialCall(number, dialInfo);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::AnswerCall(int32_t callId, int32_t videoState, bool isRTT)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (videoState != static_cast<int32_t>(VideoStateType::TYPE_VOICE)) {
        return CALL_ERR_VIDEO_NOT_SUPPORTED;
    }
    if (isRTT) {
        return CALL_ERR_FUNCTION_NOT_SUPPORTED;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->AnswerCall(callId, videoState, isRTT);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::RejectCall(int32_t callId, bool rejectWithMessage, std::u16string textMessage)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (rejectWithMessage && !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SEND_MESSAGES)) {
        TELEPHONY_LOGE("SEBD_MESSAGES Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (rejectWithMessage) {
        return CALL_ERR_FUNCTION_NOT_SUPPORTED;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->RejectCall(callId, rejectWithMessage, textMessage);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::HangUpCall(int32_t callId)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->HangUpCall(callId);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::GetCallState()
{
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->GetCallState();
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::HoldCall(int32_t callId)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->HoldCall(callId);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::UnHoldCall(int32_t callId)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->UnHoldCall(callId);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::SwitchCall(int32_t callId)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->SwitchCall(callId);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

bool CallManagerService::HasCall(const bool isInCludeVoipCall)
{
    if (callControlManagerPtr_ != nullptr) {
        if (isInCludeVoipCall) {
            return callControlManagerPtr_->HasCall();
        } else {
            return callControlManagerPtr_->HasCellularCallExist();
        }
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return false;
    }
}

int32_t CallManagerService::IsNewCallAllowed(bool &enabled)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->IsNewCallAllowed(enabled);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::IsRinging(bool &enabled)
{
    if (!CheckCallerIsSystemApp()) {
        return TELEPHONY_ERR_ILLEGAL_USE_OF_SYSTEM_API;
    }
    if (!CheckSetTelephonyStatePermission()) {
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->IsRinging(enabled);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled)
{
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->IsEmergencyPhoneNumber(number, slotId, enabled);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

bool CallManagerService::EndCall()
{
    if (!CheckCallerIsSystemApp()) {
        return false;
    }
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Permission denied!");
        return false;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->EndCall();
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return false;
    }
}

int32_t CallManagerService::AnswerCall()
{
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_MANAGE_CALL_FOR_DEVICES)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->AnswerCall(INVALID_CALLID, static_cast<int32_t>(VideoStateType::TYPE_VOICE));
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::RejectCall()
{
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_MANAGE_CALL_FOR_DEVICES)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->RejectCall(INVALID_CALLID, false, u"");
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

int32_t CallManagerService::HangUpCall()
{
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_ANSWER_CALL) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE) &&
        !TelephonyPermission::CheckPermission(OHOS_PERMISSION_MANAGE_CALL_FOR_DEVICES)) {
        TELEPHONY_LOGE("Permission denied!");
        return TELEPHONY_ERR_PERMISSION_ERR;
    }
    if (callControlManagerPtr_ != nullptr) {
        return callControlManagerPtr_->HangUpCall(INVALID_CALLID);
    } else {
        TELEPHONY_LOGE("callControlManagerPtr_ is nullptr!");
        return TELEPHONY_ERR_LOCAL_PTR_NULL;
    }
}

bool CallManagerService::CheckSetTelephonyStatePermission()
{
    if (!TelephonyPermission::CheckPermission(OHOS_PERMISSION_SET_TELEPHONY_STATE)) {
        TELEPHONY_LOGE("Permission denied!");
        return false;
    }
    return true;
}

bool CallManagerService::CheckCallerIsSystemApp()
{
    if (!TelephonyPermission::CheckCallerIsSystemApp()) {
        TELEPHONY_LOGE("Non-system applications use system APIs!");
        return false;
    }
    return true;
}

std::string CallManagerService::GetBundleInfo()
{
    return CallPermissionAdapter::GetCallerBundleName();
}

int32_t CallManagerServiceMiniStart()
{
    LosEventHandler &loop = TelMiniMainLoop();
    if (!loop.IsRunning()) {
        int32_t ret = loop.Init(LosEventHandler::DefaultConfig());
        if (ret != TELEPHONY_SUCCESS) {
            TELEPHONY_LOGE("main loop init failed: %{public}d", ret);
            return ret;
        }
    }
    int32_t result = TELEPHONY_ERR_FAIL;
    int32_t ret = loop.PostSyncCall<int32_t>(
        []() { return DelayedSingleton<CallManagerService>::GetInstance()->Init(); }, result, MINI_SYNC_WAIT_MS);
    if (ret != TELEPHONY_SUCCESS || result != TELEPHONY_SUCCESS) {
        TELEPHONY_LOGE("call manager init failed: post %{public}d, init %{public}d", ret, result);
        loop.Stop();
        return (ret != TELEPHONY_SUCCESS) ? ret : result;
    }
    return TELEPHONY_SUCCESS;
}

void CallManagerServiceMiniStop()
{
    LosEventHandler &loop = TelMiniMainLoop();
    if (!loop.IsRunning()) {
        return;
    }
    loop.PostSyncTask([]() { DelayedSingleton<CallManagerService>::GetInstance()->UnInit(); }, MINI_SYNC_WAIT_MS);
    loop.Stop();
}
} // namespace Telephony
} // namespace OHOS
