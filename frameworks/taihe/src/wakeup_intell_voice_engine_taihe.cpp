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

#include "wakeup_intell_voice_engine_taihe.h"

#include "want.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "intell_voice_log.h"
#include "i_intell_voice_engine_callback.h"

#define LOG_TAG "WakeupIntellVoiceEngineNapi"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {
WakeupIntelligentVoiceEngineImpl::WakeupIntelligentVoiceEngineImpl(
    WakeupIntelligentVoiceEngineDescriptor const &descriptor)
{
    descriptor_.wakeupPhrase = descriptor.wakeupPhrase;
    engine_ = std::make_shared<WakeupIntellVoiceEngine>(descriptor_);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create wakeup engine failed");
        return;
    }
}

WakeupIntelligentVoiceEngineImpl::~WakeupIntelligentVoiceEngineImpl()
{
    engine_ = nullptr;
}

::taihe::array<::taihe::string> WakeupIntelligentVoiceEngineImpl::GetSupportedRegionsSync()
{
    return { "CN" };
}

void WakeupIntelligentVoiceEngineImpl::SetWakeupHapInfoSync(::ohos::ai::intelligentVoice::WakeupHapInfo const &info)
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

void WakeupIntelligentVoiceEngineImpl::SetSensibilitySync(::ohos::ai::intelligentVoice::SensibilityType sensibility)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->SetSensibility(sensibility);
}

void WakeupIntelligentVoiceEngineImpl::SetParameterSync(::taihe::string_view key, ::taihe::string_view value)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->SetParameter(std::string(key), std::string(value));
}

void WakeupIntelligentVoiceEngineImpl::GetParameterSync(::taihe::string_view key)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->GetParameter(std::string(key));
}

::taihe::array<uint8_t> WakeupIntelligentVoiceEngineImpl::GetPcmSync()
{
    std::vector<uint8_t> data;
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return ::taihe::array<uint8_t>(data);
    }
    engine_->GetWakeupPcm(data);
    return ::taihe::array<uint8_t>(data);
}

void WakeupIntelligentVoiceEngineImpl::StartCapturerSync(int32_t channels)
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->StartCapturer(channels);
}

::taihe::array<uint8_t> WakeupIntelligentVoiceEngineImpl::ReadSync()
{
    std::vector<uint8_t> data;
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return ::taihe::array<uint8_t>(data);
    }
    engine_->Read(data);
    return ::taihe::array<uint8_t>(data);
}

void WakeupIntelligentVoiceEngineImpl::StopCapturerSync()
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->StopCapturer();
}

void WakeupIntelligentVoiceEngineImpl::ReleaseSync()
{
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->Release();
}

void WakeupIntelligentVoiceEngineImpl::onWakeupIntelligentVoiceEvent(
    callback_view<void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>
        callback)
{
    auto callbackRef = std::make_shared<callback_view<void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>>(callback);
    callback_ = std::make_shared<WakeupIntellVoiceEngineCallbackTaihe>(callbackRef);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is nullptr");
        return;
    }
    engine_->SetCallback(callback_);
}

void WakeupIntelligentVoiceEngineImpl::offWakeupIntelligentVoiceEvent(::taihe::optional_view<
    ::taihe::callback<void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>>
        callback)
{
    (void)callback;
    callback_->ClearCallbackRef();
}

WakeupIntelligentVoiceEngine CreateWakeupIntelligentVoiceEngineSync(
    WakeupIntelligentVoiceEngineDescriptor const &descriptor)
{
    // The parameters in the make_holder function should be of the same type
    // as the parameters in the constructor of the actual implementation class.
    return taihe::make_holder<WakeupIntelligentVoiceEngineImpl, WakeupIntelligentVoiceEngine>(descriptor);
}

}  // namespace IntellVoiceTaihe
}  // namespace OHOS

TH_EXPORT_CPP_API_CreateWakeupIntelligentVoiceEngineSync(
    OHOS::IntellVoiceTaihe::CreateWakeupIntelligentVoiceEngineSync);