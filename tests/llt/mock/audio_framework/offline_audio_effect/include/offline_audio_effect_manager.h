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

#ifndef MOCK_OFFLINE_AUDIO_EFFECT_MANAGER_H
#define MOCK_OFFLINE_AUDIO_EFFECT_MANAGER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {

struct OfflineEffectInfo {
    std::string effectName_;
    bool isEnabled_;
};

class OfflineAudioEffectChain {
public:
    OfflineAudioEffectChain(const std::string& name) : name_(name) {}
    virtual ~OfflineAudioEffectChain() = default;

    virtual int32_t Release()
    {
        return 0;
    }

    virtual int32_t Process(const uint8_t* inData, size_t inDataSize, uint8_t* outData, size_t outDataSize)
    {
        (void)inData;
        (void)inDataSize;
        (void)outData;
        (void)outDataSize;
        return 0;
    }

    virtual int32_t SetParam(const std::vector<uint8_t>& param)
    {
        (void)param;
        return 0;
    }

    virtual int32_t Configure(const AudioStreamInfo& inputStreamInfo, const AudioStreamInfo& outputStreamInfo)
    {
        (void)inputStreamInfo;
        (void)outputStreamInfo;
        return 0;
    }

    virtual int32_t Prepare()
    {
        return 0;
    }

    virtual int32_t GetEffectBufferSize(uint32_t& inputBufferSize, uint32_t& outputBufferSize)
    {
        inputBufferSize = 4096;
        outputBufferSize = 4096;
        return 0;
    }

    std::string name_;
};

class OfflineAudioEffectManager {
public:
    static OfflineAudioEffectManager& GetInstance()
    {
        static OfflineAudioEffectManager instance;
        return instance;
    }
    virtual ~OfflineAudioEffectManager() = default;

    int32_t SetProcessName(const std::string& processName)
    {
        (void)processName;
        return 0;
    }

    int32_t GetOfflineEffectInfos(std::vector<OfflineEffectInfo>& effectInfos)
    {
        (void)effectInfos;
        return 0;
    }

    int32_t SetOfflineEffectEnabled(const std::string& effectName, bool enabled)
    {
        (void)effectName;
        (void)enabled;
        return 0;
    }

    std::vector<std::string> GetOfflineAudioEffectChains()
    {
        return {"asr_tse_reg"};
    }

    std::unique_ptr<OfflineAudioEffectChain> CreateOfflineAudioEffectChain(const std::string& name)
    {
        return std::make_unique<OfflineAudioEffectChain>(name);
    }
};

} // namespace AudioStandard
} // namespace OHOS

#endif