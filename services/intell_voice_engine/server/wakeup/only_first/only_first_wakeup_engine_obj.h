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
#ifndef ONLY_FIRST_WAKEUP_ENGINE_OBJ_H
#define ONLY_FIRST_WAKEUP_ENGINE_OBJ_H

#include "base_macros.h"
#include "intell_voice_generic_factory.h"
#include "only_first_wakeup_engine.h"
#include "only_first_wakeup_engine_impl.h"

namespace OHOS {
namespace IntellVoiceEngine {
class OnlyFirstWakeupEngineObj : public OnlyFirstWakeupEngine, public OnlyFirstWakeupEngineImpl {
public:
    ~OnlyFirstWakeupEngineObj() {}

private:
    OnlyFirstWakeupEngineObj() {}
    IMPL_ROLE(OnlyFirstWakeupEngineImpl);
    friend class IntellVoiceUtils::SptrFactory<EngineBase, OnlyFirstWakeupEngineObj>;
};
}
}
#endif