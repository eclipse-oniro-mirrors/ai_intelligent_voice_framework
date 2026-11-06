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
#include "adapter_host_manager.h"
#include "engine_host_manager.h"

#include "intell_voice_log.h"
#include "intell_voice_util.h"

#define LOG_TAG "AdapterHostManager"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
AdapterHostManager::~AdapterHostManager()
{
    adapterProxy1_0_ = nullptr;
    adapterProxy1_2_ = nullptr;
}

bool AdapterHostManager::Init(const IntellVoiceEngineAdapterDescriptor &desc)
{
    INTELL_VOICE_LOG_INFO("enter");
    const auto &engineHostProxy1_0 = EngineHostManager::GetInstance().GetEngineHostProxy1_0();
    const auto &engineHostProxy1_2 = EngineHostManager::GetInstance().GetEngineHostProxy1_2();
    const auto &engineHostProxy1_3 = EngineHostManager::GetInstance().GetEngineHostProxy1_3();
    CHECK_CONDITION_RETURN_FALSE(engineHostProxy1_0 == nullptr, "engine host proxy_v1_0 is null");

    if (engineHostProxy1_3 != nullptr) {
        OHOS::HDI::IntelligentVoice::Engine::V1_3::IntellVoiceEngineAdapterDescriptor descriptor;
        descriptor.adapterType
            = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_3::IntellVoiceEngineAdapterType>(desc.adapterType);
        engineHostProxy1_3->CreateAdapter_V_3(descriptor, adapterProxy1_2_);
        if (adapterProxy1_2_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("engine adapter proxy1_2_ is nullptr");
            return false;
        }
        adapterProxy1_0_ = adapterProxy1_2_;
        return true;
    }
    if (engineHostProxy1_2 != nullptr) {
        engineHostProxy1_2->CreateAdapter_V_2(desc, adapterProxy1_2_);
        if (adapterProxy1_2_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("engine adapter proxy1_2_ is nullptr");
            return false;
        }

        adapterProxy1_0_ = adapterProxy1_2_;
        return true;
    }

    engineHostProxy1_0->CreateAdapter(desc, adapterProxy1_0_);
    if (adapterProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine adapter proxy1_0_ is nullptr");
        return false;
    }
    return true;
}

int32_t AdapterHostManager::SetCallback(const sptr<IIntellVoiceEngineCallback> &engineCallback)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->SetCallback(engineCallback);
}

int32_t AdapterHostManager::Attach(const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterInfo &info)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->Attach(info);
}

int32_t AdapterHostManager::Detach()
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->Detach();
}

int32_t AdapterHostManager::SetParameter(const std::string &keyValueList)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->SetParameter(keyValueList);
}

int32_t AdapterHostManager::GetParameter(const std::string &keyList, std::string &valueList)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->GetParameter(keyList, valueList);
}

int32_t AdapterHostManager::Start(const OHOS::HDI::IntelligentVoice::Engine::V1_0::StartInfo &info)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->Start(info);
}

int32_t AdapterHostManager::Stop()
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->Stop();
}

int32_t AdapterHostManager::WriteAudio(const std::vector<uint8_t> &buffer)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->WriteAudio(buffer);
}

int32_t AdapterHostManager::Read(OHOS::HDI::IntelligentVoice::Engine::V1_0::ContentType type, sptr<Ashmem> &buffer)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_0_ == nullptr, -1, "adapter proxy is null");
    return adapterProxy1_0_->Read(type, buffer);
}

int32_t AdapterHostManager::GetWakeupPcm(std::vector<uint8_t> &data)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_2_ == nullptr, -1, "adapter proxy is null");
    if (adapterProxy1_2_ == nullptr) {
        return -1;
    }

    return adapterProxy1_2_->GetWakeupPcm(data);
}

int32_t AdapterHostManager::Evaluate(const std::string &word, EvaluationResultInfo &info)
{
    CHECK_CONDITION_RETURN_RET(adapterProxy1_2_ == nullptr, -1, "adapter proxy is null");
    if (adapterProxy1_2_ == nullptr) {
        return -1;
    }
    return adapterProxy1_2_->Evaluate(word, info);
}
}
}
