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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEADAPTER_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEADAPTER_H

#include <stdint.h>
#include <string>
#include <vector>
#include "hdf_base.h"
#include "hdi_base.h"
#include "intell_voice_engine_types.h"
#include "../v1_0/iintell_voice_engine_adapter.h"

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
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_2 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

enum {
    CMD_INTELL_VOICE_ENGINE_ADAPTER_GET_WAKEUP_PCM = 10,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_EVALUATE = 11,
};

class IIntellVoiceEngineAdapter : public OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineAdapter {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.intelligent_voice.engine.v1_2.IIntellVoiceEngineAdapter")

    // HDI interface version reported by GetVersion
    static constexpr uint32_t INTERFACE_MAJOR_VERSION = 1;
    static constexpr uint32_t INTERFACE_MINOR_VERSION = 2;

    virtual ~IIntellVoiceEngineAdapter() = default;

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineAdapter> CastFrom(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineAdapter> &parent);

    virtual int32_t GetWakeupPcm(std::vector<uint8_t>& data) = 0;

    virtual int32_t Evaluate(const std::string& word,
         OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultInfo& info) = 0;

    int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer) override
    {
        majorVer = INTERFACE_MAJOR_VERSION;
        minorVer = INTERFACE_MINOR_VERSION;
        return HDF_SUCCESS;
    }

    const std::u16string GetDesc() override
    {
        return metaDescriptor_;
    }
};
} // V1_2
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEADAPTER_H

