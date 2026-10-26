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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEMANAGER_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEMANAGER_H

#include <stdint.h>
#include <vector>
#include "hdf_base.h"
#include "hdi_base.h"
#include "iintell_voice_engine_adapter.h"
#include "intell_voice_engine_types.h"

#ifndef HDI_BUFF_MAX_SIZE
#define HDI_BUFF_MAX_SIZE (1024 * 200)
#endif

#ifndef HDI_VECTOR_MAX_SIZE
#define HDI_VECTOR_MAX_SIZE 102400000
#endif

#ifndef HDI_MAP_MAX_SIZE
#define HDI_MAP_MAX_SIZE 102400000
#endif

#ifndef HDI_CHECK_VALUE_RETURN
#define HDI_CHECK_VALUE_RETURN(lv, compare, rv, ret) do { \
    if ((lv) compare (rv)) { \
        return ret; \
    } \
} while (false)
#endif

#ifndef HDI_CHECK_VALUE_RET_GOTO
#define HDI_CHECK_VALUE_RET_GOTO(lv, compare, rv, ret, value, table) do { \
    if ((lv) compare (rv)) { \
        ret = value; \
        goto table; \
    } \
} while (false)
#endif

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_0 {
using namespace OHOS;
using namespace OHOS::HDI;

enum {
    CMD_INTELL_VOICE_ENGINE_MANAGER_GET_VERSION = 0,
    CMD_INTELL_VOICE_ENGINE_MANAGER_GET_ADAPTER_DESCRIPTORS = 1,
    CMD_INTELL_VOICE_ENGINE_MANAGER_CREATE_ADAPTER = 2,
    CMD_INTELL_VOICE_ENGINE_MANAGER_RELEASE_ADAPTER = 3,
};

class IIntellVoiceEngineManager : public HdiBase {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.intelligent_voice.engine.v1_0.IIntellVoiceEngineManager")

    virtual ~IIntellVoiceEngineManager() = default;

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> Get(
        bool isStub = false);
    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> Get(
        const std::string &serviceName, bool isStub = false);

    virtual int32_t GetAdapterDescriptors(
        std::vector<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor>& descs) = 0;

    virtual int32_t CreateAdapter(
        const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor& descriptor,
        sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineAdapter>& adapter) = 0;

    virtual int32_t ReleaseAdapter(
        const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor& descriptor) = 0;
};
} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEMANAGER_H

