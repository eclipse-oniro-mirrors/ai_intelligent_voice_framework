/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef ENGINE_MANAGER_HOST_H
#define ENGINE_MANAGER_HOST_H
#include "v1_0/iintell_voice_engine_manager.h"
#include "v1_2/iintell_voice_engine_manager.h"
#include "v1_3/iintell_voice_engine_manager.h"
#include "intell_voice_death_recipient.h"
#include "i_adapter_host_manager.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineAdapter;
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor;
using OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceDataOprCallback;
using OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFile;

class EngineHostManager {
public:
    EngineHostManager() = default;
    ~EngineHostManager();
    static EngineHostManager &GetInstance()
    {
        static EngineHostManager engineHostMgr;
        return engineHostMgr;
    }

    bool Init();
    bool RegisterEngineHDIDeathRecipient();
    void DeregisterEngineHDIDeathRecipient();
    void SetDataOprCallback();
    std::shared_ptr<IAdapterHostManager> CreateEngineAdapter(const IntellVoiceEngineAdapterDescriptor &desc);
    void ReleaseEngineAdapter(const IntellVoiceEngineAdapterDescriptor &desc);
    int GetUploadFiles(int numMax, std::vector<UploadHdiFile> &files);
    int32_t GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles);
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer);
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer);
    int32_t ClearUserWakeupData(const std::string &wakeupPhrase);
    const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> &GetEngineHostProxy1_0() const
    {
        return engineHostProxy1_0_;
    }
    const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> &GetEngineHostProxy1_1() const
    {
        return engineHostProxy1_1_;
    }
    const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> &GetEngineHostProxy1_2() const
    {
        return engineHostProxy1_2_;
    }
    const sptr<OHOS::HDI::IntelligentVoice::Engine::V1_3::IIntellVoiceEngineManager> &GetEngineHostProxy1_3() const
    {
        return engineHostProxy1_3_;
    }

private:
    static void OnEngineHDIDiedCallback();

    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager> engineHostProxy1_0_ = nullptr;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager> engineHostProxy1_1_ = nullptr;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager> engineHostProxy1_2_ = nullptr;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_3::IIntellVoiceEngineManager> engineHostProxy1_3_ = nullptr;
    sptr<OHOS::IntellVoiceUtils::IntellVoiceDeathRecipient> engineHdiDeathRecipient_ = nullptr;
    sptr<IIntellVoiceDataOprCallback> dataOprCb_ = nullptr;
};
}
}
#endif
