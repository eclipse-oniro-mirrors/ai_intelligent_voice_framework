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
#include "engine_device_manager.h"
#include "json/json.h"
#include "intell_voice_log.h"
#include "intell_voice_definitions.h"
#include "wakeup_track.h"

#undef LOG_TAG
#define LOG_TAG "EngineDeviceManager"
 
using namespace OHOS::AudioStandard;
using namespace OHOS::IntellVoiceUtils;
namespace OHOS {
namespace IntellVoiceEngine {
 
AudioDeviceChangeListener::AudioDeviceChangeListener(OnAudioDeviceChangeCb callback): callback_(callback)
{
    INTELL_VOICE_LOG_INFO("constructor");
}
 
AudioDeviceChangeListener::~AudioDeviceChangeListener()
{
    INTELL_VOICE_LOG_INFO("destructor");
}
 
void AudioDeviceChangeListener::OnPreferredOutputDeviceUpdated(
    const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (callback_ != nullptr) {
        callback_(desc);
    }
}
 
EngineDeviceManager::EngineDeviceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
    InitDefaultDevice();
    InitListener();
}
 
EngineDeviceManager::~EngineDeviceManager()
{
    INTELL_VOICE_LOG_INFO("destructor");
    if (listener_ != nullptr) {
        AudioPolicyManager::GetInstance().UnsetPreferredOutputDeviceChangeCallback(listener_);
    }
}
 
void EngineDeviceManager::OnAudioPolicyServiceUp()
{
    try
    {
        std::lock_guard<std::mutex> lock(audioMutex_);
        INTELL_VOICE_LOG_INFO("enter");
        SetAudioPolicyListener();
        InitAudioDevice();
    } catch(...)
    {
        INTELL_VOICE_LOG_ERROR("init failed");
    }
}
 
std::shared_ptr<WakeupDeviceDetail> EngineDeviceManager::GetWakeupDevice()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return wakeupDevice_;
}
 
int32_t EngineDeviceManager::RegisterDevice(const std::string &macAddress, const WakeupDeviceInfo &info)
{
    if (macAddress.empty() || info.type < PHONE || info.type >= DEVICE_MAX) {
        INTELL_VOICE_LOG_ERROR("device info error, mac is empty: %{public}d, device type:%{public}d",
            macAddress.empty(), info.type);
        return -1;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("register start");
    for (auto it = deviceList_.begin(); it != deviceList_.end();) {
        if ((*it)->macAddress == macAddress) {
            INTELL_VOICE_LOG_INFO("remove device type:%{public}d", (*it)->type);
            it = deviceList_.erase(it);
        } else {
            it++;
        }
    }
    deviceList_.push_back(ExtractDeviceDetail(macAddress, info));
    RefreshActiveDevice();
    INTELL_VOICE_LOG_INFO("register success,service:%{public}s", info.serviceName.c_str());
    return 0;
}
 
int32_t EngineDeviceManager::DeregisterDevice(const std::string &macAddress, std::string &serviceName)
{
    if (macAddress.empty()) {
        INTELL_VOICE_LOG_ERROR("macAddress is empty");
        return -1;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("deregister start");
    for (auto it = deviceList_.begin(); it != deviceList_.end();) {
        if ((*it)->macAddress == macAddress) {
            serviceName = (*it)->serviceName;
            INTELL_VOICE_LOG_INFO("remove device type:%{public}d", (*it)->type);
            it = deviceList_.erase(it);
        } else {
            it++;
        }
    }
    RefreshActiveDevice();
    INTELL_VOICE_LOG_INFO("deregister success,service:%{public}s", serviceName.c_str());
    return 0;
}
 
int32_t EngineDeviceManager::CountByServiceName(const std::string &serviceName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (auto it = deviceList_.begin(); it != deviceList_.end(); it++) {
        if ((*it)->serviceName == serviceName) {
            count++;
        }
    }
    INTELL_VOICE_LOG_INFO("device count: %{public}d, service:%{public}s", count, serviceName.c_str());
    return count;
}
 
int32_t EngineDeviceManager::RemoveByServiceName(const std::string &serviceName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("service name: %{public}s", serviceName.c_str());
    for (auto it = deviceList_.begin(); it != deviceList_.end();) {
        if ((*it)->serviceName == serviceName) {
            it = deviceList_.erase(it);
            INTELL_VOICE_LOG_INFO("removed");
        } else {
            it++;
        }
    }
    RefreshActiveDevice();
    return 0;
}
 
void EngineDeviceManager::UpdateAudioDevice(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (desc.empty() || desc[0]->deviceType_ == OHOS::AudioStandard::DEVICE_TYPE_SPEAKER) {
        audioMacAddress_ = DEFAULT_MAC_ADDRESS;
        INTELL_VOICE_LOG_INFO("default device");
    } else {
        audioMacAddress_ = desc[0]->macAddress_;
        INTELL_VOICE_LOG_INFO("device type: %{public}d", desc[0]->deviceType_);
    }
    RefreshActiveDevice();
    INTELL_VOICE_LOG_INFO("update audioDevice success");
}
 
uint64_t EngineDeviceManager::RegisterDeviceChangeCb(OnDeviceChangeCb callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    uint64_t id = nextCallbackId_++;
    if (id == 0) {
        id = nextCallbackId_++;
    }
    callbackList_[id] = callback;
    INTELL_VOICE_LOG_INFO("registered, id:%{public}lu", static_cast<unsigned long>(id));
    return id;
}

void EngineDeviceManager::UnregisterDeviceChangeCb(uint64_t handle)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter, id:%{public}lu", static_cast<unsigned long>(handle));
    auto it = callbackList_.find(handle);
    if (it == callbackList_.end()) {
        INTELL_VOICE_LOG_WARN("id:%{public}lu not found", static_cast<unsigned long>(handle));
        return;
    }
    callbackList_.erase(it);
}
 
void EngineDeviceManager::RefreshActiveDevice()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::thread([this]() {
        INTELL_VOICE_LOG_INFO("enter");
        try {
            NotifyCallbacksIfChanged();
        } catch (...) {
            INTELL_VOICE_LOG_ERROR("refresh device error");
        }
    }).detach();
}

