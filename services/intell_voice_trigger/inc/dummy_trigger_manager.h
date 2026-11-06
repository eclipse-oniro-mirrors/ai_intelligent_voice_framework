/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef DUMMY_TRIGGER_MANAGER_H
#define DUMMY_TRIGGER_MANAGER_H

#include <cstdint>
#include <vector>
#include <functional>
#include <string>
#include "trigger_base_type.h"

namespace OHOS {
namespace IntellVoiceTrigger {
class DummyTriggerManager {
public:
    DummyTriggerManager() {};
    ~DummyTriggerManager() = default;

    void UpdateModel(std::vector<uint8_t> buffer, int32_t uuid, TriggerModelType type) {};
    void DeleteModel(int32_t uuid) {};
    bool IsModelExist(int32_t uuid) { return false; };
    void CreateDetector(int32_t uuid, std::function<void()> onDetected) {};
    void ReleaseTriggerDetector(int32_t uuid) {};
    int32_t StartDetection(int32_t uuid) { return 1; };
    void StopDetection(int32_t uuid) {};
    int32_t SetParameter(const std::string &key, const std::string &value) { return 0; };
    void ClearConcurrentControlList(bool isWhiteList, bool isBlackList){};
    std::string GetParameter(const std::string &key) { return ""; };
    void OnServiceStart() {};
    void OnServiceStop() {};
    void OnTelephonyStateRegistryServiceChange(bool isAdded) {};
    void OnAudioDistributedServiceChange(bool isAdded) {};
    void OnAudioPolicyServiceChange(bool isAdded) {};
    void OnPowerManagerServiceChange(bool isAdded) {};
    void OnDisplayManagerServiceChange(bool isAdded) {};
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif