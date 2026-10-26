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

#ifndef AUDIO_ASR_H
#define AUDIO_ASR_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace IntellVoice {

class AudioAsr {
public:
    AudioAsr() = default;
    virtual ~AudioAsr() = default;
    int32_t Init() { return 0; }
    int32_t Start() { return 0; }
    int32_t Stop() { return 0; }
    int32_t SetParameter(const std::string& key, const std::string& value) { return 0; }
    int32_t SendAudioData(const std::vector<uint8_t>& audioData) { return 0; }
};

} // namespace IntellVoice
} // namespace OHOS

#endif