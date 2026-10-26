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
#ifndef INTELL_VOICE_TRIGGER_SERVICE_H
#define INTELL_VOICE_TRIGGER_SERVICE_H

#include "trigger_db_helper.h"
#include "trigger_helper.h"

namespace OHOS {
namespace IntellVoiceTrigger {
class TriggerService {
public:
    TriggerService();
    ~TriggerService();
    void UpdateGenericTriggerModel(std::shared_ptr<GenericTriggerModel> model);
    void DeleteGenericTriggerModel(int32_t uuid);
    std::shared_ptr<GenericTriggerModel> GetGenericTriggerModel(int32_t uuid);

    int32_t StartRecognition(int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    int32_t StopRecognition(int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    void UnloadTriggerModel(int32_t uuid);
    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
#ifdef SUPPORT_TELEPHONY_SERVICE
    void AttachTelephonyObserver();
    void DetachTelephonyObserver();
#endif
    void AttachAudioRendererEventListener();
    void DetachAudioRendererEventListener();
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    void AttachAudioCapturerEventListener();
    void DetachAudioCapturerEventListener();
    void ClearConcurrentControlList(bool isWhiteList, bool isBlackList);
#else
    void AttachAudioCaptureListener();
    void DetachAudioCaptureListener();
#endif

#ifdef POWER_MANAGER_ENABLE
    void AttachHibernateObserver();
    void DetachHibernateObserver();
#endif
#ifdef SUPPORT_WINDOW_MANAGER
    void AttachFoldStatusListener();
    void DetachFoldStatusListener();
#endif
    void AttachAudioSceneEventListener();
    void DetachAudioSceneEventListener();
#ifdef SUPPORT_FAN_STATUS_LISTEN
    void AttachFanStateListener();
    void DetachFanStateListener();
#endif

private:
    std::shared_ptr<TriggerHelper> triggerHelper_ = nullptr;
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif