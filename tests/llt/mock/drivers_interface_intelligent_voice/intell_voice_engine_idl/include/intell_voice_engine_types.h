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

#ifndef MOCK_INTELL_VOICE_ENGINE_TYPES_H
#define MOCK_INTELL_VOICE_ENGINE_TYPES_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace IntellVoice {

enum class EngineState {
    INVALID = 0,
    IDLE = 1,
    READY = 2,
    WORKING = 3,
};

struct IntellVoiceEngineInfo {
    std::string engineVersion;
    int32_t supportedSampleRate;
};

struct IntellVoiceRecognitionInfo {
    int32_t recognitionTime;
    std::string recognitionResult;
};

} // namespace IntellVoice
} // namespace OHOS

#endif