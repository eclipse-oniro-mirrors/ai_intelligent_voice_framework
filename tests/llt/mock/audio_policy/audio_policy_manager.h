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

#ifndef AUDIO_POLICY_MANAGER_H
#define AUDIO_POLICY_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "audio_system.h"

namespace OHOS {
namespace AudioStandard {

class AudioPolicyManager {
public:
    static AudioPolicyManager& GetInstance()
    {
        static AudioPolicyManager instance;
        return instance;
    }
    virtual ~AudioPolicyManager() = default;
    
    int32_t Init();
    int32_t SetAudioScene(AudioScene scene)
    {
        (void)scene;
        return 0;
    }
    bool IsStreamActive(int32_t streamType)
    {
        (void)streamType;
        return true;
    }
    int32_t SetPreferredOutputDeviceChangeCallback(
        const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback>& callback)
    {
        (void)callback;
        return 0;
    }
    int32_t SetPreferredOutputDeviceChangeCallback(AudioRendererInfo rendererInfo,
        const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback>& callback)
    {
        (void)rendererInfo;
        (void)callback;
        return 0;
    }
    int32_t UnsetPreferredOutputDeviceChangeCallback(
        const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback>& callback)
    {
        (void)callback;
        return 0;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetPreferredOutputDeviceDescriptors()
    {
        return {};
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetPreferredOutputDeviceDescriptors(
        AudioRendererInfo rendererInfo)
    {
        (void)rendererInfo;
        return {};
    }
    int32_t SelectInputDevice(void* filter, std::vector<void*>& devices)
    {
        (void)filter;
        (void)devices;
        return 0;
    }
};

} // namespace AudioStandard
} // namespace OHOS

#endif