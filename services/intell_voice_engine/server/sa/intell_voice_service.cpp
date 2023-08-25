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
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "tokenid_kit.h"
#include "idevmgr_hdi.h"
#include "audio_system_manager.h"
#include "intell_voice_log.h"
#include "system_ability_definition.h"
#include "intell_voice_service_manager.h"
#include "common_event_manager.h"
#include "common_event_support.h"

#define LOG_TAG "IntellVoiceService"

using namespace std;
using namespace OHOS::AppExecFwk;
using namespace OHOS::EventFwk;
using namespace OHOS::AudioStandard;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;

namespace OHOS {
namespace IntellVoiceEngine {
REGISTER_SYSTEM_ABILITY_BY_ID(IntellVoiceService, INTELL_VOICE_SERVICE_ID, true);
const std::string OHOS_PERMISSION_INTELL_VOICE = "ohos.permission.MANAGE_INTELLIGENT_VOICE";

IntellVoiceService::IntellVoiceService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(INTELL_VOICE_SERVICE_ID, true)
{
    systemAbilityChangeMap_[COMMON_EVENT_SERVICE_ID] =
        [this](bool isAdded) { this->OnCommonEventServiceChange(isAdded); };
    systemAbilityChangeMap_[DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID] =
        [this](bool isAdded) { this->OnDistributedKvDataServiceChange(isAdded); };
    systemAbilityChangeMap_[TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID] =
        [this](bool isAdded) { this->OnTelephonyStateRegistryServiceChange(isAdded); };
    systemAbilityChangeMap_[AUDIO_DISTRIBUTED_SERVICE_ID] =
        [this](bool isAdded) { this->OnAudioDistributedServiceChange(isAdded); };
}

IntellVoiceService::~IntellVoiceService()
{
}

int32_t IntellVoiceService::CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst)
{
    if (!CheckIsSystemApp()) {
        INTELL_VOICE_LOG_WARN("not system app");
        return -1;
    }
    if (!VerifyClientPermission(OHOS_PERMISSION_INTELL_VOICE)) {
        INTELL_VOICE_LOG_WARN("verify permission");
    }

    INTELL_VOICE_LOG_INFO("enter, type: %{public}d", type);
    std::unique_ptr<IntellVoiceServiceManager> &mgr = IntellVoiceServiceManager::GetInstance();
    if (mgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("mgr is nullptr");
        return -1;
    }

    inst = mgr->CreateEngine(type);
    if (inst == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return -1;
    }
    INTELL_VOICE_LOG_INFO("create engine ok");
    return 0;
}

int32_t IntellVoiceService::ReleaseIntellVoiceEngine(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter, type: %{public}d", type);
    std::unique_ptr<IntellVoiceServiceManager> &mgr = IntellVoiceServiceManager::GetInstance();
    if (mgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("mgr is nullptr");
        return -1;
    }
    return mgr->ReleaseEngine(type);
}

void IntellVoiceService::OnStart(const SystemAbilityOnDemandReason& startReason)
{
    INTELL_VOICE_LOG_ERROR("enter, reason id:%{public}d", startReason.GetId());
    reasonId_ = static_cast<int32_t>(startReason.GetId());
    LoadIntellVoiceHost();

    bool ret = Publish(this);
    if (!ret) {
        INTELL_VOICE_LOG_ERROR("publish failed!");
        UnloadIntellVoiceHost();
        return;
    }
    CreateSystemEventObserver();
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    AddSystemAbilityListener(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    AddSystemAbilityListener(TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID);
    AddSystemAbilityListener(AUDIO_DISTRIBUTED_SERVICE_ID);
    RegisterPermissionCallback(OHOS_PERMISSION_INTELL_VOICE);
    INTELL_VOICE_LOG_ERROR("publish ok");
}

void IntellVoiceService::OnStop(void)
{
    INTELL_VOICE_LOG_INFO("enter");

    auto triggerMgr = IntellVoiceTrigger::TriggerManager::GetInstance();
    if (triggerMgr != nullptr) {
        triggerMgr->DettachTelephonyObserver();
        triggerMgr->DettachAudioCaptureListener();
    }

    const auto &manager = IntellVoiceServiceManager::GetInstance();
    if (manager != nullptr) {
        manager->ReleaseSwitchProvider();
    }

    if (systemEventObserver_ != nullptr) {
        systemEventObserver_->Unsubscribe();
    }

    UnloadIntellVoiceHost();
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
}

void IntellVoiceService::CreateSystemEventObserver()
{
    std::shared_ptr<EventRunner> runner = EventRunner::Create("service");
    if (runner == nullptr) {
        INTELL_VOICE_LOG_ERROR("runner is null");
        return;
    }

    std::shared_ptr<EventHandler> handler = std::make_shared<EventHandler>(runner);
    if (handler == nullptr) {
        INTELL_VOICE_LOG_ERROR("handler is null");
        return;
    }

    OHOS::EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_DATA_CLEARED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REPLACED);
    matchingSkills.AddEvent(OHOS::EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    OHOS::EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    systemEventObserver_ = SystemEventObserver::Create(subscribeInfo);
    if (systemEventObserver_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("systemEventObserver_ is nullptr");
        return;
    }
    systemEventObserver_->SetEventHandler(handler);
    INTELL_VOICE_LOG_INFO("create system event observer successfully");
}

static void print_to_file(void *fp, const char *s)
{
    (void)fputs(s, static_cast<FILE *>(fp));
}

int IntellVoiceService::Dump(int fd, const std::vector<std::u16string> &args)
{
    FILE *fp = fdopen(fd, "w+");
    if (fp != nullptr) {
        malloc_stats_print(print_to_file, fp, "");
        fp = nullptr;
    }
    return 0;
}

bool IntellVoiceService::VerifyClientPermission(const std::string &permissionName)
{
    Security::AccessToken::AccessTokenID clientTokenId = IPCSkeleton::GetCallingTokenID();
    INTELL_VOICE_LOG_INFO("clientTokenId:%{public}d", clientTokenId);
    int res = Security::AccessToken::AccessTokenKit::VerifyAccessToken(clientTokenId, permissionName);
    if (res != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
        INTELL_VOICE_LOG_ERROR("Permission denied!");
        return false;
    }
    return true;
}

bool IntellVoiceService::CheckIsSystemApp()
{
    uint64_t fullTokenId = IPCSkeleton::GetCallingFullTokenID();
    if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId)) {
        INTELL_VOICE_LOG_INFO("Not system app, permission reject tokenid: %{public}lu", fullTokenId);
        return false;
    }

