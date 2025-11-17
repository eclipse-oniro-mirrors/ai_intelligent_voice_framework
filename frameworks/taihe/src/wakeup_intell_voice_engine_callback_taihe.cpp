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

#include "wakeup_intell_voice_engine_callback_taihe.h"

#include "intell_voice_log.h"
#include "intell_voice_info.h"
#include "v1_2/intell_voice_engine_types.h"

#define LOG_TAG "WakeupEngineCallbackTaihe"

using namespace std;
using namespace taihe;
using namespace ohos::ai::intelligentVoice;
using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

namespace OHOS {
namespace IntellVoiceTaihe {
WakeupIntellVoiceEngineCallbackTaihe::WakeupIntellVoiceEngineCallbackTaihe(
    std::shared_ptr<::taihe::callback_view<
    void(::ohos::ai::intelligentVoice::WakeupIntelligentVoiceEngineCallbackInfo const &)>> callback): callback_(callback)
{
}

WakeupIntellVoiceEngineCallbackTaihe::~WakeupIntellVoiceEngineCallbackTaihe()
{
}

void WakeupIntellVoiceEngineCallbackTaihe::ClearCallbackRef()
{
    callback_ = nullptr;
}

void WakeupIntellVoiceEngineCallbackTaihe::OnEvent(const IntellVoiceEngineCallBackEvent &event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    WakeupIntelligentVoiceEventType::key_t eventId = WakeupIntelligentVoiceEventType::key_t::INTELLIGENT_VOICE_EVENT_WAKEUP_NONE;
    if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        eventId = WakeupIntelligentVoiceEventType::key_t::INTELLIGENT_VOICE_EVENT_RECOGNIZE_COMPLETE;
    } else if (event.msgId == static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(
        OHOS::HDI::IntelligentVoice::Engine::V1_2::INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE)) {
        eventId = WakeupIntelligentVoiceEventType::key_t::INTELLIGENT_VOICE_EVENT_HEADSET_RECOGNIZE_COMPLETE;
    } else {
        INTELL_VOICE_LOG_ERROR("error msgId:%{public}d", event.msgId);
        return;
    }
    WakeupIntelligentVoiceEventType eventType(eventId);
    WakeupIntelligentVoiceEngineCallbackInfo cbInfo = {eventType, event.result == 0 ? true : false, event.info};
    INTELL_VOICE_LOG_INFO("OnEvent EngineCallBackInfo: eventId: %{public}d, isSuccess: %{public}u",
        cbInfo.eventId.get_key(), cbInfo.isSuccess);
    if (callback_ != nullptr) {
        (*callback_)(cbInfo);
    }
}

}  // namespace IntellVoiceNapi
}  // namespace OHOS