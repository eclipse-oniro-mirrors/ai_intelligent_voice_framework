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
#ifndef WAKEUP_ENGINE_H
#define WAKEUP_ENGINE_H

#include <string>
#include "base_macros.h"
#include "engine_base.h"
#include "wakeup_engine_impl_facade.h"
#include "wakeup_engine_impl.h"

namespace OHOS {
namespace IntellVoiceEngine {
class WakeupEngine : public EngineBase {
public:
    WakeupEngine();
    ~WakeupEngine();
    bool Init(const std::string &param, bool reEnroll = false) override;
    void SetCallback(sptr<IRemoteObject> object) override;
    int32_t Attach(const IntellVoiceEngineInfo &info) override;
    int32_t Detach(void) override;
    int32_t Start(bool isLast) override;
    int32_t SetParameter(const std::string &keyValueList) override;
    std::string GetParameter(const std::string &key) override;
    int32_t Stop() override;
    int32_t GetWakeupPcm(std::vector<uint8_t> &data) override;

    void OnDetected(int32_t uuid) override;
    bool ResetAdapter() override;
    void ReleaseAdapter() override;

    int32_t StartCapturer(int32_t channels) override;
    int32_t Read(std::vector<uint8_t> &data) override;
    int32_t StopCapturer() override;
    int32_t NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event) override;

private:
    int32_t HandleCapturerMsg(StateMsg &msg);
    static void StartAbility(const std::string &event);
    USE_ROLE(WakeupEngineImplFacade);
};
}
}
#endif