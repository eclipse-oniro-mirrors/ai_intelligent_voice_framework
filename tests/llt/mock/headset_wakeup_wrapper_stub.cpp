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

/*
 * x86_64 stub override for headset_wakeup_wrapper.cpp
 * In the test environment, the headset Bluetooth library cannot be loaded,
 * so ReadHeadsetStream returns fake audio data instead of -1.
 * This ensures HeadsetWakeupEngineImpl's ReadThread writes data into
 * WakeupSourceProcess so that Read() can succeed.
 */

#include "headset_wakeup_wrapper.h"
#include "intell_voice_log.h"

#undef LOG_TAG
#define LOG_TAG "HeadsetWakeupWrapperStub"

namespace OHOS {
namespace IntellVoiceEngine {

HeadsetWakeupWrapper::HeadsetWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter (stub)");
}

HeadsetWakeupWrapper::~HeadsetWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter (stub)");
}

int32_t HeadsetWakeupWrapper::ReadHeadsetStream(std::vector<uint8_t> &audioStream, bool &hasAwakeWord)
{
    INTELL_VOICE_LOG_INFO("enter (stub)");
    std::lock_guard<std::mutex> lock(mutex_);
    // In test environment, return fake audio data so WakeupSourceProcess can read it
    audioStream.resize(640, 0);
    hasAwakeWord = false;
    return 1;
}

int32_t HeadsetWakeupWrapper::NotifyVerifyResult(bool result)
{
    INTELL_VOICE_LOG_INFO("enter (stub), result:%{public}d", result);
    return 0;
}

int32_t HeadsetWakeupWrapper::StopReadingStream()
{
    INTELL_VOICE_LOG_INFO("enter (stub)");
    return 0;
}

}  // namespace IntellVoice
}  // namespace OHOS
