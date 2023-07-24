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

#ifndef INTELL_VOICE_MANAGER_H
#define INTELL_VOICE_MANAGER_H

#include <condition_variable>

#include "i_intell_voice_engine.h"

namespace OHOS {
namespace IntellVoice {
using namespace std;
using namespace OHOS::IntellVoiceEngine;

class IntellVoiceManager {
public:
    static IntellVoiceManager *GetInstance();

    int32_t CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst);
    int32_t ReleaseIntellVoiceEngine(IntellVoiceEngineType type);
    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);
    void LoadSystemAbilityFail();
    bool Init();


private:
    IntellVoiceManager();
    ~IntellVoiceManager();

    static std::mutex mutex_;
    static std::condition_variable proxyConVar_;
};
}  // namespace IntellVoice
}  // namespace OHOS

#endif
