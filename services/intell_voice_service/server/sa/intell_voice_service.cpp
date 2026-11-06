/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "intell_voice_service.h"

#include <malloc.h>
#include <fcntl.h>
#include <cstdio>
#include <unistd.h>
#include "intell_voice_log.h"
#include "ipc_skeleton.h"
#include "system_ability_definition.h"
#include "intell_voice_service_manager.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "intell_voice_util.h"
#include "intell_voice_info.h"
#include "wakeup_track.h"

#define LOG_TAG "IntellVoiceService"

using namespace std;
using namespace OHOS::IntellVoice;
using namespace OHOS::AppExecFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
inline bool Between(IntellVoiceEngineType val, IntellVoiceEngineType min, IntellVoiceEngineType max)
{
    return (val >= min) && (val <= max);
}

REGISTER_SYSTEM_ABILITY_BY_ID(IntellVoiceService, INTELL_VOICE_SERVICE_ID, true);
const std::string OHOS_PERMISSION_INTELL_VOICE = "ohos.permission.MANAGE_INTELLIGENT_VOICE";

IntellVoiceService::IntellVoiceService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(INTELL_VOICE_SERVICE_ID, true)
{
    systemAbilityChangeMap_[COMMON_EVENT_SERVICE_ID] = [this](bool isAdded) {
        this->OnCommonEventServiceChange(isAdded);
    };
    systemAbilityChangeMap_[DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID] = [this](bool isAdded) {
        this->OnDistributedKvDataServiceChange(isAdded);
    };
}

IntellVoiceService::~IntellVoiceService()
{
}

int32_t IntellVoiceService::CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst)
{
    INTELL_VOICE_LOG_INFO("enter, type: %{public}d", type);
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    inst = ServiceManagerType::GetInstance().HandleCreateEngine(type);
    if (inst == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return INTELLIGENT_VOICE_NO_MEMORY;
    }
    INTELL_VOICE_LOG_INFO("create engine ok");
    return 0;
}

int32_t IntellVoiceService::ReleaseIntellVoiceEngine(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter, type: %{public}d", type);
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }
    if (type == IntellVoiceEngineType::INTELL_VOICE_DEVICE) {
        INTELL_VOICE_LOG_INFO("do not release");
        return INTELLIGENT_VOICE_SUCCESS;
    }

    return ServiceManagerType::GetInstance().HandleReleaseEngine(type);
}

void IntellVoiceService::OnStart(const SystemAbilityOnDemandReason &startReason)
{
    reasonId_ = static_cast<int32_t>(startReason.GetId());
    reasonName_ = startReason.GetName();
    reasonValue_ = startReason.GetValue();
    INTELL_VOICE_LOG_INFO("enter, reason id:%{public}d, reasonName:%{public}s, reasonValue:%{public}s",
        reasonId_, reasonName_.c_str(), reasonValue_.c_str());
    ServiceManagerType::GetInstance().OnServiceStart(systemAbilityChangeMap_);

    bool ret = Publish(this);
    if (!ret) {
        INTELL_VOICE_LOG_ERROR("publish failed!");
        ServiceManagerType::GetInstance().OnServiceStop();
        return;
    }
    CreateSystemEventObserver();
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    AddSystemAbilityListener(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    AddSystemAbilityListener(TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID);
    AddSystemAbilityListener(AUDIO_DISTRIBUTED_SERVICE_ID);
    AddSystemAbilityListener(AUDIO_POLICY_SERVICE_ID);
#ifdef POWER_MANAGER_ENABLE
    AddSystemAbilityListener(POWER_MANAGER_SERVICE_ID);
#endif
    AddSystemAbilityListener(DISPLAY_MANAGER_SERVICE_ID);
    RegisterPermissionCallback(OHOS_PERMISSION_INTELL_VOICE);
    INTELL_VOICE_LOG_INFO("publish ok");
}

void IntellVoiceService::OnStop(void)
{
    INTELL_VOICE_LOG_INFO("enter");

    auto &manager = ServiceManagerType::GetInstance();
    manager.HandleServiceStop();
    manager.ReleaseSwitchProvider();
    manager.OnServiceStop();

    if (systemEventObserver_ != nullptr) {
        systemEventObserver_->Unsubscribe();
    }
}

int32_t IntellVoiceService::OnIdle(const SystemAbilityOnDemandReason &idleReason)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!ServiceManagerType::GetInstance().HandleOnIdle()) {
        INTELL_VOICE_LOG_INFO("reject to unload service");
        return -1;
    }

    return 0;
}

