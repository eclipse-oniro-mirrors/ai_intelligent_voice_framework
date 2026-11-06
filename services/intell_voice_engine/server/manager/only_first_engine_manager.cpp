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
#include "only_first_engine_manager.h"

#include <vector>
#include <fstream>
#include <cstdio>
#include "intell_voice_log.h"
#include "intell_voice_util.h"
#include "string_util.h"
#include "iservice_registry.h"
#include "intell_voice_generic_factory.h"
#include "memory_guard.h"
#include "engine_callback_message.h"
#include "intell_voice_definitions.h"
#include "only_first_wakeup_engine_obj.h"

#define LOG_TAG "OnlyFirstEngineManager"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {

OnlyFirstEngineManager::OnlyFirstEngineManager()
{
}

OnlyFirstEngineManager::~OnlyFirstEngineManager()
{
}

bool OnlyFirstEngineManager::IsEngineExist(IntellVoiceEngineType type)
{
    if (type == INTELL_VOICE_WAKEUP) {
        if (wakeupEngine_ != nullptr) {
            return true;
        }
    }

    return false;
}

bool OnlyFirstEngineManager::AnyEngineExist(const std::vector<IntellVoiceEngineType>& types)
{
    for (const auto &type : types) {
        if (IsEngineExist(type)) {
            return true;
        }
    }

    return false;
}

bool OnlyFirstEngineManager::RegisterProxyDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(deathMutex_);
    INTELL_VOICE_LOG_INFO("enter, type:%{public}d", type);

    if (type != INTELL_VOICE_WAKEUP) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return false;
    }

    if (proxyDeathRecipient_ != nullptr) {
        INTELL_VOICE_LOG_WARN("death recipient is not nullptr, type:%{public}d", type);
        return false;
    }

    deathRecipientObj_ = object;
    proxyDeathRecipient_ = new (std::nothrow) IntellVoiceDeathRecipient([&]() {
        INTELL_VOICE_LOG_INFO("receive wakeup proxy death recipient, clear wakeup engine callback");
        EngineCallbackMessage::CallFunc(HANDLE_CLEAR_WAKEUP_ENGINE_CB);
    });
    if (proxyDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create death recipient failed");
        return false;
    }

    return deathRecipientObj_->AddDeathRecipient(proxyDeathRecipient_);
}

bool OnlyFirstEngineManager::DeregisterProxyDeathRecipient(IntellVoiceEngineType type)
{
    std::lock_guard<std::mutex> lock(deathMutex_);
    INTELL_VOICE_LOG_INFO("enter, type:%{public}d", type);
    if (type != INTELL_VOICE_WAKEUP) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return false;
    }

    if (deathRecipientObj_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("death obj is nullptr, type:%{public}d", type);
        return false;
    }

    if (proxyDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("death recipient is nullptr, type:%{public}d", type);
        deathRecipientObj_ = nullptr;
        return false;
    }

    auto ret = deathRecipientObj_->RemoveDeathRecipient(proxyDeathRecipient_);
    deathRecipientObj_ = nullptr;
    proxyDeathRecipient_ = nullptr;

    return ret;
}

int32_t OnlyFirstEngineManager::SetParameter(const std::string &key, const std::string &value)
{
    if (wakeupEngine_ != nullptr) {
        std::string keyValue = key;
        keyValue.append("=").append(value);
        wakeupEngine_->SetParameter(keyValue);
    }

    return 0;
}

std::string OnlyFirstEngineManager::GetParameter(const std::string &key)
{
    std::string val = "";

    if (wakeupEngine_ != nullptr) {
        val = wakeupEngine_->GetParameter(key);
    }

    return val;
}

void OnlyFirstEngineManager::EngineOnDetected(int32_t uuid)
{
    if (wakeupEngine_ != nullptr) {
        INTELL_VOICE_LOG_INFO("on engine detected, uuid:%{public}d", uuid);
        wakeupEngine_->OnDetected(uuid);
    }
}

sptr<IIntellVoiceEngine> OnlyFirstEngineManager::CreateEngineInner(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("create engine enter, type: %{public}d", type);
    OHOS::IntellVoiceUtils::MemoryGuard memoryGuard;
    if (wakeupEngine_ != nullptr) {
        return wakeupEngine_;
    }

    wakeupEngine_ = SptrFactory<EngineBase, OnlyFirstWakeupEngineObj>::CreateInstance();
    if (wakeupEngine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create engine failed, type:%{public}d", type);
        return nullptr;
    }

    INTELL_VOICE_LOG_INFO("create engine ok");
    return wakeupEngine_;
}

sptr<IIntellVoiceEngine> OnlyFirstEngineManager::CreateEngine(IntellVoiceEngineType type, const std::string &param)
{
    INTELL_VOICE_LOG_INFO("enter, type:%{public}d", type);
    if (type != INTELL_VOICE_WAKEUP) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return nullptr;
    }

    return CreateEngineInner(type);
}

bool OnlyFirstEngineManager::CreateOrResetWakeupEngine()
{
    if (wakeupEngine_ != nullptr) {
        INTELL_VOICE_LOG_INFO("wakeup engine is existed");
        wakeupEngine_->ReleaseAdapter();
        if (!wakeupEngine_->ResetAdapter()) {
            INTELL_VOICE_LOG_ERROR("failed to reset adapter");
            return false;
        }
    } else {
        if (CreateEngineInner(INTELL_VOICE_WAKEUP) == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to create wakeup engine");
            return false;
        }
    }
    return true;
}

int32_t OnlyFirstEngineManager::ReleaseEngineInner(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter, type:%{public}d", type);
    if (type != INTELL_VOICE_WAKEUP) {
        INTELL_VOICE_LOG_ERROR("invalid type:%{public}d", type);
        return 0;
    }

    // hap only releases Napi, and does not release sa resources
    OHOS::IntellVoiceUtils::MemoryGuard memoryGuard;
    if (wakeupEngine_ != nullptr) {
        wakeupEngine_->Detach();
        wakeupEngine_ = nullptr;
    }

    return 0;
}

int32_t OnlyFirstEngineManager::ServiceStopProc()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (wakeupEngine_ != nullptr) {
        INTELL_VOICE_LOG_INFO("clear wakeup engine callback");
        wakeupEngine_->Detach();
    }

    return 0;
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS