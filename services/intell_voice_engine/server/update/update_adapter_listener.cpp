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
#include "update_adapter_listener.h"

#include <thread>
#include "intell_voice_log.h"

#define LOG_TAG "UpdateAdapterListener"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

namespace OHOS {
namespace IntellVoiceEngine {
UpdateEngineCallback::UpdateEngineCallback(OnEnrollEventCb enrollEventCb) : enrollEventCb_(enrollEventCb)
{
    INTELL_VOICE_LOG_INFO("constructor");
}

UpdateEngineCallback::~UpdateEngineCallback()
{
    INTELL_VOICE_LOG_INFO("destructor");
}

int UpdateEngineCallback::OnIntellVoiceHdiEvent(const IntellVoiceEngineCallBackEvent& event)
{
    INTELL_VOICE_LOG_INFO("OnIntellVoiceHdiEvent update msgId %{public}d result %{public}d", event.msgId, event.result);

    enrollEventCb_(event.msgId, event.result);

    return 0;
}
}
}