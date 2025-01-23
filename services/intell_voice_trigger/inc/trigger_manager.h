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
#ifndef INTELL_VOICE_TRIGGER_MANAGER_H
#define INTELL_VOICE_TRIGGER_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include "trigger_base_type.h"

namespace OHOS {
namespace IntellVoiceTrigger {
class TriggerService;
class TriggerDetector;

class TriggerManager {
public:
    TriggerManager();
    ~TriggerManager();

    void UpdateModel(std::vector<uint8_t> buffer, int32_t uuid, TriggerModelType type);
    void DeleteModel(int32_t uuid);
    bool IsModelExist(int32_t uuid);
    std::shared_ptr<GenericTriggerModel> GetModel(int32_t uuid);
    void CreateDetector(int32_t uuid, std::function<void()> onDetected);
    void ReleaseTriggerDetector(int32_t uuid);
    int32_t StartDetection(int32_t uuid);
    void StopDetection(int32_t uuid);
    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    void OnServiceStart();
    void OnServiceStop();
    void OnTelephonyStateRegistryServiceChange(bool isAdded);
    void OnAudioDistributedServiceChange(bool isAdded);
    void OnAudioPolicyServiceChange(bool isAdded);
    void OnPowerManagerServiceChange(bool isAdded);

private:
    void AttachTelephonyObserver();
    void DetachTelephonyObserver();
    void AttachAudioCaptureListener();
    void DetachAudioCaptureListener();
    void AttachAudioRendererEventListener();
    void DetachAudioRendererEventListener();
    void AttachHibernateObserver();
    void DetachHibernateObserver();

private:
    std::mutex detectorMutex_; //这个锁回头看看有没有必要删掉
    std::map<int32_t, std::shared_ptr<TriggerDetector>> detectors_;
    std::shared_ptr<TriggerService> service_ = nullptr;
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif