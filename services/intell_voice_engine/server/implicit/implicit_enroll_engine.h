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
#ifndef IMPLICIT_ENROLL_ENGINE_H
#define IMPLICIT_ENROLL_ENGINE_H

#include <memory>
#include <string>
#include "v1_0/iintell_voice_engine_callback.h"
#include "audio_info.h"
#include "audio_source.h"
#include "intell_voice_generic_factory.h"
#include "engine_base.h"
#include "engine_util.h"
#include "state_manager.h"
#include "base_wakeup_engine_impl.h"
#include "implicit_enroll_adapter_listener.h"
#include "task_executor.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::State;
using OHOS::IntellVoiceUtils::ProcessType;
using OHOS::IntellVoiceUtils::StateInfoListener;

class ImplicitEnrollEngine : private OHOS::IntellVoiceUtils::ModuleStates,
    public EngineBase, private EngineUtil, private OHOS::IntellVoiceUtils::TaskExecutor {
public:
    ~ImplicitEnrollEngine();
    bool Init(const std::string &param, bool reEnroll = false) override;
    void SetCallback(sptr<IRemoteObject> object) override;
    int32_t Attach(const IntellVoiceEngineInfo &info) override;
    int32_t Detach(void) override;
    int32_t Start(bool isLast) override { return 0; };
    int32_t Stop() override { return 0; };
    int32_t SetParameter(const std::string &keyValueList) override;
    std::string GetParameter(const std::string &key) override { return ""; };

private:
    ImplicitEnrollEngine();
    int32_t Handle(const StateMsg &msg);
    bool InitStates();
    bool CheckParams(const std::string &param);
    void SetCallbackInner();
    void OnEnrollEvent(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);
    void OnEnrollComplete(int32_t result, const std::string &info);
    int32_t HandleSetParam(const StateMsg &msg, State &nextState);
    int32_t HandleInit(const StateMsg &msg, State &nextState);
    int32_t HandleSetListener(const StateMsg &msg, State &nextState);
    int32_t HandleEnrollComplete(const StateMsg &msg, State &nextState);
    int32_t HandleStart(const StateMsg &msg, State &nextState);
    int32_t HandleRecognizingTimeout(const StateMsg &msg, State &nextState);
    int32_t HandleRelease(const StateMsg &msg, State &nextState);

private:
    using EngineUtil::adapter_;
    std::shared_ptr<ImplicitEnrollAdapterListener> adapterListener_ = nullptr;
    sptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineCallback> callback_ = nullptr;
    std::string wakeupPhrase_;
    bool adapterInited_ = false;
    bool started_ = false;
    std::string stage_;
    std::string bundleName_;
    std::string abilityName_;
    int startTime_;
    int endTime_;
    std::string clearDataStr_ = "";
    friend class IntellVoiceUtils::SptrFactory<EngineBase, ImplicitEnrollEngine>;
};
}
}

#endif