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

#ifndef AUDIO_INFO_H
#define AUDIO_INFO_H

#include "audio_system.h"
#include <cstdint>

namespace OHOS {
namespace AudioStandard {

struct AudioSampleAttributes {
    uint32_t samplingRate = 16000;
    uint8_t channel = 1;
    uint8_t format = 1;
    uint8_t interleave = 0;
    uint32_t frameSize = 0;
    uint32_t stereoSamplingRate = 16000;
    uint32_t monoSamplingRate = 16000;
};

} // namespace AudioStandard
} // namespace OHOS

#endif