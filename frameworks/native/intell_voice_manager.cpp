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

#include <chrono>

#include "intell_voice_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "intell_voice_log.h"
#include "intell_voice_load_callback.h"
#include "i_intell_voice_service.h"
#include "intell_voice_service_proxy.h"

using namespace std;
using namespace OHOS::IntellVoiceEngine;
#define LOG_TAG "IntellVoiceManager"

namespace OHOS {
namespace IntellVoice {
static sptr<IIntellVoiceService> g_sProxy = nullptr;
constexpr int32_t LOADSA_TIMEOUT_MS = 5000;
std::mutex IntellVoiceManager::mutex_;
std::condition_variable IntellVoiceManager::proxyConVar_;

IntellVoiceManager::IntellVoiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
}

IntellVoiceManager::~IntellVoiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
}

IntellVoiceManager *IntellVoiceManager::GetInstance()
{
    static IntellVoiceManager manager;
    return &manager;
}

bool IntellVoiceManager::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("init start");

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("get sa manager failed");
        return false;
    }

    sptr<IntellVoiceLoadCallback> loadCallback = new (std::nothrow) IntellVoiceLoadCallback(
        std::bind(&IntellVoiceManager::LoadSystemAbilitySuccess, this, std::placeholders::_1),
        std::bind(&IntellVoiceManager::LoadSystemAbilityFail, this));
    if (loadCallback == nullptr) {
        INTELL_VOICE_LOG_ERROR("loadCallback is nullptr");
        return false;
    }

    int32_t ret = samgr->LoadSystemAbility(INTELL_VOICE_SERVICE_ID, loadCallback);
    if (ret != ERR_OK) {
        INTELL_VOICE_LOG_ERROR("Failed to load systemAbility");
        return false;
    }

    auto waitStatus = proxyConVar_.wait_for(lock, std::chrono::milliseconds(LOADSA_TIMEOUT_MS));
    if (waitStatus == std::cv_status::no_timeout) {
        INTELL_VOICE_LOG_INFO("Load systemAbility success");
        return true;
    }

    INTELL_VOICE_LOG_ERROR("Load systemAbility timeout");
    return false;
}

void IntellVoiceManager::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &object)
{
    std::unique_lock<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("IntellVoiceManager finish start systemAbility");
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("get system ability failed");
        return;
    }
    g_sProxy = iface_cast<IIntellVoiceService>(object);
    if (g_sProxy != nullptr) {
        INTELL_VOICE_LOG_INFO("init Service Proxy success");
    }
    proxyConVar_.notify_one();
}

void IntellVoiceManager::LoadSystemAbilityFail()
{
    std::unique_lock<std::mutex> lock(mutex_);
    g_sProxy = nullptr;
    proxyConVar_.notify_one();
}

int32_t IntellVoiceManager::CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }
    return g_sProxy->CreateIntellVoiceEngine(type, inst);
}

int32_t IntellVoiceManager::ReleaseIntellVoiceEngine(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }
    return g_sProxy->ReleaseIntellVoiceEngine(type);
}
}
}