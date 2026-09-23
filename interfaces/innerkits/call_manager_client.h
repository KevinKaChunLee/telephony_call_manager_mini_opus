/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef CALL_MANAGER_CLIENT_H
#define CALL_MANAGER_CLIENT_H

#include "singleton.h"
#include "pac_map.h"

#include "call_manager_callback.h"
#include "i_call_status_callback.h"
#include "transfer_control.h"

namespace OHOS {
namespace Telephony {
class CallManagerClient : public std::enable_shared_from_this<CallManagerClient> {
    DECLARE_DELAYED_SINGLETON(CallManagerClient)

public:
    void Init(int32_t systemAbilityId);
    void UnInit();

    /**
     * @brief Register callback
     *
     * @param callback[in], callback function pointer
     * @return Returns 0 on success, others on failure.
     */
    int32_t RegisterCallBack(std::unique_ptr<CallManagerCallback> callback);

    /**
     * @brief unregister callback
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnRegisterCallBack();

    /**
     * @brief the application subscribe the OnCallDetailsChange event
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t ObserverOnCallDetailsChange();

    /**
     * @brief the application subscribe the OnMeeTimeDetailsChange event
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t ObserverOnMeeTimeDetailsChange();

    /**
     * @brief Dial a phone call
     *
     * @param number[in], dial param.
     * @param extras[in], extras date.
     * @return Returns callId when the value is greater than zero, others on failure.
     */
    int32_t DialCall(std::u16string number, AppExecFwk::PacMap &extras);

    /**
     * @brief Make a phone call
     *
     * @param number[in],  call param.
     * @return Returns 0 on success, others on failure.
     */
    int32_t MakeCall(std::string number);

    /**
     * @brief Make a phone call with options, generate a token for the call
     *
     * @param number[in], phone number to call
     * @param options[in], call options including isCustomAccessibility flag
     * @param token[out], generated token for the call
     * @return Returns 0 on success, others on failure.
     */
    int32_t MakeCallWithToken(std::string number, AppExecFwk::PacMap &options, std::string &token);

    /**
     * @brief Answer a phone call
     *
     * @param callId[in], call id
     * @param videoState[in], 0: audio, 1: video
     * @param isRTT[in], 0: No RTT, 1: Ims RTT Call
     * @return Returns 0 on success, others on failure.
     */
    int32_t AnswerCall(int32_t callId, int32_t videoState, bool isRTT = false);

    /**
     * @brief Reject a phone call
     *
     * @param callId[in], call id
     * @param rejectWithMessage[in], Whether to enter the reason for rejection,true:yes false:no
     * @param textMessage[in], The reason you reject the call
     * @return Returns 0 on success, others on failure.
     */
    int32_t RejectCall(int32_t callId, bool isSendSms, std::u16string content);

    /**
     * @brief Hang up the phone
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t HangUpCall(int32_t callId);

    /**
     * @brief Obtain the call status of the device
     *
     * @return Returns call state.
     */
    int32_t GetCallState();

    /**
     * @brief Park a phone call
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t HoldCall(int32_t callId);

    /**
     * @brief Activate a phone call
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnHoldCall(int32_t callId);

    /**
     * @brief Switch the phone call between hold and unhold
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t SwitchCall(int32_t callId);

    /**
     * @brief Merge calls to form a conference
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t CombineConference(int32_t callId);

    /**
     * @brief Separates a specified call from a conference call
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t SeparateConference(int32_t callId);

    /**
     * @brief Hangup a specified call from a conference call
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t KickOutFromConference(int32_t callId);

    /**
     * @brief Obtain the ID of the primary call in a conference
     *
     * @param callId[in], Id of a call in a conference
     * @return Returns main call id, -1 on not call id.
     */
    int32_t GetMainCallId(int32_t &callId, int32_t &mainCallId);

