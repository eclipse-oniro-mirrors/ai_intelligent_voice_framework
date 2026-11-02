/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef ENGINE_DEVICE_MANAGER_H
#define ENGINE_DEVICE_MANAGER_H
 
#include <memory>
#include <mutex>
#include <unordered_map>
#include "audio_policy_manager.h"
#include "i_device_wakeup.h"
#include "i_intell_voice_service.h"
 
namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::AudioStandard::AudioDeviceDescriptor;
 
using OnDeviceChangeCb = std::function<void(const std::shared_ptr<WakeupDeviceDetail> &)>;
using OnAudioDeviceChangeCb = std::function<void(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &)>;
 
class AudioDeviceChangeListener : public AudioStandard::AudioPreferredOutputDeviceChangeCallback {
public:
    explicit AudioDeviceChangeListener(OnAudioDeviceChangeCb callback);
    ~AudioDeviceChangeListener();
    void OnPreferredOutputDeviceUpdated(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc) override;
 
private:
    OnAudioDeviceChangeCb callback_ = nullptr;
};
 
class EngineDeviceManager : public std::enable_shared_from_this<EngineDeviceManager> {
public:
    static EngineDeviceManager &GetInstance()
    {
        static EngineDeviceManager deviceManager;
        return deviceManager;
    }
 
    ~EngineDeviceManager();
 
    std::shared_ptr<WakeupDeviceDetail> GetWakeupDevice();
    int32_t RegisterDevice(const std::string &macAddress, const WakeupDeviceInfo &info);
    int32_t DeregisterDevice(const std::string &macAddress, std::string &serviceName);
    int32_t RemoveByServiceName(const std::string &serviceName);
    int CountByServiceName(const std::string &serviceName);
    void OnAudioPolicyServiceUp();
    void UpdateAudioDevice(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc);
    uint64_t RegisterDeviceChangeCb(OnDeviceChangeCb callback);
    void UnregisterDeviceChangeCb(uint64_t handle);

private:
    explicit EngineDeviceManager();
    void SelectWakeupDevice(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &outputDeviceList);
    void RefreshActiveDevice();
    void NotifyCallbacksIfChanged();
    void RefreshWakeupDevice();
    void InitListener();
    void InitDefaultDevice();
    void InitAudioDevice();
    void SetAudioPolicyListener();
    std::shared_ptr<WakeupDeviceDetail> ExtractDeviceDetail(const std::string &macAddress,
        const WakeupDeviceInfo &info);
 
private:
    std::mutex mutex_;
    std::mutex audioMutex_;
    std::string audioMacAddress_;
    std::string wakeupDeviceMacAddress_;
    std::shared_ptr<WakeupDeviceDetail> wakeupDevice_ = nullptr;
    std::shared_ptr<WakeupDeviceDetail> defaultWakeupDevice_ = nullptr;
    std::shared_ptr<AudioDeviceChangeListener> listener_ = nullptr;
    std::unordered_map<uint64_t, OnDeviceChangeCb> callbackList_;
    uint64_t nextCallbackId_ = 1;
    std::vector<std::shared_ptr<WakeupDeviceDetail>> deviceList_;
};
 
}  // namespace IntellVoice
}  // namespace OHOS
#endif