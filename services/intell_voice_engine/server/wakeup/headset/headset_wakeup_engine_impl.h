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
#ifndef HEADSET_WAKEUP_ENGINE_IMPL_H
#define HEADSET_WAKEUP_ENGINE_IMPL_H

#include <memory>
#include <string>
#include <atomic>
#include "v1_0/iintell_voice_engine_callback.h"
#include "audio_info.h"
#include "intell_voice_generic_factory.h"
#include "i_intell_voice_engine.h"
#include "i_device_wakeup.h"
#include "base_wakeup_engine_impl.h"
#include "state_manager.h"
#include "engine_util.h"
#include "wakeup_adapter_listener.h"
#include "wakeup_source_process.h"
#include "task_executor.h"
#include "accessories_cloud_reporter.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::State;
using OHOS::IntellVoiceUtils::ProcessType;
using OHOS::IntellVoiceUtils::StateInfoListener;

class HeadsetWakeupEngineImpl : private OHOS::IntellVoiceUtils::ModuleStates, private EngineUtil,
    private WakeupSourceProcess, private OHOS::IntellVoiceUtils::TaskExecutor, public BaseWakeupEngineImpl {
public:
    HeadsetWakeupEngineImpl();
    ~HeadsetWakeupEngineImpl() override;
    bool Create(std::shared_ptr<StateInfoListener> stateListener) override;
    bool Destroy() override;
    bool Init();
    void OnTriggerDetected(int32_t uuid, const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice,
        const EventDetail &event) override;
    int32_t Handle(const StateMsg &msg) override;

private:
    bool SetCallbackInner();
    int32_t AttachInner(const IntellVoiceEngineInfo &info);
    void OnWakeupEvent(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);
    void OnInitDone(int32_t result);
    void OnWakeupRecognition(int32_t result, const std::string &info);
    bool StartAudioSource();
    void StopAudioSource();
    void ReadThread();

private:
    bool InitStates();
    int32_t HandleSetParam(const StateMsg &msg, State &nextState);
    int32_t HandleInit(const StateMsg &msg, State &nextState);
    int32_t HandleSetListener(const StateMsg &msg, State &nextState);
    int32_t HandleInitDone(const StateMsg &msg, State &nextState);
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
    using EngineUtil::adapter_;
    int32_t channels_ = 0;
    std::atomic<bool> isReading_ = false;
    bool needSecondStageWakeup = true;
    std::thread readThread_;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineCallback> callback_ = nullptr;
    std::shared_ptr<WakeupAdapterListener> adapterListener_ = nullptr;
    friend class OHOS::IntellVoiceUtils::UniquePtrFactory<HeadsetWakeupEngineImpl>;
    std::shared_ptr<AccessoriesCloudReporter> mCloudReporter { nullptr };
};
}
}
#endif