/**
 * @brief 服务重新激活回调函数
 * @param activeReason 激活原因，包含事件ID、名称和值
 * @description 在服务从idle状态被重新激活时调用（CancelIdle），
 *              介于OnIdle和OnStop之间。如果被特定系统事件重新激活，
 *              会尝试再次卸载服务。
 * @note 可能会修改类的成员变量 reasonId_, reasonName_, reasonValue_
 */
void IntellVoiceService::OnActive(const SystemAbilityOnDemandReason& activeReason)
{
    // if reactivated by system event, unload again
    reasonId_ = static_cast<int32_t>(activeReason.GetId());
    reasonName_ = activeReason.GetName();
    reasonValue_ = activeReason.GetValue();
    INTELL_VOICE_LOG_INFO("enter, reason id:%{public}d, reasonName:%{public}s, reasonValue:%{public}s",
        reasonId_, reasonName_.empty() ? "" : reasonName_.c_str(), reasonValue_.empty() ? "" : reasonValue_.c_str());
    auto &manager = ServiceManagerType::GetInstance();
    manager.OnServiceActive();
    if (reasonId_ == static_cast<int32_t>(OHOS::OnDemandReasonId::COMMON_EVENT) &&
        (reasonName_ == std::string("usual.event.BOOT_COMPLETED") ||
        reasonName_ == std::string("usual.event.POWER_SAVE_MODE_CHANGED"))) {
        INTELL_VOICE_LOG_INFO("try to unload again");
        manager.HandleUnloadIntellVoiceService(true);
    }
}

void IntellVoiceService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    INTELL_VOICE_LOG_INFO("add systemAbilityId:%{public}d", systemAbilityId);
    auto iter = systemAbilityChangeMap_.find(systemAbilityId);
    if (iter == systemAbilityChangeMap_.end()) {
        INTELL_VOICE_LOG_WARN("unhandled sysabilityId");
        return;
    }

    if (iter->second == nullptr) {
        INTELL_VOICE_LOG_WARN("func is nullptr");
        return;
    }

    iter->second(true);
}

void IntellVoiceService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    if (!NeedProcOnRemoved(systemAbilityId)) {
        return;
    }
    auto iter = systemAbilityChangeMap_.find(systemAbilityId);
    if (iter == systemAbilityChangeMap_.end()) {
        INTELL_VOICE_LOG_WARN("unhandled sysabilityId");
        return;
    }

    if (iter->second == nullptr) {
        INTELL_VOICE_LOG_WARN("func is nullptr");
        return;
    }

    iter->second(false);
}

bool IntellVoiceService::NeedProcOnRemoved(int32_t systemAbilityId)
{
    if (systemAbilityId == DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID) {
        return true;
    }
    return false;
}

void IntellVoiceService::CreateSystemEventObserver()
{
    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_DATA_CLEARED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REPLACED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    systemEventObserver_ = SystemEventObserver::Create(subscribeInfo,
        std::bind(&IntellVoiceService::OnReceiveCes, this, std::placeholders::_1));
    if (systemEventObserver_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("systemEventObserver_ is nullptr");
        return;
    }
    INTELL_VOICE_LOG_INFO("create system event observer successfully");
}

void IntellVoiceService::RegisterPermissionCallback(const std::string &permissionName)
{
    INTELL_VOICE_LOG_INFO("enter");
    Security::AccessToken::PermStateChangeScope scopeInfo;
    scopeInfo.permList = {permissionName};
    auto callbackPtr = std::make_shared<PerStateChangeCbCustomizeCallback>(scopeInfo);
    int32_t res = Security::AccessToken::AccessTokenKit::RegisterPermStateChangeCallback(callbackPtr);
    if (res < 0) {
        INTELL_VOICE_LOG_ERROR("fail to call RegisterPermStateChangeCallback.");
    }
}

