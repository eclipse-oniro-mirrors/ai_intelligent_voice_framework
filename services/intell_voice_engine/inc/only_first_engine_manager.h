/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ONLY_FIRST_ENGINE_MANAGER_H
#define ONLY_FIRST_ENGINE_MANAGER_H

#include <mutex>
#include "engine_base.h"
#include "i_intell_voice_engine.h"
#include "i_intell_voice_service.h"
#include "intell_voice_definitions.h"
#include "intell_voice_death_recipient.h"

namespace OHOS {
namespace IntellVoiceEngine {
class OnlyFirstEngineManager {
public:
    OnlyFirstEngineManager();
    ~OnlyFirstEngineManager();
    void SetScreenOff(bool value) {};
    static bool GetEnrollResult(IntellVoiceEngineType type) { return false; };
    int32_t GetWakeupSourceFilesList(std::vector<std::string> &cloneFiles) { return 0; };
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer) { return 0; };
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer) { return 0; };
    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files) { return 0; };
    void ClearUserDataInner() {};
    void DeviceServiceDie(const std::string &serviceName) {};
    bool IsNeedUpdateComplete(int32_t result, const std::string &param) { return false; };
    bool IsNeedUpdateRetry() { return false; };
    int32_t SilenceUpdate() { return 0; };
    int32_t WhisperVprUpdate(bool reEnroll = false) { return 0; };
    int32_t CloneUpdate(const std::string &wakeupInfo, const sptr<IRemoteObject> &object) { return 0; };
    int32_t ImplicitEnroll(const std::string &wakeupInfo, const sptr<IRemoteObject> &object) { return 0; };
    void SetDspSensibility(const std::string &sensibility) {};
    void OnServiceStart() {};
    void OnServiceStop() {};
    void ClearWakeupEngineCb() {};

    bool IsEngineExist(IntellVoiceEngineType type);
    bool AnyEngineExist(const std::vector<IntellVoiceEngineType>& types);
    bool RegisterProxyDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object);
    bool DeregisterProxyDeathRecipient(IntellVoiceEngineType type);
    bool RegisterDeviceDeathRecipient(const std::string &serviceName,
        const sptr<IRemoteObject> &object) { return false; };
    int32_t RegisterWakeupDevice(const std::string &macAddress, const WakeupDeviceInfo &info) { return 0; };
    int32_t DeregisterWakeupDevice(const std::string &macAddress) { return 0; };
    void OnAudioPolicyServiceChange(bool isAdded) {};

    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    void EngineOnDetected(int32_t uuid);
    sptr<IIntellVoiceEngine> CreateEngine(IntellVoiceEngineType type, const std::string &param = "");
    int32_t ReleaseEngineInner(IntellVoiceEngineType type);
    bool CreateOrResetWakeupEngine();
    int32_t ServiceStopProc();
    void SubscribeSwingEvent(const std::string &event) {};
    void UnSubscribeSwingEvent(const std::string &event) {};

private:
    sptr<IIntellVoiceEngine> CreateEngineInner(IntellVoiceEngineType type);

private:
    std::mutex deathMutex_;
    sptr<EngineBase> wakeupEngine_ = nullptr;
    sptr<IntellVoiceUtils::IntellVoiceDeathRecipient> proxyDeathRecipient_ = nullptr;
    sptr<IRemoteObject> deathRecipientObj_ = nullptr;
};
}  // namespace IntellVoiceEngine
}  // namespace OHOS
#endif
