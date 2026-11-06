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
#include "trigger_detector.h"
#include "intell_voice_log.h"
#include "trigger_detector_recognition_callback.h"
#include "memory_guard.h"

#define LOG_TAG "TriggerDetector"

namespace OHOS {
namespace IntellVoiceTrigger {
TriggerDetector::TriggerDetector(int32_t uuid, std::shared_ptr<TriggerService> service,
    std::shared_ptr<IIntellVoiceTriggerDetectorCallback> callback) : uuid_(uuid), service_(service)
{
    callback_ = std::make_shared<TriggerDetectorRecognitionCallback>(callback);
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
    }
}

TriggerDetector::~TriggerDetector()
{
    service_ = nullptr;
}

bool TriggerDetector::StartRecognition()
{
    OHOS::IntellVoiceUtils::MemoryGuard memoryGuard;
    if (service_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("service_ is nullptr");
        return false;
    }
    return service_->StartRecognition(uuid_, callback_) == 0;
}

bool TriggerDetector::StopRecognition()
{
    OHOS::IntellVoiceUtils::MemoryGuard memoryGuard;
    if (service_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("service_ is nullptr");
        return false;
    }
    service_->StopRecognition(uuid_, callback_);
    return true;
}

void TriggerDetector::UnloadTriggerModel()
{
    OHOS::IntellVoiceUtils::MemoryGuard memoryGuard;
    if (service_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("service_ is nullptr");
        return;
    }

    service_->UnloadTriggerModel(uuid_);
}
}
}