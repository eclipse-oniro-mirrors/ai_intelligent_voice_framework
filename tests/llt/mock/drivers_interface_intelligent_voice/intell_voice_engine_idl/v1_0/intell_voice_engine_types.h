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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_INTELLVOICEENGINETYPES_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_INTELLVOICEENGINETYPES_H

#include <cstdbool>
#include <cstdint>
#include <string>

#ifndef HDI_BUFF_MAX_SIZE
#define HDI_BUFF_MAX_SIZE (1024 * 200)
#endif

#ifndef HDI_VECTOR_MAX_SIZE
#define HDI_VECTOR_MAX_SIZE 102400000
#endif

#ifndef HDI_MAP_MAX_SIZE
#define HDI_MAP_MAX_SIZE 102400000
#endif

namespace OHOS {
class MessageParcel;
}

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_0 {

using namespace OHOS;

enum IntellVoiceEngineAdapterType : int32_t {
    ENROLL_ADAPTER_TYPE = 0,
    WAKEUP_ADAPTER_TYPE = 1,
    UPDATE_ADAPTER_TYPE = 2,
    ADAPTER_TYPE_BUT,
};

enum ContentType : int32_t {
    DSP_MODLE = 0,
    CONTENT_TYPE_BUT,
};

enum IntellVoiceEngineMessageType : int32_t {
    INTELL_VOICE_ENGINE_MSG_NONE = 0,
    INTELL_VOICE_ENGINE_MSG_INIT_DONE = 1,
    INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE = 2,
    INTELL_VOICE_ENGINE_MSG_COMMIT_ENROLL_COMPLETE = 3,
    INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE = 4,
};

enum IntellVoiceEngineErrors : int32_t {
    INTELL_VOICE_ENGINE_OK = 0,
    INTELL_VOICE_ENGINE_ERROR_OFFSET = -100,
    INTELL_VOICE_ENGINE_INVALID_PARAMS = -101,
    INTELL_VOICE_ENGINE_INIT_FAILED = -102,
    INTELL_VOICE_ENGINE_ENROLL_FAILED = -103,
    INTELL_VOICE_ENGINE_COMMIT_ENROLL_FAILED = -104,
    INTELL_VOICE_ENGINE_WAKEUP_FAILED = -105,
};

struct IntellVoiceEngineAdapterDescriptor {
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterType adapterType;
} __attribute__ ((aligned(8)));

struct IntellVoiceEngineAdapterInfo {
    std::string wakeupPhrase;
    int32_t minBufSize;
    int32_t sampleChannels;
    int32_t bitsPerSample;
    int32_t sampleRate;
};

struct StartInfo {
    bool isLast;
} __attribute__ ((aligned(8)));

struct IntellVoiceEngineCallBackEvent {
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType msgId;
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors result;
    std::string info;
};

bool IntellVoiceEngineAdapterDescriptorBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor& dataBlock);

bool IntellVoiceEngineAdapterDescriptorBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor& dataBlock);

bool IntellVoiceEngineAdapterInfoBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterInfo& dataBlock);

bool IntellVoiceEngineAdapterInfoBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterInfo& dataBlock);

bool StartInfoBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::StartInfo& dataBlock);

bool StartInfoBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_0::StartInfo& dataBlock);

bool IntellVoiceEngineCallBackEventBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent& dataBlock);

bool IntellVoiceEngineCallBackEventBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent& dataBlock);

} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_INTELLVOICEENGINETYPES_H

