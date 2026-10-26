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
#ifndef GLASSES_WAKEUP_ENGINE_IMPL_H
#define GLASSES_WAKEUP_ENGINE_IMPL_H
 
#include <memory>
#include <string>
#include <atomic>
#include "i_intell_voice_engine.h"
#include "i_device_wakeup.h"
#include "wakeup_adapter_listener.h"
#include "base_wakeup_engine_impl.h"
#include "glasses_wakeup_wrapper.h"
#include "state_manager.h"
#include "wakeup_source_process.h"
#include "task_executor.h"
#include "engine_util.h"
#include "accessories_cloud_reporter.h"
 
namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::State;
using OHOS::IntellVoiceUtils::ProcessType;
using OHOS::IntellVoiceUtils::StateInfoListener;
 
class GlassesWakeupEngineImpl : private OHOS::IntellVoiceUtils::ModuleStates,
    private WakeupSourceProcess, private OHOS::IntellVoiceUtils::TaskExecutor, public BaseWakeupEngineImpl {
public:
    GlassesWakeupEngineImpl();
    ~GlassesWakeupEngineImpl() override;
    bool Create(std::shared_ptr<StateInfoListener> stateListener) override;
    bool Destroy() override;
    bool Init();
    void OnTriggerDetected(int32_t uuid, const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice,
        const EventDetail &event) override;
    int32_t Handle(const StateMsg &msg) override;
 
private:
    bool StartAudioSource();
    void StopAudioSource();
    void OnWakeupEvent(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);
    void OnWakeupRecognition(int32_t result, const std::string &info);
    void ReadThread();
 
private:
    bool InitStates();
    int32_t HandleSetParam(const StateMsg &msg, State &nextState);
    int32_t HandleInit(const StateMsg &msg, State &nextState);
    int32_t HandleSetListener(const StateMsg &msg, State &nextState);
    int32_t HandleStart(const StateMsg &msg, State &nextState);
    int32_t HandleStop(const StateMsg &msg, State &nextState);
    int32_t HandleRecognizeComplete(const StateMsg &msg, State &nextState);
    int32_t HandleStartCapturer(const StateMsg &msg, State &nextState);
    int32_t HandleRead(const StateMsg &msg, State &nextState);
    int32_t HandleStopCapturer(const StateMsg &msg, State &nextState);
    int32_t HandleRecognizingTimeout(const StateMsg &msg, State &nextState);
    int32_t HandleResetAdapter(const StateMsg &msg, State &nextState);
    int32_t HandleRelease(const StateMsg &msg, State &nextState);
 
private:
    int32_t channels_ = 0;
    bool needSecondStageWakeup_ = false;
    std::atomic<bool> isReading_ = false;
    std::thread readThread_;
    std::shared_ptr<WakeupAdapterListener> adapterListener_ = nullptr;
    std::shared_ptr<AccessoriesCloudReporter> mCloudReporter { nullptr };
};
}
}
#endif