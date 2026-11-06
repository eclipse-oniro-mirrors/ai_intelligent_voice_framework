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

#ifndef INTELL_VOICE_ENGINE_PROXY_H
#define INTELL_VOICE_ENGINE_PROXY_H

#include <iremote_proxy.h>
#include "i_intell_voice_engine.h"
#include "v1_2/intell_voice_engine_types.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultInfo;

class IntellVoiceEngineProxy : public IRemoteProxy<IIntellVoiceEngine> {
public:
    explicit IntellVoiceEngineProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IIntellVoiceEngine>(impl) {};
    virtual ~IntellVoiceEngineProxy() {};
    void SetCallback(sptr<IRemoteObject> object) override;
    int32_t Attach(const IntellVoiceEngineInfo &info) override;
    int32_t Detach(void) override;
    int32_t SetParameter(const std::string &keyValueList) override;
    std::string GetParameter(const std::string &key) override;
    int32_t Start(bool isLast) override;
    int32_t Stop(void) override;
    int32_t WriteAudio(const uint8_t *buffer, uint32_t size) override;
    int32_t StartCapturer(int32_t channels) override;
    int32_t Read(std::vector<uint8_t> &data) override;
    int32_t StopCapturer() override;
    int32_t GetWakeupPcm(std::vector<uint8_t> &data) override;
    int32_t Evaluate(const std::string &word, EvaluationResult &result) override;
    int32_t NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event) override;

private:
    static inline BrokerDelegator<IntellVoiceEngineProxy> delegator_;
};
}
}

#endif
