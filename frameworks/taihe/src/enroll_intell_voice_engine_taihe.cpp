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

#include "enroll_intell_voice_engine_taihe.h"

#include "intellvoice_taihe_utils.h"

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "intell_voice_log.h"
#include "i_intell_voice_engine_callback.h"

#define LOG_TAG "EnrollEngineTaihe"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {
EnrollIntelligentVoiceEngineImpl::EnrollIntelligentVoiceEngineImpl(
    EnrollIntelligentVoiceEngineDescriptor const &descriptor)
{
    descriptor_.wakeupPhrase = descriptor.wakeupPhrase;
    engine_ = std::make_shared<EnrollIntellVoiceEngine>(descriptor_);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create enroll engine failed");
        return;
    }

    callback_ = std::make_shared<EnrollIntellVoiceEngineCallbackTaihe>();
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create intell voice engine callback taihe failed");
        return;
    }

    if (engine_->SetCallback(callback_) != 0) {
        INTELL_VOICE_LOG_ERROR("set callback failed");
        return;
    }
}

EnrollIntelligentVoiceEngineImpl::~EnrollIntelligentVoiceEngineImpl()
{
    engine_ = nullptr;
}

::taihe::array<::taihe::string> EnrollIntelligentVoiceEngineImpl::GetSupportedRegionsSync()
{
    return {"CN"};
}

void EnrollIntelligentVoiceEngineImpl::InitSync(::ohos::ai::intelligentVoice::EnrollEngineConfig const &config)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    EngineConfig engineConfig{
        .language = std::string(config.language),
        .region = std::string(config.region),
    };
    engine_->Init(engineConfig);
}

::ohos::ai::intelligentVoice::EnrollCallbackInfo EnrollIntelligentVoiceEngineImpl::EnrollForResultSync(bool isLast)
{
    ::ohos::ai::intelligentVoice::EnrollResult::key_t resultCodeKey;
    int retCode = UNKNOWN;
    IntellVoiceTaiheUtils::GetEnumKeyByValue<::ohos::ai::intelligentVoice::EnrollResult>(retCode, resultCodeKey);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return {ohos::ai::intelligentVoice::EnrollResult(resultCodeKey), ""};
    }

    engine_->Start(isLast);
    return {ohos::ai::intelligentVoice::EnrollResult(resultCodeKey), ""};
}

void EnrollIntelligentVoiceEngineImpl::StopSync()
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->Stop();
}

void EnrollIntelligentVoiceEngineImpl::CommitSync()
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->Commit();
}

void EnrollIntelligentVoiceEngineImpl::SetWakeupHapInfoSync(::ohos::ai::intelligentVoice::WakeupHapInfo const &info)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    WakeupHapInfo hapInfo{
        .bundleName = std::string(info.bundleName),
        .abilityName = std::string(info.abilityName),
    };
    engine_->SetWakeupHapInfo(hapInfo);
}

void EnrollIntelligentVoiceEngineImpl::SetSensibilitySync(::ohos::ai::intelligentVoice::SensibilityType sensibility)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->SetSensibility(sensibility);
}

void EnrollIntelligentVoiceEngineImpl::SetParameterSync(::taihe::string_view key, ::taihe::string_view value)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->SetParameter(std::string(key), std::string(value));
}

void EnrollIntelligentVoiceEngineImpl::GetParameterSync(::taihe::string_view key)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->GetParameter(std::string(key));
}

::ohos::ai::intelligentVoice::EvaluationResult EnrollIntelligentVoiceEngineImpl::EvaluateForResultSync(
    ::taihe::string_view word)
{
    ::ohos::ai::intelligentVoice::EvaluationResultCode::key_t resultCodeKey;
    int retCode = UNKNOWN;
    IntellVoiceTaiheUtils::GetEnumKeyByValue<::ohos::ai::intelligentVoice::EvaluationResultCode>(
        retCode, resultCodeKey);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return {0, ohos::ai::intelligentVoice::EvaluationResultCode(resultCodeKey)};
    }
    EvaluationResult result;
    engine_->Evaluate(std::string(word), result);

    IntellVoiceTaiheUtils::GetEnumKeyByValue<::ohos::ai::intelligentVoice::EvaluationResultCode>(
        result.resultCode, resultCodeKey);
    ::ohos::ai::intelligentVoice::EvaluationResult ret{
        .score = result.score,
        .resultCode = ohos::ai::intelligentVoice::EvaluationResultCode(resultCodeKey),
    };
    return ret;
}

void EnrollIntelligentVoiceEngineImpl::ReleaseSync()
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->Release();
}

EnrollIntelligentVoiceEngine CreateEnrollIntelligentVoiceEngineSync(
    EnrollIntelligentVoiceEngineDescriptor const &descriptor)
{
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<EnrollIntelligentVoiceEngineImpl, EnrollIntelligentVoiceEngine>(descriptor);
}

}  // namespace IntellVoiceTaihe
}  // namespace OHOS

TH_EXPORT_CPP_API_CreateEnrollIntelligentVoiceEngineSync(
    OHOS::IntellVoiceTaihe::CreateEnrollIntelligentVoiceEngineSync);
