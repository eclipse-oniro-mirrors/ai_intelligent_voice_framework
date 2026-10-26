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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEADAPTER_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEADAPTER_H

#include <stdint.h>
#include <string>
#include <vector>
#include <ashmem.h>
#include "hdf_base.h"
#include "hdi_base.h"
#include "iintell_voice_engine_callback.h"
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
    CMD_INTELL_VOICE_ENGINE_ADAPTER_GET_VERSION = 0,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_SET_CALLBACK = 1,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_ATTACH = 2,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_DETACH = 3,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_SET_PARAMETER = 4,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_GET_PARAMETER = 5,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_START = 6,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_STOP = 7,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_WRITE_AUDIO = 8,
    CMD_INTELL_VOICE_ENGINE_ADAPTER_READ = 9,
};

class IIntellVoiceEngineAdapter : public HdiBase {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.intelligent_voice.engine.v1_0.IIntellVoiceEngineAdapter")

    virtual ~IIntellVoiceEngineAdapter() = default;

    virtual int32_t SetCallback(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineCallback>& engineCallback) = 0;

    virtual int32_t Attach(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterInfo& info) = 0;

    virtual int32_t Detach() = 0;

    virtual int32_t SetParameter(const std::string& keyValueList) = 0;

    virtual int32_t GetParameter(const std::string& keyList, std::string& valueList) = 0;

    virtual int32_t Start(const OHOS::HDI::IntelligentVoice::Engine::V1_0::StartInfo& info) = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t WriteAudio(const std::vector<uint8_t>& buffer) = 0;

    virtual int32_t Read(OHOS::HDI::IntelligentVoice::Engine::V1_0::ContentType type, sptr<Ashmem>& buffer) = 0;

    virtual int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer)
    {
        majorVer = 1;
        minorVer = 0;
        return HDF_SUCCESS;
    }

    virtual bool IsProxy()
    {
        return false;
    }

    virtual const std::u16string GetDesc()
    {
        return metaDescriptor_;
    }
};
} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_0_IINTELLVOICEENGINEADAPTER_H

