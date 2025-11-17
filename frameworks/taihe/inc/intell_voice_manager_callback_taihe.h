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

#ifndef INTELL_VOICE_MANAGER_CALLBACK_TAIHE_H
#define INTELL_VOICE_MANAGER_CALLBACK_TAIHE_H

#include "iremote_object.h"
#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using namespace taihe;
using ohos::ai::intelligentVoice::ServiceChangeType;

class IntellVoiceManagerCallbackTaihe : public IRemoteObject::DeathRecipient {
public:
    explicit IntellVoiceManagerCallbackTaihe(std::shared_ptr<callback_view<void(ServiceChangeType)>> callback);
    ~IntellVoiceManagerCallbackTaihe() override {};

    void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

private:
    std::mutex mutex_;
    std::shared_ptr<callback_view<void(ServiceChangeType)>> callback_ = nullptr;

};
}  // namespace IntellVoiceNapi
}  // namespace OHOS
#endif
