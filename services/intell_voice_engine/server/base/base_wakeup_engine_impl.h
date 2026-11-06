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
#ifndef BASE_WAKEUP_ENGINE_IMPL_H
#define BASE_WAKEUP_ENGINE_IMPL_H
 
#include "state_manager.h"
#include "i_intell_voice_engine.h"
#include "i_device_wakeup.h"
 
namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::ProcessType;
using OHOS::IntellVoiceUtils::StateInfoListener;
 
enum EngineState {
    IDLE = 0,
    INITIALIZING = 1,
    INITIALIZED = 2,
    RECOGNIZING = 3,
    RECOGNIZED = 4,
    READ_CAPTURER = 5,
};

class BaseWakeupEngineImpl {
public:
    virtual ~BaseWakeupEngineImpl() = default;
    virtual bool Create(std::shared_ptr<StateInfoListener> stateListener) { return true; }
    virtual bool Destroy() { return true; }
    virtual void OnTriggerDetected(int32_t uuid, const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice,
        const EventDetail &event) {};
    virtual int32_t Handle(const StateMsg &msg) { return 0; }
};
}
}
#endif