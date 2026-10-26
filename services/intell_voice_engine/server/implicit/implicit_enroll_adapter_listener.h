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
#ifndef IMPLICIT_ENROLL_ADAPTER_LISTENER_H
#define IMPLICIT_ENROLL_ADAPTER_LISTENER_H

#include <functional>
#include "intell_voice_adapter_listener.h"
#include "i_intell_voice_update_callback.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OnEnrollEventCb = std::function<void(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)>;

class ImplicitEnrollAdapterListener : public IntellVoiceAdapterListener {
public:
    explicit ImplicitEnrollAdapterListener(OnEnrollEventCb enrollEventCb);
    ~ImplicitEnrollAdapterListener();

    void OnIntellVoiceHdiEvent(
        const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event) override;
    void SetCallback(const sptr<IntellVoice::IIntelligentVoiceUpdateCallback> &cb);
    void Notify(
        const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);
    void ClearBackupEvent();
    void BackupCallBackEvent(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event);

private:
    std::mutex mutex_;
    std::shared_ptr<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent> historyEvent_ = nullptr;
    sptr<IntellVoice::IIntelligentVoiceUpdateCallback> cb_ = nullptr;
    OnEnrollEventCb enrollEventCb_;
};
}
}
#endif