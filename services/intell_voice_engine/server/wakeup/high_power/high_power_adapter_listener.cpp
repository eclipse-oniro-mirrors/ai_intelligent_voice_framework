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
#include "high_power_adapter_listener.h"
#include "intell_voice_log.h"
#include "v1_2/intell_voice_engine_types.h"
#include "intell_voice_util.h"

#define LOG_TAG "HighPowerAdapterListener"

using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_2;

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string PRE_WAKEUP_EVENT = "pre_wakeup_event";

HighPowerAdapterListener::HighPowerAdapterListener()
{
    INTELL_VOICE_LOG_INFO("constructor");
}

HighPowerAdapterListener::~HighPowerAdapterListener()
{
    INTELL_VOICE_LOG_INFO("destructor");
}

void HighPowerAdapterListener::SetCallback(const sptr<IIntelligentVoiceEngineCallback> &cb)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (cb == nullptr) {
        INTELL_VOICE_LOG_INFO("clear callback");
        cb_ = nullptr;
        return;
    }
    cb_ = cb;
    if (historyEvent_ != nullptr) {
        uint64_t rightnow = TimeUtil::GetCurrentTimeMs();
        cb_->OnIntellVoiceEngineEvent(*(historyEvent_.get()));
        INTELL_VOICE_LOG_WARN("callback delayed %{public}llu ms",
            static_cast<unsigned long long>(rightnow - historyMsgTime_));
        historyEvent_ = nullptr;
    }
}

std::string HighPowerAdapterListener::GetCallbackStatus()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (cb_ == nullptr) {
        INTELL_VOICE_LOG_WARN("cb_ is nullptr");
        return "false";
    }
    return "true";
}

void HighPowerAdapterListener::OnIntellVoiceHdiEvent(const IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_INIT_DONE) {
        IntellVoiceUtil::StartAbility(PRE_WAKEUP_EVENT);
    } else if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        if (cb_ == nullptr) {
            INTELL_VOICE_LOG_WARN("cb_ is nullptr");
            BackupCallBackEvent(event);
            return;
        }
        cb_->OnIntellVoiceEngineEvent(event);
    } else {
        INTELL_VOICE_LOG_WARN("unknow msg id:%{public}d", event.msgId);
    }
}

void HighPowerAdapterListener::BackupCallBackEvent(const IntellVoiceEngineCallBackEvent &event)
{
    if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        INTELL_VOICE_LOG_INFO("backup callBackEvent, msg id:%{public}d", event.msgId);
 
        historyEvent_ = std::make_shared<IntellVoiceEngineCallBackEvent>();
        if (historyEvent_ == nullptr) {
            INTELL_VOICE_LOG_INFO("historyEvent_ is nullptr");
            return;
        }
        historyEvent_->msgId = event.msgId;
        historyEvent_->result = event.result;
        historyEvent_->info = event.info;
        historyMsgTime_ = TimeUtil::GetCurrentTimeMs();
    } else {
        INTELL_VOICE_LOG_WARN("unknow msg id:%{public}d", event.msgId);
    }
}
}
}