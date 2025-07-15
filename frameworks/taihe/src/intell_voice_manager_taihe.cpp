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
#include "intell_voice_manager_taihe.h"
#include "intell_voice_log.h"
#include "intell_voice_info.h"
#include "accesstoken_kit.h"
#include "tokenid_kit.h"
#include "ipc_skeleton.h"

#define LOG_TAG "IntellVoiceManagerTaihe"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {

IntelligentVoiceManagerImpl::IntelligentVoiceManagerImpl()
{
    manager_ = IntellVoiceManager::GetInstance();
    if (manager_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create native manager failed");
    }
}

IntelligentVoiceManagerImpl::~IntelligentVoiceManagerImpl()
{
    manager_ = nullptr;
}

taihe::array<IntelligentVoiceEngineType> IntelligentVoiceManagerImpl::getCapabilityInfo()
{
    INTELL_VOICE_LOG_INFO("enter");
    return {};
}

void IntelligentVoiceManagerImpl::onServiceChange(callback_view<void(ServiceChangeType)> callback)
{}

void IntelligentVoiceManagerImpl::offServiceChange(optional_view<callback<void(ServiceChangeType)>> callback)
{}

IntelligentVoiceManager GetIntelligentVoiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
    return make_holder<IntelligentVoiceManagerImpl, IntelligentVoiceManager>();
}
}  // namespace IntellVoiceTaihe
}  // namespace OHOS

TH_EXPORT_CPP_API_GetIntelligentVoiceManager(GetIntelligentVoiceManager);
