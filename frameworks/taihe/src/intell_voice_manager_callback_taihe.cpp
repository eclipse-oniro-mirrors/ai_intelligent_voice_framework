/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "intell_voice_manager_callback_taihe.h"

#include "intell_voice_log.h"

#define LOG_TAG "IntellVoiceManagerCallbackTaihe"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {

IntellVoiceManagerCallbackTaihe::IntellVoiceManagerCallbackTaihe(
    std::shared_ptr<callback_view<void(ServiceChangeType)>> callback): callback_(callback)
{
    INTELL_VOICE_LOG_INFO("enter");
}

void IntellVoiceManagerCallbackTaihe::OnRemoteDied(const wptr<IRemoteObject> &remote)
{
    std::unique_lock<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("receive sa death callback");
    (void)remote;
    if (callback_ != nullptr) {
        (*callback_)(ServiceChangeType(ServiceChangeType::key_t::SERVICE_UNAVAILABLE));
    }
}

}  // namespace IntellVoiceTaihe
}  // namespace OHOS