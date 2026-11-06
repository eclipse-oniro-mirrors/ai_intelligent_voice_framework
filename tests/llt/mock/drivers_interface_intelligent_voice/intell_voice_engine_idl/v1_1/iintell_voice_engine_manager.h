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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_1_IINTELLVOICEENGINEMANAGER_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_1_IINTELLVOICEENGINEMANAGER_H

#include <stdint.h>
#include "hdf_base.h"
#include "hdi_base.h"
#include "../v1_0/iintell_voice_engine_manager.h"
#include "iintell_voice_data_opr_callback.h"

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
namespace V1_1 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

enum {
    CMD_INTELL_VOICE_ENGINE_MANAGER_SET_DATA_OPR_CALLBACK = 4,
};

class IIntellVoiceEngineManager : public OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.intelligent_voice.engine.v1_1.IIntellVoiceEngineManager")

    virtual ~IIntellVoiceEngineManager() = default;

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> Get(
        bool isStub = false);
    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> Get(
        const std::string &serviceName, bool isStub = false);

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> CastFrom(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> &parent);

    virtual int32_t SetDataOprCallback(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceDataOprCallback>& dataOprCallback) = 0;
};
} // V1_1
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_1_IINTELLVOICEENGINEMANAGER_H

