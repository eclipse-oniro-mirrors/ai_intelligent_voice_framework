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

#include "enroll_intell_voice_engine_callback_taihe.h"
#include "intell_voice_log.h"

#define LOG_TAG "EnrollEngineCallbackTaihe"

using namespace std;
using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;

namespace OHOS {
namespace IntellVoiceTaihe {
EnrollIntellVoiceEngineCallbackTaihe::EnrollIntellVoiceEngineCallbackTaihe()
{
}

EnrollIntellVoiceEngineCallbackTaihe::~EnrollIntellVoiceEngineCallbackTaihe()
{
}

void EnrollIntellVoiceEngineCallbackTaihe::OnEvent(const IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("OnEvent: msgId: %{public}d, errCode: %{public}d", event.msgId, event.result);
}

}  // namespace IntellVoiceNapi
}  // namespace OHOS