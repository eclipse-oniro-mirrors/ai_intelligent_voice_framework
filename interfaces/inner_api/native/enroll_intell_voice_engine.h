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

#ifndef ENROLL_INTELL_VOICE_ENGINE_H
#define ENROLL_INTELL_VOICE_ENGINE_H

#include "i_intell_voice_engine.h"
#include "engine_callback_inner.h"
#include "intell_voice_info.h"

namespace OHOS {
namespace IntellVoice {
using OHOS::IntellVoiceEngine::IIntellVoiceEngineEventCallback;
using OHOS::IntellVoiceEngine::IIntellVoiceEngine;
using OHOS::IntellVoiceEngine::EngineCallbackInner;
using OHOS::IntellVoiceEngine::EvaluationResult;

struct EnrollIntelligentVoiceEngineDescriptor {
    std::string wakeupPhrase;
};

struct EngineConfig {
    std::string language;
    std::string region;
    std::string enrollScenario;
};

class EnrollIntellVoiceEngine {
public:
    EnrollIntellVoiceEngine(const EnrollIntelligentVoiceEngineDescriptor &descriptor);
    ~EnrollIntellVoiceEngine();

    int32_t Init(const EngineConfig &config);
    int32_t Start(const bool &isLast);
    int32_t Stop();
    int32_t Commit();
    int32_t SetSensibility(const int32_t &sensibility);
    int32_t SetWakeupHapInfo(const WakeupHapInfo &info);
    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    int32_t Release();
    int32_t SetCallback(std::shared_ptr<IIntellVoiceEngineEventCallback> callback);
    int32_t Evaluate(const std::string &word, EvaluationResult &result);

    int32_t result_ = INTELLIGENT_VOICE_SUCCESS;

private:
    sptr<IIntellVoiceEngine> engine_ = nullptr;
    std::unique_ptr<EnrollIntelligentVoiceEngineDescriptor> descriptor_ = nullptr;
    sptr<EngineCallbackInner> callback_ = nullptr;
};
}  // namespace IntellVoice
}  // namespace OHOS

#endif