    /**
     * @brief Obtain the list of neutron call ids
     *
     * @param callId[in], Id of a call in a conference
     * @param callIdList[out], the list of neutron call ids
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetSubCallIdList(int32_t callId, std::vector<std::u16string> &callIdList);

    /**
     * @brief Obtain the callId list of all calls in a conference
     *
     * @param callId[in], Id of a call in a conference
     * @param callIdList[out], the callId list of all calls in a conference
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallIdListForConference(int32_t callId, std::vector<std::u16string> &callIdList);

    /**
     * @brief Gets whether the call waiting service of the current account is enabled
     *
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallWaiting(int32_t slotId);

    /**
     * @brief Set the call waiting function for the current account
     *
     * @param slotId[in], The slot id
     * @param activate[in], Activation of switch
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallWaiting(int32_t slotId, bool activate);

    /**
     * @brief Gets the call restriction information of the specified account
     *
     * @param slotId[in], The slot id
     * @param type[in], Call Restriction type
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallRestriction(int32_t slotId, CallRestrictionType type);

    /**
     * @brief Set the call restriction function for the current account
     *
     * @param slotId[in], The slot id
     * @param info[in], Call restriction information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallRestriction(int32_t slotId, CallRestrictionInfo &info);

    /**
     * @brief Set the call restriction password function for the current account
     *
     * @param slotId[in], The slot id
     * @param fac[in], Call restriction type
     * @param oldPassword[in], Old password of call restriction type
     * @param newPassword[in], New password of call restriction type
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallRestrictionPassword(
        int32_t slotId, CallRestrictionType fac, const char *oldPassword, const char *newPassword);

    /**
     * @brief Gets the call transfer information of the current account
     *
     * @param slotId[in], The slot id
     * @param type[in], Call Transfer Type
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallTransferInfo(int32_t slotId, CallTransferType type);

    /**
     * @brief Set the call transfer function for the current account
     *
     * @param slotId[in], The slot id
     * @param info[in], Call Transfer Information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallTransferInfo(int32_t slotId, CallTransferInfo &info);

    /**
     * @brief confirm whether IMS can set call transfer time.
     *
     * @param slotId[in], The slot id
     * @param result[out], The result of can set or not
     * @return Returns true on can set, others on can not set.
     */
    int32_t CanSetCallTransferTime(int32_t slotId, bool &result);

    /**
     * @brief Setting the Call Type
     *
     * @param slotId[in], The slot id
     * @param mode[in], Preference Mode
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallPreferenceMode(int32_t slotId, int32_t mode);

    /**
     * @brief Enable and send DTMF
     *
     * @param callId[in], call id
     * @param str[in], Characters sent
     * @return Returns 0 on success, others on failure.
     */
    int32_t StartDtmf(int32_t callId, char str);

    /**
     * @brief Stop the DTMF
     *
     * @param callId[in], call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t StopDtmf(int32_t callId);

    int32_t PostDialProceed(int32_t callId, bool proceed);
    /**
     * @brief Whether the ringing
     *
     * @param enabled[out], true on ringing, false on there is no ringing
     * @return Returns interface processing results.
     */
    int32_t IsRinging(bool &enabled);

    /**
     * @brief Is there Call
     *
     * @param isInCludeVoipCall[in], include voip call or not
     * @return Returns true on has call, others on there is no call.
     */
    bool HasCall(const bool isInCludeVoipCall = true);

    /**
     * @brief Can I initiate a call
     *
     * @param enabled[out], whether allow new calls
     * @return Returns interface processing results.
     */
    int32_t IsNewCallAllowed(bool &enabled);

    /**
     * @brief Is there an emergency call
     *
     * @param enabled[out], true on emergency call, false on no emergency call
     * @return Returns interface processing results.
     */
    int32_t IsInEmergencyCall(bool &enabled);

    /**
     * @brief Is it an emergency call
     *
     * @param number[in], Phone number to be formatted
     * @param slotId[in], The slot id
     * @param enabled[out] result of is it an emergency call
     * @return Returns 0 on success, others on failure.
     */
    int32_t IsEmergencyPhoneNumber(std::u16string &number, int32_t slotId, bool &enabled);