void IntellVoiceService::PerStateChangeCbCustomizeCallback::PermStateChangeCallback(
    Security::AccessToken::PermStateChangeInfo &result)
{
    INTELL_VOICE_LOG_INFO("enter, permStateChangeType: %{public}d", result.permStateChangeType);
    if (result.permStateChangeType == 0) {
        INTELL_VOICE_LOG_ERROR("The permission is canceled.");
    }
}

void IntellVoiceService::InitIntellVoiceService()
{
    INTELL_VOICE_LOG_INFO("enter");
    auto &manager = ServiceManagerType::GetInstance();
    manager.CreateSwitchProvider();
    manager.ProcBreathModel();
    manager.ProcSingleLevelModel();
    manager.HandleSilenceUpdate();

    if (reasonId_ == static_cast<int32_t>(OHOS::OnDemandReasonId::COMMON_EVENT)) {
        INTELL_VOICE_LOG_INFO("common event start");
        manager.HandleSwitchOn(true, VOICE_WAKEUP_MODEL_UUID, false);
        manager.HandleSwitchOn(true, PROXIMAL_WAKEUP_MODEL_UUID, false);
        if (reasonName_ == std::string("usual.event.POWER_SAVE_MODE_CHANGED")) {
            INTELL_VOICE_LOG_INFO("power save mode change");
            manager.HandlePowerSaveModeChange();
        } else {
            INTELL_VOICE_LOG_INFO("power on");
            manager.HandleUnloadIntellVoiceService(true);
        }
    } else if (reasonId_ == static_cast<int32_t>(OHOS::OnDemandReasonId::INTERFACE_CALL)) {
        INTELL_VOICE_LOG_INFO("interface call start");
        manager.HandleSwitchOn(true, VOICE_WAKEUP_MODEL_UUID, false);
        manager.HandleSwitchOn(true, PROXIMAL_WAKEUP_MODEL_UUID, false);
    } else {
        INTELL_VOICE_LOG_INFO("no need to process, reason id:%{public}d", reasonId_);
    }
    IntellVoiceUtil::StartAbility("power_on");
    reasonId_ = -1;
}

void IntellVoiceService::OnReceiveCes(const OHOS::EventFwk::CommonEventData &data)
{
    const OHOS::AAFwk::Want &want = data.GetWant();
    std::string action = want.GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_DATA_SHARE_READY) {
        std::lock_guard<std::mutex> lock(initServiceMutex_);
        if (!isServiceInit_) {
            INTELL_VOICE_LOG_INFO("receive COMMON_EVENT_DATA_SHARE_READY, init intell voice service");
            if (isDataShareRemoved_) {
                reasonId_ = static_cast<int32_t>(OHOS::OnDemandReasonId::INTERFACE_CALL);
            }
            InitIntellVoiceService();
            isServiceInit_ = true;
            isDataShareRemoved_ = false;
        }
    }
}

void IntellVoiceService::OnCommonEventServiceChange(bool isAdded)
{
    if (isAdded) {
        INTELL_VOICE_LOG_INFO("comment event service is added");
        if (systemEventObserver_ != nullptr) {
            systemEventObserver_->Subscribe();
        }
    } else {
        INTELL_VOICE_LOG_INFO("comment event service is removed");
    }
}

void IntellVoiceService::OnDistributedKvDataServiceChange(bool isAdded)
{
    std::lock_guard<std::mutex> lock(initServiceMutex_);
    if (isAdded) {
        INTELL_VOICE_LOG_INFO("distributed kv data service is added");
        if (isServiceInit_) {
            INTELL_VOICE_LOG_INFO("already init intell voice service");
            return;
        }
        if (!SwitchProvider::CheckIfDataShareReady()) {
            INTELL_VOICE_LOG_WARN("data share is not ready");
            return;
        }
        INTELL_VOICE_LOG_INFO("data service change, init intell voice service");
        if (isDataShareRemoved_) {
            reasonId_ = static_cast<int32_t>(OHOS::OnDemandReasonId::INTERFACE_CALL);
        }
        InitIntellVoiceService();
        isServiceInit_ = true;
        isDataShareRemoved_ = false;
    } else {
        INTELL_VOICE_LOG_INFO("distributed kv data service is removed");
        isServiceInit_ = false;
        isDataShareRemoved_ = true;
    }
}