void EngineDeviceManager::NotifyCallbacksIfChanged()
{
    std::unordered_map<uint64_t, OnDeviceChangeCb> callbacks;
    std::shared_ptr<WakeupDeviceDetail> device;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::shared_ptr<WakeupDeviceDetail> lastDevice = wakeupDevice_;
        RefreshWakeupDevice();
        if (lastDevice == wakeupDevice_) {
            return;
        }
        WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_OPEN,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_SWITCH_DEVICE);
        callbacks = callbackList_;
        device = wakeupDevice_;
    }
    for (auto &[id, cb] : callbacks) {
        cb(device);
    }
}
 
void EngineDeviceManager::RefreshWakeupDevice()
{
    wakeupDevice_ = defaultWakeupDevice_;
    // if audio device in our list, active this one
    // else active last registed one
    bool findAudioDevice = false;
    for (auto it = deviceList_.begin(); it != deviceList_.end(); it++) {
        if ((*it)->macAddress == audioMacAddress_) {
            wakeupDevice_ = *it;
            findAudioDevice = true;
            break;
        }
    }
 
    if (!findAudioDevice && !deviceList_.empty()) {
        wakeupDevice_ = deviceList_.back();
    }
}
 
void EngineDeviceManager::InitDefaultDevice()
{
    std::lock_guard<std::mutex> lock(mutex_);
    WakeupDeviceInfo defaultDeviceInfo = { PHONE, DEFAULT_SERVICE_NAME, "{\"needSecondStageWakeup\":true}" };
    defaultWakeupDevice_ = ExtractDeviceDetail(DEFAULT_MAC_ADDRESS, defaultDeviceInfo);
    deviceList_.push_back(defaultWakeupDevice_);
    audioMacAddress_ = DEFAULT_MAC_ADDRESS;
    wakeupDevice_ = defaultWakeupDevice_;
    INTELL_VOICE_LOG_INFO("init default device success");
}
 
void EngineDeviceManager::InitListener()
{
    if (listener_ == nullptr) {
        listener_ = std::make_shared<AudioDeviceChangeListener>(
            [this](const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc) { UpdateAudioDevice(desc); });
        INTELL_VOICE_LOG_INFO("init listener success");
    }
}
 
void EngineDeviceManager::InitAudioDevice()
{
    AudioRendererInfo rendererInfo;
    rendererInfo.streamUsage = STREAM_USAGE_MUSIC;
    UpdateAudioDevice(AudioPolicyManager::GetInstance().GetPreferredOutputDeviceDescriptors(rendererInfo));
}
 
void EngineDeviceManager::SetAudioPolicyListener()
{
    if (listener_ == nullptr) {
        return;
    }
    AudioRendererInfo rendererInfo;
    rendererInfo.streamUsage = STREAM_USAGE_MUSIC;
    int32_t ret = AudioPolicyManager::GetInstance().SetPreferredOutputDeviceChangeCallback(rendererInfo, listener_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("init listener failed");
    }
}
 
std::shared_ptr<WakeupDeviceDetail> EngineDeviceManager::ExtractDeviceDetail(const std::string &macAddress,
    const WakeupDeviceInfo &info)
{
    std::shared_ptr<WakeupDeviceDetail> deviceDetail = std::make_shared<WakeupDeviceDetail>(info, macAddress);
    std::istringstream jsonStrm(deviceDetail->parameters);
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;
    reader["collectComments"] = false;
    if (!parseFromStream(reader, jsonStrm, &root, &errs)) {
        INTELL_VOICE_LOG_ERROR("parameters str is not json");
        return deviceDetail;
    }
    if (root.isMember("needSecondStageWakeup")) {
        deviceDetail->needSecondStageWakeup = root["needSecondStageWakeup"].asBool();
    }
    if (root.isMember("modelType")) {
        deviceDetail->modelType = root["modelType"].asString();
    }
    return deviceDetail;
}
}
}