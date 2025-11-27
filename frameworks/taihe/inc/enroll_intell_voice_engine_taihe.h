/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ENROLL_INTELL_VOICE_ENGINE_TAIHE_H
#define ENROLL_INTELL_VOICE_ENGINE_TAIHE_H

#include "intell_voice_manager.h"
#include "enroll_intell_voice_engine.h"
#include "enroll_intell_voice_engine_callback_taihe.h"

#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using ohos::ai::intelligentVoice::EnrollIntelligentVoiceEngine;
using ohos::ai::intelligentVoice::EnrollIntelligentVoiceEngineDescriptor;

class EnrollIntelligentVoiceEngineImpl {
public:
    explicit EnrollIntelligentVoiceEngineImpl(EnrollIntelligentVoiceEngineDescriptor const &descriptor);
    ~EnrollIntelligentVoiceEngineImpl();

    ::taihe::array<::taihe::string> GetSupportedRegionsSync();

    void InitSync(::ohos::ai::intelligentVoice::EnrollEngineConfig const &config);

    ::ohos::ai::intelligentVoice::EnrollCallbackInfo EnrollForResultSync(bool isLast);

    void StopSync();

    void CommitSync();

    void SetWakeupHapInfoSync(::ohos::ai::intelligentVoice::WakeupHapInfo const &info);

    void SetSensibilitySync(::ohos::ai::intelligentVoice::SensibilityType sensibility);

    void SetParameterSync(::taihe::string_view key, ::taihe::string_view value);

    void GetParameterSync(::taihe::string_view key);

    ::ohos::ai::intelligentVoice::EvaluationResult EvaluateForResultSync(::taihe::string_view word);

    void ReleaseSync();

private:
    std::shared_ptr<IntellVoice::EnrollIntellVoiceEngine> engine_ = nullptr;
    IntellVoice::EnrollIntelligentVoiceEngineDescriptor descriptor_;
    std::shared_ptr<EnrollIntellVoiceEngineCallbackTaihe> callback_ = nullptr;
};

}  // namespace IntellVoiceTaihe
}  // namespace OHOS

#endif