    /**
     * @brief Formatting a phone number
     *
     * @param number[in], Phone number to be formatted
     * @param countryCode[in], Country code of the phone number
     * @param formatNumber[out] Formatting a phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t FormatPhoneNumber(std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);

    /**
     * @brief Formatting a phone number
     *
     * @param number[in]. Phone number to be formatted
     * @param countryCode[in], Country code of the phone number
     * @param formatNumber[out] Formatting a phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t FormatPhoneNumberToE164(
        std::u16string &number, std::u16string &countryCode, std::u16string &formatNumber);

    /**
     * @brief Mute the Microphone
     *
     * @param isMute[in], mute state
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetMuted(bool isMute);

    /**
     * @brief Call mute
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t MuteRinger();

    /**
     * @brief Setting the Audio Channel
     *
     * @param audioDevice[in], contain audioDeviceType and address
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetAudioDevice(const AudioDevice &audioDevice);

    /**
     * @brief Open or close camera
     *
     * @param callId[in], The call id
     * @param cameraId[in], The camera id
     * @return Returns 0 on success, others on failure.
     */
    int32_t ControlCamera(int32_t callId, std::u16string cameraId);

    /**
     * @brief Set the location and size of the preview window for videos captured by the local camera.
     *
     * @param callId[in], The call id
     * @param surfaceId[in], Window information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetPreviewWindow(int32_t callId, std::string &surfaceId);

    /**
     * @brief Sets the location and size of the remote video window.
     *
     * @param callId[in], The call id
     * @param surfaceId[in], Window information
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetDisplayWindow(int32_t callId, std::string &surfaceId);

    /**
     * @brief Sets the local camera zoom scale
     *
     * @param zoomRatio[in], Camera scale
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCameraZoom(float zoomRatio);

    /**
     * @brief APP sets the screen of the remote video freeze immediately.
     * If the APP does not call this interface when making a video call,
     * the last frame before the remote video freeze is displayed by default
     *
     * @param callId[in], The call id
     * @param path[in], Local Picture address
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetPausePicture(int32_t callId, std::u16string path);

    /**
     * @brief Set the rotation Angle of the local device. The default value is 0
     *
     * @param rotation[in], Rotation Angle
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetDeviceDirection(int32_t callId, int32_t rotation);

    /**
     * @brief Obtain the IMS service configuration
     *
     * @param slotId[in], The slot id
     * @param item[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetImsConfig(int32_t slotId, ImsConfigItem item);

    /**
     * @brief Example Set the IMS service configuration
     *
     * @param slotId[in], The slot id
     * @param item[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetImsConfig(int32_t slotId, ImsConfigItem item, std::u16string &value);

    /**
     * @brief Gets the value of the IMS function item of the specified network type
     *
     * @param slotId[in], The slot id
     * @param info[in], FeatureType
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetImsFeatureValue(int32_t slotId, FeatureType type);

    /**
     * @brief Set the value of the IMS function item of the specified network type
     *
     * @param slotId[in], The slot id
     * @param info[in], FeatureType
     * @param value[in]
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetImsFeatureValue(int32_t slotId, FeatureType type, int32_t value);

    /**
     * @brief Setting the Call Mode
     *
     * @param callId[in], The call id
     * @param mode[in], Calling patterns
     * @return Returns 0 on success, others on failure.
     */
    int32_t UpdateImsCallMode(int32_t callId, ImsCallMode mode);

    /**
     * @brief Start VoLte
     *
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t EnableImsSwitch(int32_t slotId);

    /**
     * @brief Stop VoLte
     *
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t DisableImsSwitch(int32_t slotId);

    /**
     * @brief Whether to enable VoLte
     *
     * @param slotId[in], The slot id
     * @param enabled[out], The result of enable or not
     * @return Returns 0 on success, others on failure.
     */
    int32_t IsImsSwitchEnabled(int32_t slotId, bool &enabled);

