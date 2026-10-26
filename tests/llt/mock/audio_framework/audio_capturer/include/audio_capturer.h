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

#ifndef MOCK_AUDIO_CAPTURER_H
#define MOCK_AUDIO_CAPTURER_H

#include <cstdint>
#include <memory>
#include <string>
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {

class AudioCapturerCallback {
public:
    virtual ~AudioCapturerCallback() = default;
    virtual void OnStateChange(const int32_t state) {}
};

class AudioCapturer {
public:
    AudioCapturer() = default;
    virtual ~AudioCapturer() = default;

    static std::unique_ptr<AudioCapturer> Create(const int32_t& audioScene, const int32_t& audioMode)
    {
        (void)audioScene;
        (void)audioMode;
        return std::make_unique<AudioCapturer>();
    }

    static std::unique_ptr<AudioCapturer> Create(const AudioCapturerOptions& capturerOptions,
        const std::string& cachePath)
    {
        (void)capturerOptions;
        (void)cachePath;
        return std::make_unique<AudioCapturer>();
    }
    
    bool Start() { return true; }
    bool Stop() { return true; }
    bool Release() { return true; }
    int32_t GetBufferSize(int32_t& bufferSize)
    {
        bufferSize = 4096;
        return 0;
    }
    template<typename T>
    int32_t Read(T&& buffer, int32_t size, int32_t readCount)
    {
        (void)buffer;
        (void)size;
        (void)readCount;
        return size;
    }
    bool IsScoChanged(int32_t flag) { return flag == 0; }
};

} // namespace AudioStandard
} // namespace OHOS

#endif