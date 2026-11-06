/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "implicit_enroll_adapter_listener.h"

#include <thread>
#include "intell_voice_log.h"

#define LOG_TAG "ImplicitEnrollAdapterListener"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoice;

namespace OHOS {
namespace IntellVoiceEngine {
ImplicitEnrollAdapterListener::ImplicitEnrollAdapterListener(OnEnrollEventCb enrollEventCb)
    : enrollEventCb_(enrollEventCb)
{
    INTELL_VOICE_LOG_INFO("constructor");
}

ImplicitEnrollAdapterListener::~ImplicitEnrollAdapterListener()
{
    INTELL_VOICE_LOG_INFO("destructor");
}

void ImplicitEnrollAdapterListener::OnIntellVoiceHdiEvent(const IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("OnIntellVoiceHdiEvent");
    if (enrollEventCb_ != nullptr) {
        enrollEventCb_(event);
    }
}

void ImplicitEnrollAdapterListener::SetCallback(const sptr<IIntelligentVoiceUpdateCallback> &cb)
{
    int result = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        INTELL_VOICE_LOG_INFO("enter");
        if (cb == nullptr) {
            INTELL_VOICE_LOG_INFO("clear callback");
            cb_ = nullptr;
            return;
        }
        cb_ = cb;
        if (historyEvent_ == nullptr) {
            return;
        }
        result = static_cast<int>(historyEvent_->result);
    }
    cb_->OnUpdateComplete(result);
}

void ImplicitEnrollAdapterListener::ClearBackupEvent()
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    historyEvent_ = nullptr;
}

void ImplicitEnrollAdapterListener::Notify(const IntellVoiceEngineCallBackEvent &event)
{
    sptr<IIntelligentVoiceUpdateCallback> cbCopy;
    int result = -1;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cb_ == nullptr) {
            INTELL_VOICE_LOG_WARN("cb_ is nullptr");
            BackupCallBackEvent(event);
            return;
        }

        historyEvent_ = nullptr;
        if (event.msgId != OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE) {
            return;
        }
        result = static_cast<int>(event.result);
        cbCopy = cb_;  // add sptr count in lock，avoid UAF
    }
    if (cbCopy != nullptr) {
        cbCopy->OnUpdateComplete(result);
    }
}

void ImplicitEnrollAdapterListener::BackupCallBackEvent(const IntellVoiceEngineCallBackEvent &event)
{
    if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE) {
        INTELL_VOICE_LOG_INFO("backup callBackEvent, msg id:%{public}d", event.msgId);
        IntellVoiceEngineCallBackEvent *raw = new (std::nothrow) IntellVoiceEngineCallBackEvent;
        if (raw == nullptr) {
            INTELL_VOICE_LOG_ERROR("allocate historyEvent_ failed");
            return;
        }
        historyEvent_.reset(raw);
        historyEvent_->msgId = event.msgId;
        historyEvent_->result = event.result;
        historyEvent_->info = event.info;
    } else {
        INTELL_VOICE_LOG_WARN("unknow msg id:%{public}d", event.msgId);
    }
}
}
}