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
#ifndef SERVICE_REGISTRAR_H
#define SERVICE_REGISTRAR_H
#include "i_intell_voice_engine.h"
#include "trigger_base_type.h"
namespace OHOS {
namespace IntellVoiceEngine {
class IntellVoiceEngineRegistrar {
public:
    virtual ~IntellVoiceEngineRegistrar() = default;
    void RegisterEngineCallbacks();

private:
    virtual void HandleCloseWakeupSource(bool isNeedStop = false) = 0;
    virtual void HandleClearWakeupEngineCb() = 0;
    virtual void HandleDeviceServiceDie(const std::string &serviceName) = 0;
    virtual int32_t HandleReleaseEngine(IntellVoiceEngineType type) = 0;
    virtual void HandleUpdateComplete(int32_t result, const std::string &param) = 0;
    virtual void HandleUpdateRetry() = 0;
    virtual int32_t ReleaseEngine(IntellVoiceEngineType type) = 0;
    virtual int32_t ReleaseImplicitEnrollEngine() = 0;
    virtual bool QuerySwitchStatus(const std::string &key) = 0;
    virtual std::string TriggerGetParameter(const std::string &key) = 0;
    virtual int32_t TriggerSetParameter(const std::string &key, const std::string &value) = 0;
    virtual void TriggerMgrUpdateModel(std::vector<uint8_t> buffer, int32_t uuid,
        IntellVoiceTrigger::TriggerModelType type) = 0;

private:
    void RegisterHandleCloseWakeupSource();
    void RegisterHandleClearWakeupEngineCb();
    void RegisterHandleDeviceServiceDie();
    void RegisterHandleReleaseEngine();
    void RegisterHandleUpdateComplete();
    void RegisterHandleUpdateRetry();
    void RegisterReleaseEngine();
    void RegisterReleaseImplicitEnrollEngine();
    void RegisterQuerySwitchStatus();
    void RegisterTriggerGetParameter();
    void RegisterTriggerSetParameter();
    void RegisterTriggerMgrUpdateModel();
};
}  // namespace IntellVoice
}  // namespace OHOS
#endif