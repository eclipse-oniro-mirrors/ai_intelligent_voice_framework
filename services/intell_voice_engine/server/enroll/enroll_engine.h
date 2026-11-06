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
#ifndef ENROLL_ENGINE_H
#define ENROLL_ENGINE_H
#include <atomic>
#include <memory>
#include <string>
#include "v1_0/iintell_voice_engine_callback.h"
#include "audio_info.h"
#include "audio_source.h"
#include "intell_voice_generic_factory.h"
#include "engine_base.h"
#include "engine_util.h"
#include "enroll_adapter_listener.h"
#include "task_executor.h"

namespace OHOS {
namespace IntellVoiceEngine {
class EnrollEngine : public EngineBase, private EngineUtil, private OHOS::IntellVoiceUtils::TaskExecutor {
public:
    ~EnrollEngine();
    bool Init(const std::string &param, bool reEnroll = false) override;
    void SetCallback(sptr<IRemoteObject> object) override;
    int32_t Attach(const IntellVoiceEngineInfo &info) override;
    int32_t Detach(void) override;
    int32_t Start(bool isLast) override;
    int32_t Stop() override;
    int32_t SetParameter(const std::string &keyValueList) override;
    std::string GetParameter(const std::string &key) override;
    int32_t WriteAudio(const uint8_t *buffer, uint32_t size) override;
    int32_t Evaluate(const std::string &word, EvaluationResult &result) override;

private:
    EnrollEngine();
    bool SetParameterInner(const std::string &keyValueList);
    bool StartAudioSource();
    void StopAudioSource();
    void OnEnrollEvent(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);
    void OnEnrollComplete(int32_t result, const std::string &info);
    std::string GetTSEParameter();
    void SetParamOnAudioStopOrError();

private:
    using EngineUtil::adapter_;
    std::string name_ = "lp enroll engine instance";
    bool isPcmFromExternal_ = false;
    bool isWithCmdScenario_ = false;
    bool isNormalRegMode_ = false;
    bool isTopMicRegMode_ = false;
    std::atomic<int32_t> enrollResult_ = -1;
    uint32_t callerTokenId_ = 0;
    std::shared_ptr<EnrollAdapterListener> adapterListener_ = nullptr;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineCallback> callback_ = nullptr;
    std::unique_ptr<AudioSource> audioSource_ = nullptr;
    std::string wakeupPhrase_;
    std::mutex mutex_;
    OHOS::AudioStandard::AudioCapturerOptions capturerOptions_;
    std::atomic<int32_t> tseResult_ = -1;
    friend class IntellVoiceUtils::SptrFactory<EngineBase, EnrollEngine>;
};
}
}
#endif
