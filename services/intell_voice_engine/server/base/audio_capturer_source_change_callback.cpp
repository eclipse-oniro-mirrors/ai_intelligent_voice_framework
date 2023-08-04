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

#include "intell_voice_log.h"
#include "intell_voice_service_manager.h"
#include "audio_capturer_source_change_callback.h"

#define LOG_TAG "AudioCapturerSourceChangeCallback"

namespace OHOS {
namespace IntellVoiceEngine {
void AudioCapturerSourceChangeCallback::OnCapturerState(bool isActive)
{
    INTELL_VOICE_LOG_INFO("enter");
    const auto &manager = IntellVoiceServiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return;
    }
    if(isActive) {
       manager->StopDetection();
    } else {
        manager->StartDetection();
    }
}
}
}
