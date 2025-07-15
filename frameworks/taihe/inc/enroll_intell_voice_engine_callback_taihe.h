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

#ifndef ENROLL_INTELL_VOICE_ENGINE_CALLBACK_TAIHE_H
#define ENROLL_INTELL_VOICE_ENGINE_CALLBACK_TAIHE_H

#include <queue>
#include <map>
#include <uv.h>
#include "i_intell_voice_engine_callback.h"

namespace OHOS {
namespace IntellVoiceTaihe {
using OHOS::IntellVoiceEngine::IIntellVoiceEngineEventCallback;
using OHOS::IntellVoiceEngine::IntellVoiceEngineCallBackEvent;

struct EnrollCallbackInfo {
    int32_t eventId;
    int32_t result;
    std::string context;
};


class EnrollIntellVoiceEngineCallbackTaihe : public IIntellVoiceEngineEventCallback {
public:
    explicit EnrollIntellVoiceEngineCallbackTaihe();
    virtual ~EnrollIntellVoiceEngineCallbackTaihe();
    void OnEvent(const IntellVoiceEngineCallBackEvent &event) override;
};
}  // namespace IntellVoiceNapi
}  // namespace OHOS
#endif
