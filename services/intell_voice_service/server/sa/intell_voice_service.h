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
#ifndef INTELL_VOICE_SERVICE_H
#define INTELL_VOICE_SERVICE_H
#include <map>
#include <functional>
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "system_ability.h"
#include "intell_voice_service_stub.h"
#include "system_event_observer.h"
#include "i_device_wakeup.h"

namespace OHOS {
namespace IntellVoiceEngine {
class IntellVoiceService : public SystemAbility, public IntellVoiceServiceStub {
    DECLARE_SYSTEM_ABILITY(IntellVoiceService);
public:
    explicit IntellVoiceService(int32_t systemAbilityId, bool runOnCreate = true);
    ~IntellVoiceService();
    int32_t CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst) override;
    int32_t ReleaseIntellVoiceEngine(IntellVoiceEngineType type) override;

    bool RegisterDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object) override;
    bool DeregisterDeathRecipient(IntellVoiceEngineType type) override;
    bool RegisterDeviceDeathRecipient(const std::string &serviceName, const sptr<IRemoteObject> &object) override;

    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files) override;

    int32_t SetParameter(const std::string &keyValueList) override;
    std::string GetParameter(const std::string &key) override;
    int32_t GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles) override;
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer) override;
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer) override;
    int32_t EnrollWithWakeupFilesForResult(const std::string &wakeupInfo, const sptr<IRemoteObject> object) override;
    int32_t ClearUserData() override;
    int32_t RegisterDeviceWakeupInfo(const std::string &macAddress, const WakeupDeviceInfo &info) override;
    int32_t DeregisterDeviceWakeupInfo(const std::string &macAddress) override;

    class PerStateChangeCbCustomizeCallback : public Security::AccessToken::PermStateChangeCallbackCustomize {
    public:
        explicit PerStateChangeCbCustomizeCallback(const Security::AccessToken::PermStateChangeScope &scopeInfo)
            : PermStateChangeCallbackCustomize(scopeInfo) {}
        ~PerStateChangeCbCustomizeCallback() {}

        void PermStateChangeCallback(Security::AccessToken::PermStateChangeInfo& result) override;
    };

protected:
    void OnStart(const SystemAbilityOnDemandReason &startReason) override;
    void OnStop() override;
    int32_t OnIdle(const SystemAbilityOnDemandReason &idleReason) override;
    void OnActive(const SystemAbilityOnDemandReason& activeReason) override;
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

private:
    void CreateSystemEventObserver();
    void RegisterPermissionCallback(const std::string &permissionName);
    void OnCommonEventServiceChange(bool isAdded);
    void OnDistributedKvDataServiceChange(bool isAdded);
    void InitIntellVoiceService();
    void OnReceiveCes(const OHOS::EventFwk::CommonEventData &data);
    bool NeedProcOnRemoved(int32_t systemAbilityId);

private:
    int32_t reasonId_ = -1;
    bool isServiceInit_ = false;
    bool isDataShareRemoved_ = false;
    std::string reasonName_;
    std::string reasonValue_;
    std::mutex initServiceMutex_;
    std::shared_ptr<SystemEventObserver> systemEventObserver_ = nullptr;
    std::map<int32_t, std::function<void(bool)>> systemAbilityChangeMap_;
};
}
}
#endif