    /**
     * @brief Set VoNR Switch Status
     *
     * @param state[in] Indicates the VoNR state, 0: off, 1: on
     * @param slotId[in] the slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoNRState(int32_t slotId, int32_t state);

    /**
     * @brief Get VoNR Switch Status
     *
     * @param slotId[in] the slot id
     * @param state[out] Indicates the VoNR state, 0: off, 1: on
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoNRState(int32_t slotId, int32_t &state);

    /**
     * @brief Bring someone into a meeting
     *
     * @param callId[in], The call id
     * @param numberList[in], List of calls to join the conference
     * @return Returns 0 on success, others on failure.
     */
    int32_t JoinConference(int32_t callId, std::vector<std::u16string> &numberList);

    /**
     * @brief report ott call details info
     *
     * @param ottVec[in], ott call status detail info list
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportOttCallDetailsInfo(std::vector<OttCallDetailsInfo> &ottVec);

    /**
     * @brief report ott call event info
     *
     * @param eventInfo[in], ott call event detail info
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportOttCallEventInfo(OttCallEventInfo &eventInfo);

    /**
     * @brief Close Unfinished ussd function for the current account
     *
     * @param slotId[in], The slot id
     * @return Returns 0 on success, others on failure.
     */
    int32_t CloseUnFinishedUssd(int32_t slotId);

    /**
     * @brief Handle special code from dialer.
     *
     * @param specialCode[in], special code
     * @return Returns 0 on success, others on failure.
     */
    int32_t InputDialerSpecialCode(const std::string &specialCode);

    /**
     * @brief Remove missed incoming call notification.
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t RemoveMissedIncomingCallNotification();

    /**
     * @brief Set VoIP Call state
     *
     * @param state[in] Indicates the VoIP Call state
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoIPCallState(int32_t state);

    /**
     * @brief Get VoIP Call Switch Status
     *
     * @param state[out] Indicates the VoIP Call state
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoIPCallState(int32_t &state);

    /**
     * @brief Set VoIP Call info
     *
     * @param callId[in] Indicates the VoIP Call id
     * @param state[in] Indicates the VoIP Call state
     * @param phoneNumber[in] Indicates the VoIP Call phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetVoIPCallInfo(int32_t callId, int32_t state, std::string phoneNumber);

    /**
     * @brief Get VoIP Call info
     *
     * @param callId[out] Indicates the VoIP Call id
     * @param state[out] Indicates the VoIP Call state
     * @param phoneNumber[out] Indicates the VoIP Call phone number
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetVoIPCallInfo(int32_t &callId, int32_t &state, std::string &phoneNumber);

    /**
     * @brief Checks whether a device supports voice calls
     *
     * @return true on support voice calls, false on not support.
     */
    bool HasVoiceCapability();

    /**
     * @brief set register mmi code callback state
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetRegMmiCodeCallbackState(bool isReg);

    /**
     * @brief report audio device info
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t ReportAudioDeviceInfo();

    /**
     * @brief cancel upgrade to video call
     *
     * @param callId[in], The call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t CancelCallUpgrade(int32_t callId);

    /**
     * @brief request camera capabilities
     *
     * @param callId[in], The call id
     * @return Returns 0 on success, others on failure.
     */
    int32_t RequestCameraCapabilities(int32_t callId);

    /**
     * @brief notify voip register callstatus callback
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t RegisterVoipCallManagerCallback();

    /**
     * @brief notify voip unregister callstatus callback
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnRegisterVoipCallManagerCallback();

    /**
     * @brief preload call ui
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t PreloadCallUi(bool enable);

    /**
     * @brief send call ui event
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t SendCallUiEvent(int32_t callId, std::string &eventName);

    /**
     * @brief notify bluetoothcall register callstatus callback
     *
     * @return Returns sptr<ICallStatusCallback>.
     */
    sptr<ICallStatusCallback> RegisterBluetoothCallManagerCallbackPtr(std::string &macAddress);

    /**
     * @brief notify primary or secondary phone register callstatus callback
     *
     * @return Returns sptr<ICallStatusCallback>.
     */
    sptr<ICallStatusCallback> RegisterTwoPhoneCallManagerCallbackPtr(int32_t phoneIndex, std::string &macAddress);

    /**
     * @brief send ussd response to modem
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t SendUssdResponse(int32_t slotId, std::string &content);

    /**
     * @brief set telephony call trust/block list policy
     * @param isDialingTrustlist[in], dialing policy flag, false is block, true is trust.
     * @param dialingList[in], dialing trust/block number list.
     * @param isIncomingTrustlist[in], incoming policy flag, false is block, true is trust.
     * @param incomingList[in], incoming trust/block number list.
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallPolicyInfo(bool isDialingTrustlist, const std::vector<std::string> &dialingList,
        bool isIncomingTrustlist, const std::vector<std::string> &incomingList);

    /**
     * @brief endCall for callManager
     *
     * @return Returns true on success, others on failure.
     */
    bool EndCall();

    /**
     * @brief notify voip audio stream start
     *
     * @param uid[in], uid
     * @return Returns 0 on success, others on failure.
     */
    int32_t NotifyVoIPAudioStreamStart(int32_t uid);

#ifdef SUPPORT_RTT_CALL
    /**
     * @brief Send RTT text message.
     *
     * @param callId[in], The call id
     * @param rttMessage[in], Send RTT message.
     * @return Returns 0 on success, others on failure.
     */
    int32_t SendRttMessage(int32_t callId, const std::string &rttMessage);

    /**
     * @brief RTT Call Switch Settings
     *
     * @param slotId[in], The slot id
     * @param isEnable[out], The result of enable or not
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetRttCapability(int32_t slotId, bool isEnable);

    /**
     * @brief rtt upgrade or downgrade
     * @param callId[in], call id
     * @param mode[in], rtt modify type
     * @return Returns 0 on success, others on failure.
     */
    int32_t UpdateImsRttCallMode(int32_t callId, ImsRTTCallMode mode);
#endif

    /**
     * @brief Set Call Audio Mode
     *
     * @param slotId[in], The mode
     * @param slotId[in], The scenarios
     * @return Returns 0 on success, others on failure.
     */
    int32_t SetCallAudioMode(int32_t mode, int32_t scenarios);

    /**
     * @brief Answer a phone call by callManager
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t AnswerCall();

    /**
     * @brief Reject a phone call by callManager
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t RejectCall();

    /**
     * @brief Reject a phone call by reject type
     * @param rejectType[in], the reject type
     * @return Returns 0 on success, others on failure.
     */
    int32_t RejectCall(RejectType rejectType);

    /**
     * @brief Hang up the phone by callManager
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t HangUpCall();

    /**
     * @brief Gets the call transfer information of the current phone number
     *
     * @param number[in], The phone number which sim card get call transfer
     * @param type[in], Call Transfer Type
     * @return Returns 0 on success, others on failure.
     */
    int32_t GetCallTransferInfo(const std::string number, CallTransferType type);
    /**
     * @brief Register transfer controller callback
     *
     * @param transferControl[in], callback function pointer
     * @return Returns 0 on success, others on failure.
     */
    int32_t RegisterTransferController(std::unique_ptr<TransferControl> transferControl);

    /**
     * @brief unregister transfer call callback
     *
     * @return Returns 0 on success, others on failure.
     */
    int32_t UnRegisterTransferController();
    /**
     * @brief Notify transfer call contact name
     *
     * @param contactName[in], contact name for transfer call
     * @return Returns 0 on success, others on failure.
     */
    int32_t NotifyTransferCallContact(const std::string contactName);
    /**
     * @brief Check if recording permission is allowed for the call
     *
     * @param cellularRecordPhoneNum[in], phone number of the call to check
     * @param cellularRecordToken[in], token of the call to verify
     * @return Returns true if permission is allowed, false otherwise.
     */
    bool CheckCallRecordingPermission(const std::string& cellularRecordPhoneNum,
        const std::string& cellularRecordToken);

private:
    std::atomic<bool> isInit_ = false;
};
} // namespace Telephony
} // namespace OHOS

#endif
