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
#include "wakeup_engine_test.h"
#include "intell_voice_log.h"
#include "adapter_host_manager_test.h"

#define LOG_TAG "WakeupEngineTest"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;
using namespace std;

namespace OHOS {
namespace IntellVoiceEngine {

WakeupEngineTest::WakeupEngineTest() {}

WakeupEngineTest::~WakeupEngineTest() {}

int32_t WakeupEngineTest::InitState()
{
    ForState(IDLE).ACT(EVENT_INIT, HandleInitTest);
    ForState(INITIALIZED).ACT(EVENT_START_RECOGNIZE, HandleStartTest);
    ForState(RECOGNIZED).ACT(EVENT_START_CAPTURER, HandleStartCapturerTest);
    return 0;
}

int32_t WakeupEngineTest::HandleInitTest(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    adapter_ = std::make_shared<AdapterHostManagerTest>();
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }

    adapter_->SetParameter("model_path=/vendor/etc/audio/encoder.om");
    IntellVoiceEngineAdapterInfo adapterInfo = {};

    adapter_->Attach(adapterInfo);
    nextState = State(INITIALIZING);
    return 0;
}

int32_t WakeupEngineTest::HandleStartTest(const StateMsg & /* msg */, State &nextState)
{
    nextState = State(RECOGNIZING);
    return 0;
}

int32_t WakeupEngineTest::HandleStartCapturerTest(const StateMsg & /* msg */, State &nextState)
{
    nextState = State(READ_CAPTURER);
    return 0;
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS