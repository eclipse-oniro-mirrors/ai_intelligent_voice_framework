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
#ifndef ENGINE_BASE_H
#define ENGINE_BASE_H
#include <vector>
#include "intell_voice_engine_stub.h"

namespace OHOS {
namespace IntellVoiceEngine {
class EngineBase : public IntellVoiceEngineStub {
public:
    ~EngineBase() = default;
    virtual bool Init(const std::string &param = "", bool reEnroll = false) = 0;
    int32_t WriteAudio(const uint8_t *buffer, uint32_t size) override
    {
        return 0;
    }
    virtual void OnDetected(int32_t uuid) {};
    virtual bool ResetAdapter()
    {
        return true;
    }
    virtual void ReleaseAdapter()
    {
    }
    int32_t StartCapturer(int32_t channels) override;
    int32_t Read(std::vector<uint8_t> &data) override;
    int32_t StopCapturer() override;
    int32_t GetWakeupPcm(std::vector<uint8_t> &data) override;
    int32_t Evaluate(const std::string &word, EvaluationResult &result) override;
    int32_t NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event) override;
protected:
    EngineBase() = default;
};
}
}
#endif
