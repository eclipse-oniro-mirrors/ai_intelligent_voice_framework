/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_3_INTELLVOICEENGINETYPES_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_3_INTELLVOICEENGINETYPES_H

#include "../v1_0/intell_voice_engine_types.h"

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_3 {
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

using IntellVoiceEngineAdapterType = OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterType;

constexpr IntellVoiceEngineAdapterType IMPLICIT_ENROLL_ADAPTER_TYPE =
    static_cast<IntellVoiceEngineAdapterType>(4);

using IntellVoiceEngineAdapterDescriptor =
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor;
} // V1_3
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_3_INTELLVOICEENGINETYPES_H
