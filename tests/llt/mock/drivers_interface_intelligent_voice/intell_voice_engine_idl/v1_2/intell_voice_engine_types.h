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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_INTELLVOICEENGINETYPES_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_INTELLVOICEENGINETYPES_H

#include <cstdbool>
#include <cstdint>
#include <string>
#include <vector>
#include <ashmem.h>
#include "../v1_0/intell_voice_engine_types.h"

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
namespace V1_2 {

using namespace OHOS;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

enum ContentType : int32_t {
    DSP_MODLE = 0,
    CONTENT_TYPE_BUT,
    DEFAULT_DSP_MODLE = 0,
    SHORT_WORD_DSP_MODEL = 1,
};

enum IntellVoiceEngineMessageType : int32_t {
    INTELL_VOICE_ENGINE_MSG_NONE = 0,
    INTELL_VOICE_ENGINE_MSG_INIT_DONE = 1,
    INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE = 2,
    INTELL_VOICE_ENGINE_MSG_COMMIT_ENROLL_COMPLETE = 3,
    INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE = 4,
    INTELL_VOICE_ENGINE_MSG_RECONFIRM_RECOGNITION_COMPLETE = 5,
    INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE = 6,
};

enum UploadHdiFileType : int32_t {
    FILE_TYPE_ENROLL = 0,
    FILE_TYPE_WAKEUP = 1,
};

enum EvaluationResultCode : int32_t {
    UNKNOWN = 0,
    PASS = 1,
    WORD_EMPTY = 2,
    CHINESE_ONLY = 3,
    INVALID_LENGTH = 4,
    UNUSUAL_WORD = 5,
    CONSECUTIVE_SAME_WORD = 6,
    TOO_FEW_PHONEMES = 7,
    TOO_MANY_PHONEMES = 8,
    COMMON_INSTRUCTION = 9,
    COMMON_SPOKEN_LANGUAGE = 10,
    SENSITIVE_WORD = 11,
    NO_INITIAL_CONSONANT = 12,
    REPEATED_PHONEME = 13,
};

struct UploadHdiFile {
    OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFileType type;
    std::string filesDescription;
    std::vector<sptr<Ashmem>> filesContent;
};

struct EvaluationResultInfo {
    int32_t score;
    OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultCode resultCode;
};

inline bool UploadHdiFileBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFile& dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

inline bool UploadHdiFileBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFile& dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

inline bool EvaluationResultInfoBlockMarshalling(OHOS::MessageParcel &data,
    const OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultInfo& dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

inline bool EvaluationResultInfoBlockUnmarshalling(OHOS::MessageParcel &data,
    OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultInfo& dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

} // V1_2
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_INTELLVOICEENGINETYPES_H

