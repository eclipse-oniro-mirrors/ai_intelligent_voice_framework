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

#ifndef MOCK_INTELL_VOICE_ENGINE_PROXY_H
#define MOCK_INTELL_VOICE_ENGINE_PROXY_H

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace OHOS {
namespace IntellVoice {

struct WakeupSourceInfo {
    std::string phrase;
    int32_t confidence;
};

struct EngineRecognizeResult {
    int32_t code;
    std::string result;
};

class IIntellVoiceEngineProxy {
public:
    virtual ~IIntellVoiceEngineProxy() = default;
    virtual int32_t Start() { return 0; }
    virtual int32_t Stop() { return 0; }
    virtual int32_t SetParameter(const std::string& key, const std::string& value)
    {
        (void)key;
        (void)value;
        return 0;
    }
    virtual int32_t GetParameter(const std::string& key, std::string& value)
    {
        (void)key;
        value = "";
        return 0;
    }
    virtual int32_t SendAudioData(const std::vector<uint8_t>& audioData)
    {
        (void)audioData;
        return 0;
    }
};

class IntellVoiceEngineProxy : public IIntellVoiceEngineProxy {
public:
    IntellVoiceEngineProxy() = default;
    virtual ~IntellVoiceEngineProxy() = default;
};

} // namespace IntellVoice
} // namespace OHOS

#endif