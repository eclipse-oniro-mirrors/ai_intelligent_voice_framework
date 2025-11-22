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

#ifndef INTELL_VOICE_MANAGER_TAIHE_H
#define INTELL_VOICE_MANAGER_TAIHE_H

#include "intell_voice_info.h"
#include "intell_voice_manager.h"
#include "intell_voice_manager_callback_taihe.h"

#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using namespace taihe;
using OHOS::IntellVoice::IntellVoiceManager;
using ohos::ai::intelligentVoice::IntelligentVoiceManager;
using ohos::ai::intelligentVoice::ServiceChangeType;
using ohos::ai::intelligentVoice::IntelligentVoiceEngineType;

class IntelligentVoiceManagerImpl {
public:
    explicit IntelligentVoiceManagerImpl();
    ~IntelligentVoiceManagerImpl();

    taihe::array<IntelligentVoiceEngineType> getCapabilityInfo();

    void onServiceChange(callback_view<void(ServiceChangeType)> callback);

    void offServiceChange(optional_view<callback<void(ServiceChangeType)>> callback);

private:
    std::mutex mutex_;
    IntellVoiceManager *manager_ = nullptr;
    sptr<IntellVoiceManagerCallbackTaihe> serviceChangeCb_ = nullptr;
};
}  // namespace IntellVoiceTaihe
}  // namespace OHOS
#endif
