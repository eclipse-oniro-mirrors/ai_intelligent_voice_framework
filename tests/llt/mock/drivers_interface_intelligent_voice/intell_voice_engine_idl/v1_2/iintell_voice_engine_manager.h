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

#ifndef OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEMANAGER_H
#define OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEMANAGER_H

#include <stdint.h>
#include <string>
#include <vector>
#include "hdf_base.h"
#include "hdi_base.h"
#include "../v1_0/intell_voice_engine_types.h"
#include "../v1_1/iintell_voice_engine_manager.h"
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

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_2 {
using namespace OHOS;
using namespace OHOS::HDI;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_1;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

enum {
    CMD_INTELL_VOICE_ENGINE_MANAGER_CREATE_ADAPTER__V_2 = 5,
    CMD_INTELL_VOICE_ENGINE_MANAGER_GET_UPLOAD_FILES = 6,
    CMD_INTELL_VOICE_ENGINE_MANAGER_GET_CLONE_FILES_LIST = 7,
    CMD_INTELL_VOICE_ENGINE_MANAGER_GET_CLONE_FILE = 8,
    CMD_INTELL_VOICE_ENGINE_MANAGER_SEND_CLONE_FILE = 9,
    CMD_INTELL_VOICE_ENGINE_MANAGER_CLEAR_USER_WAKEUP_DATA = 10,
};

class IIntellVoiceEngineManager : public OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.intelligent_voice.engine.v1_2.IIntellVoiceEngineManager")

    virtual ~IIntellVoiceEngineManager() = default;

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> Get(
        bool isStub = false);
    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> Get(
        const std::string &serviceName, bool isStub = false);

    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> CastFrom(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> &parent);
    static sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> CastFrom(
        const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> &parent);

    virtual int32_t CreateAdapter_V_2(
        const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor& descriptor,
        sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineAdapter>& adapter) = 0;

    virtual int32_t GetUploadFiles(int32_t numMax,
         std::vector<OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFile>& files) = 0;

    virtual int32_t GetCloneFilesList(std::vector<std::string>& cloneFiles) = 0;

    virtual int32_t GetCloneFile(const std::string& filePath, std::vector<uint8_t>& buffer) = 0;

    virtual int32_t SendCloneFile(const std::string& filePath, const std::vector<uint8_t>& buffer) = 0;

    virtual int32_t ClearUserWakeupData(const std::string& wakeupPhrase) = 0;
};
} // V1_2
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

#endif // OHOS_HDI_INTELLIGENT_VOICE_ENGINE_V1_2_IINTELLVOICEENGINEMANAGER_H

