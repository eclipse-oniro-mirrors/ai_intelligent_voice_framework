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

#ifndef WAKEUP_INTELL_VOICE_ENGINE_H
#define WAKEUP_INTELL_VOICE_ENGINE_H

#include "intell_voice_info.h"
#include "i_intell_voice_engine.h"
#include "engine_callback_inner.h"

namespace OHOS {
namespace IntellVoice {
using OHOS::IntellVoiceEngine::IIntellVoiceEngineEventCallback;
using OHOS::IntellVoiceEngine::IIntellVoiceEngine;
using OHOS::IntellVoiceEngine::EngineCallbackInner;
using OHOS::IntellVoiceEngine::EventDetail;

struct WakeupIntelligentVoiceEngineDescriptor {
    bool needReconfirm;
    std::string wakeupPhrase;
};

class WakeupIntellVoiceEngine {
public:
    WakeupIntellVoiceEngine(const WakeupIntelligentVoiceEngineDescriptor &descriptor,
        IntellVoiceEngine::IntellVoiceEngineType type = OHOS::IntellVoiceEngine::INTELL_VOICE_WAKEUP);
    ~WakeupIntellVoiceEngine();

    int32_t SetSensibility(const int32_t &sensibility);
    int32_t SetWakeupHapInfo(const WakeupHapInfo &info);
    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    int32_t Release();
    int32_t SetCallback(std::shared_ptr<IIntellVoiceEngineEventCallback> callback);
    int32_t StartCapturer(int32_t channels);
    int32_t Read(std::vector<uint8_t> &data);
    int32_t StopCapturer();
    int32_t GetWakeupPcm(std::vector<uint8_t> &data);
    int32_t NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event);
    int32_t result_ = INTELLIGENT_VOICE_SUCCESS;

private:
    sptr<IIntellVoiceEngine> engine_ = nullptr;
    std::unique_ptr<WakeupIntelligentVoiceEngineDescriptor> descriptor_ = nullptr;
    sptr<EngineCallbackInner> callback_ = nullptr;
};
}  // namespace IntellVoice
}  // namespace OHOS

#endif
