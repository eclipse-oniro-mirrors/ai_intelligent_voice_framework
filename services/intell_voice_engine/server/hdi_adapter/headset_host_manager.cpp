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
#include "headset_host_manager.h"

#include "iproxy_broker.h"
#include "intell_voice_log.h"
#include "data_operation_callback.h"
#include "intell_voice_util.h"
#include "intell_voice_definitions.h"
#include "intell_voice_engine_manager.h"
#include "headset_adapter_host_manager.h"
#include "headset_wakeup_wrapper.h"
#include "engine_callback_message.h"
#include "engine_device_manager.h"

#undef LOG_TAG
#define LOG_TAG "HeadsetHostManager"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {

HeadsetHostManager::~HeadsetHostManager()
{
    headsetHostProxy1_0_ = nullptr;
    engineHdiDeathRecipient_ = nullptr;
}

bool HeadsetHostManager::Init()
{
    INTELL_VOICE_LOG_INFO("enter");
    headsetHostProxy1_0_ =
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager::Get(HEADSET_SERVICE_NAME);
    if (headsetHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("headsetHostProxy1_0_ is nullptr");
        return false;
    }

    return true;
}

bool HeadsetHostManager::RegisterEngineHDIDeathRecipient()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (headsetHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get engine manager");
        return false;
    }
    sptr<IRemoteObject> object = OHOS::HDI::hdi_objcast<
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager>(headsetHostProxy1_0_);
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is nullptr");
        return false;
    }

    engineHdiDeathRecipient_ = new (std::nothrow) IntellVoiceDeathRecipient(
        std::bind(&HeadsetHostManager::OnEngineHDIDiedCallback));
    if (engineHdiDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create death recipient failed");
        return false;
    }

    return object->AddDeathRecipient(engineHdiDeathRecipient_);
}

void HeadsetHostManager::DeregisterEngineHDIDeathRecipient()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHdiDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("recipient is nullptr");
        return;
    }

    if (headsetHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("headsetHostProxy1_0_ is nullptr");
        return;
    }

    sptr<IRemoteObject> object = OHOS::HDI::hdi_objcast<
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager>(headsetHostProxy1_0_);
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is nullptr");
        return;
    }

    object->RemoveDeathRecipient(engineHdiDeathRecipient_);
}

std::shared_ptr<IAdapterHostManager> HeadsetHostManager::CreateEngineAdapter(
    const IntellVoiceEngineAdapterDescriptor &desc)
{
    if (headsetHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("headsetHostProxy1_0_ is nullptr");
        return nullptr;
    }

    auto adapter = std::make_shared<HeadsetAdapterHostManager>();
    if (adapter == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create engine adapter");
        return nullptr;
    }

    if (!adapter->Init(desc)) {
        return nullptr;
    }

    return adapter;
}

void HeadsetHostManager::ReleaseEngineAdapter(const IntellVoiceEngineAdapterDescriptor &desc)
{
    if (headsetHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("headsetHostProxy1_0_ is nullptr");
        return;
    }
    headsetHostProxy1_0_->ReleaseAdapter(desc);
}

void HeadsetHostManager::OnEngineHDIDiedCallback()
{
    INTELL_VOICE_LOG_INFO("enter");
    EngineDeviceManager::GetInstance().RemoveByServiceName("aam_connection_svc");
}
}
}