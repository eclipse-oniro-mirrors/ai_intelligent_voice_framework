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
#ifndef ONLY_FIRST_WAKEUP_ENGINE_IMPL_H
#define ONLY_FIRST_WAKEUP_ENGINE_IMPL_H

#include <memory>
#include <string>
#include "i_intell_voice_engine.h"
#include "state_manager.h"
#include "wakeup_source_stop_callback.h"
#include "audio_source.h"
#include "wakeup_source_process.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::StateMsg;
using OHOS::IntellVoiceUtils::State;

enum EngineEvent {
    INIT = 0,
    INIT_DONE,
    START_RECOGNIZE,
    START_CAPTURER,
    READ,
    STOP_CAPTURER,
    READ_CAPTURER_TIMEOUT,
    GET_PARAM,
    RELEASE,
    RECOGNIZE_COMPLETE_TIMEOUT,
    RECORD_START,
};

struct CapturerData {
    std::vector<uint8_t> data;
};

struct StringParam {
    explicit StringParam(const std::string &str = "") : strParam(str) {}
    std::string strParam;
};

class OnlyFirstWakeupEngineImpl : private OHOS::IntellVoiceUtils::ModuleStates, private WakeupSourceProcess {
public:
    OnlyFirstWakeupEngineImpl();
    ~OnlyFirstWakeupEngineImpl();
    int32_t Handle(const StateMsg &msg);

private:
    enum EngineState {
        IDLE = 0,
        INITIALIZED,
        RECOGNIZED,
        READ_CAPTURER,
    };

private:
    bool StartAudioSource();
    void StopAudioSource();
    bool CreateWakeupSourceStopCallback();
    void DestroyWakeupSourceStopCallback();
    void OnInitDone(int32_t result);
    int32_t WakeupSource(void);

private:
    bool InitStates();
    int32_t HandleGetParam(const StateMsg &msg, State &nextState);
    int32_t HandleInit(const StateMsg &msg, State &nextState);
    int32_t HandleStart(const StateMsg &msg, State &nextState);
    int32_t HandleStartCapturer(const StateMsg &msg, State &nextState);
    int32_t HandleRead(const StateMsg &msg, State &nextState);
    int32_t HandleStopCapturer(const StateMsg &msg, State &nextState);
    int32_t HandleRelease(const StateMsg &msg, State &nextState);
    void ReadBufferCallback(uint8_t *buffer, uint32_t size, bool isEnd);
    int32_t HandleRecordStart(const StateMsg &msg, State &nextState);

private:
    int32_t channels_ = 0;
    uint32_t channelId_ = 0;
    std::shared_ptr<WakeupSourceStopCallback> wakeupSourceStopCallback_ = nullptr;
    std::unique_ptr<AudioSource> audioSource_ = nullptr;
    OHOS::AudioStandard::AudioCapturerOptions capturerOptions_;
    int32_t recordStart_ = -1;
};
}
}
#endif