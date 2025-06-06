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
#ifndef TRIGGER_MANAGER_TEST_H
#define TRIGGER_MANAGER_TEST_H

#include <unistd.h>
#include <cstdio>

#include "trigger_helper.h"
#include "trigger_manager.h"

namespace OHOS {
namespace IntellVoiceTrigger {

class TriggerManagerTest {
public:
    TriggerManagerTest();
    ~TriggerManagerTest();

    bool InitRecognition(int32_t uuid);
    bool StartRecognition();
    bool StopRecognition();
    bool UnloadTriggerModel();
    bool DeleteModel(int32_t uuid);
    bool OnCallStateUpdated(int32_t callState);
    bool OnCapturerStateChange(bool isActive);
    void OnDetected(int32_t uuid);
    ModelState GetState(int32_t uuid);
    std::vector<uint8_t> ReadFile(const std::string &path);

public:
    std::shared_ptr<TriggerHelper> triggerHelper_ = nullptr;
    std::shared_ptr<TriggerService> triggerService_ = nullptr;
    std::shared_ptr<TriggerDetector> triggerDetector_ = nullptr;
    std::shared_ptr<TriggerManager> triggerManager_ = nullptr;
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif