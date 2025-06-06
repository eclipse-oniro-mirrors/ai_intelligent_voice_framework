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
#include "engine_base.h"

namespace OHOS {
namespace IntellVoiceEngine {
int32_t EngineBase::StartCapturer(int32_t /* channels */)
{
    return 0;
}

int32_t EngineBase::Read(std::vector<uint8_t> & /* data */)
{
    return 0;
}

int32_t EngineBase::StopCapturer()
{
    return 0;
}


int32_t EngineBase::GetWakeupPcm(std::vector<uint8_t> & /* data */)
{
    return 0;
}

int32_t EngineBase::NotifyHeadsetWakeEvent()
{
    return 0;
}

int32_t EngineBase::Evaluate(const std::string & /* word */, EvaluationResult & /* result */)
{
    return 0;
}

int32_t EngineBase::NotifyHeadsetHostEvent(HeadsetHostEventType /* event */)
{
    return 0;
}
}
}