/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef DUMMY_ENGINE_MANAGER_H
#define DUMMY_ENGINE_MANAGER_H
#include "i_intell_voice_engine.h"
#include "i_intell_voice_service.h"

namespace OHOS {
namespace IntellVoiceEngine {
class DummyEngineManager {
public:
    DummyEngineManager() {};
    ~DummyEngineManager() = default;
    void SetScreenOff(bool value) {};
    static bool GetEnrollResult(IntellVoiceEngineType type) { return false; };

    bool AnyEngineExist(const std::vector<IntellVoiceEngineType>& types) { return false; };
    void OnAudioPolicyServiceChange(bool isAdded) {};
    bool RegisterProxyDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object) { return false; };
    bool DeregisterProxyDeathRecipient(IntellVoiceEngineType type) { return true; };
    bool RegisterDeviceDeathRecipient(const std::string &serviceName,
        const sptr<IRemoteObject> &object) { return false; };

    int32_t SetParameter(const std::string &key, const std::string &value) { return 0; };
    std::string GetParameter(const std::string &key) { return ""; };
    int32_t GetWakeupSourceFilesList(std::vector<std::string> &cloneFiles) { return 0; };
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer) { return 0; };
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer) { return 0; };
    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files) { return 0; };

    void ClearUserDataInner() {};
    void DeviceServiceDie(const std::string &serviceName) {};
    bool IsNeedUpdateComplete(int32_t result, const std::string &param) { return false; };
    bool IsNeedUpdateRetry() { return false; };
    void EngineOnDetected(int32_t uuid) {};
    void ClearWakeupEngineCb() {};
    bool CreateOrResetWakeupEngine() { return false; };
    bool IsEngineExist(IntellVoiceEngineType type) { return false; };
    sptr<IIntellVoiceEngine> CreateEngine(IntellVoiceEngineType type, const std::string &param = "")
        { return nullptr; };
    int32_t ReleaseEngineInner(IntellVoiceEngineType type) { return 0; };
    int32_t SilenceUpdate() { return 0; };
    int32_t WhisperVprUpdate(bool reEnroll = false) { return 0; };
    int32_t CloneUpdate(const std::string &wakeupInfo, const sptr<IRemoteObject> &object) { return 0; };
    int32_t ImplicitEnroll(const std::string &wakeupInfo, const sptr<IRemoteObject> &object) { return 0; };
    int32_t ServiceStopProc() { return 0; };
    void SetDspSensibility(const std::string &sensibility) {};
    void OnServiceStart() {};
    void OnServiceStop() {};
    int32_t RegisterWakeupDevice(const std::string &macAddress, const WakeupDeviceInfo &info) { return 0; };
    int32_t DeregisterWakeupDevice(const std::string &macAddress) { return 0; };
    void SubscribeSwingEvent(const std::string &event) {};
    void UnSubscribeSwingEvent(const std::string &event) {};
};
}  // namespace IntellVoice
}  // namespace OHOS
#endif
