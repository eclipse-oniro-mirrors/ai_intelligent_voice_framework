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

#include "trigger_service.h"

#include <malloc.h>

#include "intell_voice_log.h"

#define LOG_TAG "TriggerService"
using namespace std;

namespace OHOS {
namespace IntellVoiceTrigger {
TriggerService::TriggerService()
{
    triggerHelper_ = TriggerHelper::Create();
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("triggerHelper_ is nullptr");
    }
}

TriggerService::~TriggerService()
{}

void TriggerService::UpdateGenericTriggerModel(std::shared_ptr<GenericTriggerModel> model)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger model is null");
        return;
    }

    if (!TriggerDbHelper::GetInstance().UpdateGenericTriggerModel(model)) {
        INTELL_VOICE_LOG_ERROR("failed to update generic model");
    }
}

void TriggerService::DeleteGenericTriggerModel(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter");
    TriggerDbHelper::GetInstance().DeleteGenericTriggerModel(uuid);
}

std::shared_ptr<GenericTriggerModel> TriggerService::GetGenericTriggerModel(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter");
    auto model = TriggerDbHelper::GetInstance().GetGenericTriggerModel(uuid);
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get generic trigger model");
        return nullptr;
    }

    return model;
}

int32_t TriggerService::StartRecognition(
    int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return -1;
    }

    auto model = GetGenericTriggerModel(uuid);
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger model is nullptr, uuid:%{public}d", uuid);
        return -1;
    }

    return triggerHelper_->StartGenericRecognition(uuid, model, callback);
}

int32_t TriggerService::StopRecognition(int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return -1;
    }
    return triggerHelper_->StopGenericRecognition(uuid, callback);
}

void TriggerService::UnloadTriggerModel(int32_t uuid)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }

    triggerHelper_->UnloadGenericTriggerModel(uuid);
}

int32_t TriggerService::SetParameter(const std::string &key, const std::string &value)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return -1;
    }

    return triggerHelper_->SetParameter(key, value);
}

std::string TriggerService::GetParameter(const std::string &key)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return "";
    }

    return triggerHelper_->GetParameter(key);
}

#ifdef SUPPORT_TELEPHONY_SERVICE
void TriggerService::AttachTelephonyObserver()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachTelephonyObserver();
}

void TriggerService::DetachTelephonyObserver()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachTelephonyObserver();
}
#endif

#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
void TriggerService::ClearConcurrentControlList(bool isWhiteList, bool isBlackList)
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->ClearConcurrentControlList(isWhiteList, isBlackList);
}

void TriggerService::AttachAudioCapturerEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachAudioEngineListener();
}

void TriggerService::DetachAudioCapturerEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachAudioEngineListener();
}
#else
void TriggerService::AttachAudioCaptureListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachAudioCaptureListener();
}

void TriggerService::DetachAudioCaptureListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachAudioCaptureListener();
}
#endif

void TriggerService::AttachAudioRendererEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachAudioRendererEventListener();
}

void TriggerService::DetachAudioRendererEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachAudioRendererEventListener();
}

void TriggerService::AttachAudioSceneEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachAudioSceneEventListener();
}

void TriggerService::DetachAudioSceneEventListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachAudioSceneEventListener();
}

#ifdef SUPPORT_FAN_STATUS_LISTEN
void TriggerService::AttachFanStateListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachFanStateListener();
}

void TriggerService::DetachFanStateListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachFanStateListener();
}
#endif

#ifdef POWER_MANAGER_ENABLE
void TriggerService::AttachHibernateObserver()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachHibernateObserver();
}

void TriggerService::DetachHibernateObserver()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachHibernateObserver();
}
#endif

#ifdef SUPPORT_WINDOW_MANAGER
void TriggerService::AttachFoldStatusListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->AttachFoldStatusListener();
}

void TriggerService::DetachFoldStatusListener()
{
    if (triggerHelper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger helper is nullptr");
        return;
    }
    triggerHelper_->DetachFoldStatusListener();
}
#endif
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
