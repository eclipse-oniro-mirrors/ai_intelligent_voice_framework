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

#ifndef MOCK_INTELL_VOICE_TRIGGER_PROXY_H
#define MOCK_INTELL_VOICE_TRIGGER_PROXY_H

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace OHOS {
namespace IntellVoice {

struct TriggerRecognizeResult {
    int32_t code;
    std::string triggerWord;
};

class IIntellVoiceTriggerProxy {
public:
    virtual ~IIntellVoiceTriggerProxy() = default;
    virtual int32_t Attach(const std::string& triggerWord)
    {
        (void)triggerWord;
        return 0;
    }
    virtual int32_t Detach() { return 0; }
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
};

class IntellVoiceTriggerProxy : public IIntellVoiceTriggerProxy {
public:
    IntellVoiceTriggerProxy() = default;
    virtual ~IntellVoiceTriggerProxy() = default;
};

} // namespace IntellVoice
} // namespace OHOS

#endif