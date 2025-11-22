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
#ifndef WAKEUP_INTELL_VOICE_ENGINE_TAIHE_H
#define WAKEUP_INTELL_VOICE_ENGINE_TAIHE_H

#include "intell_voice_manager.h"
#include "wakeup_intell_voice_engine.h"
#include "wakeup_intell_voice_engine_callback_taihe.h"

#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngine;
using ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineDescriptor;
class WakeupIntelligentVoiceEngineImpl {
public:
    explicit WakeupIntelligentVoiceEngineImpl(WakeupIntelligentVoiceEngineDescriptor const &descriptor);
    ~WakeupIntelligentVoiceEngineImpl();

    ::taihe::array<::taihe::string> GetSupportedRegionsSync();

    void SetWakeupHapInfoSync(::ohos::ai::intelligentVoice::WakeupHapInfo const &info);

    void SetSensibilitySync(::ohos::ai::intelligentVoice::SensibilityType sensibility);

    void SetParameterSync(::taihe::string_view key, ::taihe::string_view value);

    void GetParameterSync(::taihe::string_view key);

    ::taihe::array<uint8_t> GetPcmSync();

    void StartCapturerSync(int32_t channels);

    ::taihe::array<uint8_t> ReadSync();

    void StopCapturerSync();

    void ReleaseSync();

    void onWakeupIntelligentVoiceEvent(
        ::taihe::callback_view<void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>
            callback);

    void offWakeupIntelligentVoiceEvent(::taihe::optional_view<
        ::taihe::callback<void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>>
            callback);

private:
    std::shared_ptr<IntellVoice::WakeupIntellVoiceEngine> engine_ = nullptr;
    std::shared_ptr<WakeupIntellVoiceEngineCallbackTaihe> callback_ = nullptr;
    IntellVoice::WakeupIntelligentVoiceEngineDescriptor descriptor_;
};

}  // namespace IntellVoiceTaihe
}  // namespace OHOS

#endif