bool IntellVoiceService::RegisterDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object)
{
    if (IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE) != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return false;
    }

    if (object == nullptr) {
        INTELL_VOICE_LOG_WARN("object is nullptr, no need to register");
        return false;
    }

    if (!(Between(type, INTELL_VOICE_ENROLL, INTELL_VOICE_WAKEUP)) &&
        !(Between(type, INTELL_VOICE_HEADSET_WAKEUP, INTELL_VOICE_HEADSET_WAKEUP))) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return false;
    }

    return ServiceManagerType::GetInstance().RegisterProxyDeathRecipient(type, object);
}

bool IntellVoiceService::DeregisterDeathRecipient(IntellVoiceEngineType type)
{
    if (IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE) != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return false;
    }

    if (!(Between(type, INTELL_VOICE_ENROLL, INTELL_VOICE_WAKEUP)) &&
        !(Between(type, INTELL_VOICE_HEADSET_WAKEUP, INTELL_VOICE_HEADSET_WAKEUP))) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return false;
    }

    return ServiceManagerType::GetInstance().DeregisterProxyDeathRecipient(type);
}

bool IntellVoiceService::RegisterDeviceDeathRecipient(const std::string &serviceName,
    const sptr<IRemoteObject> &object)
{
    if (IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE) != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return false;
    }
 
    if (serviceName.empty() || object == nullptr) {
        INTELL_VOICE_LOG_WARN("serviceName is empty or object is nullptr, no need to register");
        return false;
    }
 
    return ServiceManagerType::GetInstance().RegisterDeviceDeathRecipient(serviceName, object);
}

int32_t IntellVoiceService::GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    return ServiceManagerType::GetInstance().GetUploadFiles(numMax, files);
}

std::string IntellVoiceService::GetParameter(const std::string &key)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return "";
    }

    return ServiceManagerType::GetInstance().EngineGetParameter(key);
}

int32_t IntellVoiceService::SetParameter(const std::string &keyValueList)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    return ServiceManagerType::GetInstance().EngineSetParameter(keyValueList);
}

int32_t IntellVoiceService::GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    return ServiceManagerType::GetInstance().GetWakeupSourceFilesList(cloneFiles);
}

int32_t IntellVoiceService::GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    INTELL_VOICE_LOG_INFO("get wakeup source file");
    return ServiceManagerType::GetInstance().GetWakeupSourceFile(filePath, buffer);
}

int32_t IntellVoiceService::SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    INTELL_VOICE_LOG_INFO("send wakeup file");
    return ServiceManagerType::GetInstance().SendWakeupFile(filePath, buffer);
}

int32_t IntellVoiceService::EnrollWithWakeupFilesForResult(const std::string &wakeupInfo,
    const sptr<IRemoteObject> object)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    INTELL_VOICE_LOG_INFO("enter");
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is nullptr");
        return -1;
    }

    return ServiceManagerType::GetInstance().HandleCloneUpdate(wakeupInfo, object);
}

int32_t IntellVoiceService::ClearUserData()
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }

    return ServiceManagerType::GetInstance().ClearUserData();
}

int32_t IntellVoiceService::RegisterDeviceWakeupInfo(const std::string &macAddress, const WakeupDeviceInfo &info)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }
 
    return ServiceManagerType::GetInstance().RegisterWakeupDevice(macAddress, info);
}
 
int32_t IntellVoiceService::DeregisterDeviceWakeupInfo(const std::string &macAddress)
{
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_PERMISSION_INTELL_VOICE);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        INTELL_VOICE_LOG_WARN("verify permission denied");
        return ret;
    }
 
    return ServiceManagerType::GetInstance().DeregisterWakeupDevice(macAddress);
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS
