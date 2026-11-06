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
#ifndef WAKEUP_ENGINE_IMPL_FACADE_H
#define WAKEUP_ENGINE_IMPL_FACADE_H
 
#include "state_manager.h"
#include "base_wakeup_engine_impl.h"
#include "i_intell_voice_engine.h"
#include "i_device_wakeup.h"
#include "intell_voice_generic_factory.h"
#include "i_intell_voice_engine_callback.h"
#include "refbase.h"
 
namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::State;
using OHOS::IntellVoiceUtils::ProcessType;
using OHOS::IntellVoiceUtils::StateInfoListener;
 
class WakeupEngineImplFacade {
public:
    WakeupEngineImplFacade();
    virtual ~WakeupEngineImplFacade();
    virtual wptr<RefBase> GetWeakRef() = 0;
    bool Init();
    int32_t Handle(const StateMsg &msg);
    int32_t OnTriggerDetected(int32_t uuid, const std::string &macAddress, const EventDetail &event);
    void SaveCallback(sptr<IIntelligentVoiceEngineCallback> callback);
 
private:
    enum WakeupDeviceCheck {
        SUCCESS = 0,
        DEVICE_ERROR = -1,
        WAKEUP_PROCESSING = -2,
    };
 
private:
    int32_t InitDefaultImpl();
    int32_t UpdateWakeupEngineImpl(const std::shared_ptr<WakeupDeviceDetail> &info);
    int32_t ReleaseEngineImpl();
    int32_t CreateEngineImpl();
    std::shared_ptr<BaseWakeupEngineImpl> GetCurrentImpl();
    bool ProcessCheck();
    int32_t OnWispherDetected(int32_t uuid, const EventDetail &event);
    int32_t OnWakeupDetected(int32_t uuid, const std::string &macAddress, const EventDetail &event);
    void InitImpl();
    void OnStateChange(const State &stateInfo, const ProcessType &type);
 
private:
    std::shared_ptr<BaseWakeupEngineImpl> wakeupImpl_ = nullptr;
    std::shared_ptr<BaseWakeupEngineImpl> wispherImpl_ = nullptr;
    std::shared_ptr<BaseWakeupEngineImpl> defaultImpl_ = nullptr;
    std::shared_ptr<WakeupDeviceDetail> wakeupDevice_ = nullptr;
    std::mutex deviceMutex_;
    sptr<IIntelligentVoiceEngineCallback> callback_ = nullptr;
    std::mutex stateMutex_;
    EngineState curState_ = EngineState::IDLE;
    ProcessType curProcess_ = ProcessType::PROCESS_IDLE;
    std::shared_ptr<StateInfoListener> stateListener_ = nullptr;
    uint64_t deviceChangeCbId_ = 0;
};
}
}
#endif