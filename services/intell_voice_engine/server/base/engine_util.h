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
#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H
#include <memory>
#include <mutex>
#include <string>
#include <map>
#include <vector>
#include <ashmem.h>
#include "i_adapter_host_manager.h"
#include <i_intell_voice_engine_callback.h>
#include "v1_0/iintell_voice_engine_adapter.h"
#include "v1_0/iintell_voice_engine_callback.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineCallback;

enum EngineEvent {
    NONE = 0,
    INIT,
    INIT_DONE,
    START_RECOGNIZE,
    STOP_RECOGNIZE,
    RECOGNIZE_COMPLETE,
    START_CAPTURER,
    READ,
    STOP_CAPTURER,
    RECOGNIZING_TIMEOUT,
    RECOGNIZE_COMPLETE_TIMEOUT,
    READ_CAPTURER_TIMEOUT,
    SET_LISTENER,
    SET_PARAM,
    GET_PARAM,
    GET_WAKEUP_PCM,
    RELEASE_ADAPTER,
    RESET_ADAPTER,
    RELEASE,
};

struct SetListenerMsg {
    explicit SetListenerMsg(sptr<IIntelligentVoiceEngineCallback> cb) : callback(cb) {}
    sptr<IIntelligentVoiceEngineCallback> callback = nullptr;
};

struct CapturerData {
    std::vector<uint8_t> data;
};

struct StringParam {
    explicit StringParam(const std::string &str = "") : strParam(str) {}
    std::string strParam;
};
const std::string WAKEUP_SENSIBILITY = "WAKEUP_SENSIBILITY";
class EngineUtil {
public:
    EngineUtil();
    ~EngineUtil() = default;
    template<typename T> bool CreateAdapterInner(T &mgr,
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterType type)
    {
        desc_.adapterType = type;
        adapter_ = mgr.CreateEngineAdapter(desc_);
        if (adapter_ == nullptr) {
            return false;
        }
        return true;
    }
    template<typename T> void ReleaseAdapterInner(T &mgr)
    {
        mgr.ReleaseEngineAdapter(desc_);
        adapter_ = nullptr;
    }
    int32_t SetParameter(const std::string &keyValueList);
    std::string GetParameter(const std::string &key);
    int32_t WriteAudio(const uint8_t *buffer, uint32_t size);
    int32_t Stop();
    int32_t GetWakeupPcm(std::vector<uint8_t> &data);
    int32_t Evaluate(const std::string &word, EvaluationResultInfo &info);
    bool SetDspFeatures();
    std::vector<uint8_t> ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::ContentType type);
    void ProcDspModel(const std::vector<uint8_t> &buffer);
    void SetLanguage();
    void SetWhisperVpr();
    void SetArea();
    void SetSensibility();
    void SetTriggerParameter();
    void SelectInputDevice(AudioStandard::DeviceType type);
    void SetScreenStatus();
    void SetImproveParam();

protected:
    std::shared_ptr<IAdapterHostManager> adapter_ = nullptr;
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterDescriptor desc_;

private:
    static void WriteBufferFromAshmem(uint8_t *&buffer, uint32_t size, sptr<OHOS::Ashmem> ashmem);
};
}
}
#endif