    INTELL_VOICE_LOG_INFO("System app, fullTokenId:%{public}lu", fullTokenId);
    return true;
}

void IntellVoiceService::LoadIntellVoiceHost()
{
    auto devmgr = IDeviceManager::Get();
    if (devmgr != nullptr) {
        INTELL_VOICE_LOG_INFO("Get devmgr success");
        devmgr->LoadDevice("intell_voice_engine_manager_service");
    } else {
        INTELL_VOICE_LOG_ERROR("Get devmgr failed");
    }
}

void IntellVoiceService::UnloadIntellVoiceHost()
{
    auto devmgr = IDeviceManager::Get();
    if (devmgr != nullptr) {
        INTELL_VOICE_LOG_ERROR("Get devmgr success");
        devmgr->UnloadDevice("intell_voice_engine_manager_service");
    } else {
        INTELL_VOICE_LOG_ERROR("Get devmgr failed");
    }
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
    Security::AccessToken::PermStateChangeInfo& result)
{
    INTELL_VOICE_LOG_INFO("enter, permStateChangeType: %{public}d", result.permStateChangeType);
    if (result.permStateChangeType == 0) {
        INTELL_VOICE_LOG_ERROR("The permission is canceled.");
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
    if (isAdded) {
        INTELL_VOICE_LOG_INFO("distributed kv data service is added");
        const auto &manager = IntellVoiceServiceManager::GetInstance();
        if (manager == nullptr) {
            INTELL_VOICE_LOG_INFO("manager is nullptr");
            return;
        }

        manager->CreateSwitchProvider();

        if (reasonId_ == static_cast<int32_t>(OHOS::OnDemandReasonId::COMMON_EVENT)) {
            INTELL_VOICE_LOG_INFO("power on start");
            if (!manager->QuerySwitchStatus()) {
                manager->UnloadIntellVoiceService();
            } else {
                manager->OnServiceStart();
            }
        } else if (reasonId_ == static_cast<int32_t>(OHOS::OnDemandReasonId::INTERFACE_CALL)) {
            if (manager->QuerySwitchStatus()) {
                manager->OnServiceStart();
            }
        } else {
            INTELL_VOICE_LOG_INFO("no need to process, reason id:%{public}d", reasonId_);
        }
        reasonId_ = -1;
    } else {
        INTELL_VOICE_LOG_INFO("distributed kv data service is removed");
    }
}

void IntellVoiceService::OnTelephonyStateRegistryServiceChange(bool isAdded)
{
    if (isAdded) {
        INTELL_VOICE_LOG_INFO("telephony state registry service is added");
        auto triggerMgr = IntellVoiceTrigger::TriggerManager::GetInstance();
        if (triggerMgr != nullptr) {
            triggerMgr->AttachTelephonyObserver();
        }
    } else {
        INTELL_VOICE_LOG_INFO("telephony state registry service is removed");
    }
}

void IntellVoiceService::OnAudioDistributedServiceChange(bool isAdded)
{
    if (isAdded) {
        INTELL_VOICE_LOG_INFO("audio distributed service is added");
        auto triggerMgr = IntellVoiceTrigger::TriggerManager::GetInstance();
        if (triggerMgr != nullptr) {
            triggerMgr->AttachAudioCaptureListener();
        }
    } else {
        INTELL_VOICE_LOG_INFO("audio distributed service is removed");
    }
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS