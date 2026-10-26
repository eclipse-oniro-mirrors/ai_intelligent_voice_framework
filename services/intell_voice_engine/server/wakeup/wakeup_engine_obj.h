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
#ifndef WAKEUP_ENGINE_OBJ_H
#define WAKEUP_ENGINE_OBJ_H

#include "base_macros.h"
#include "intell_voice_generic_factory.h"
#include "wakeup_engine.h"
#include "wakeup_engine_impl_facade.h"
#include "engine_base.h"

namespace OHOS {
namespace IntellVoiceEngine {
class WakeupEngineObj : public WakeupEngine, public WakeupEngineImplFacade {
public:
    ~WakeupEngineObj() {}
    wptr<RefBase> GetWeakRef() override
    {
        return wptr<RefBase>(static_cast<EngineBase*>(this));
    }

private:
    WakeupEngineObj() {}
    IMPL_ROLE(WakeupEngineImplFacade);
    friend class IntellVoiceUtils::SptrFactory<EngineBase, WakeupEngineObj>;
};
}
}
#endif