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
#include "engine_host_manager.h"

#include "iproxy_broker.h"
#include "intell_voice_log.h"
#include "data_operation_callback.h"
#include "intell_voice_util.h"
#include "adapter_host_manager.h"

#define LOG_TAG "EngineHostManager"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t MINOR_VERSION_2 = 2;
static constexpr uint32_t MINOR_VERSION_3 = 3;

EngineHostManager::~EngineHostManager()
{
    engineHostProxy1_0_ = nullptr;
    engineHostProxy1_1_ = nullptr;
    engineHostProxy1_2_ = nullptr;
    engineHdiDeathRecipient_ = nullptr;
    dataOprCb_ = nullptr;
}

bool EngineHostManager::Init()
{
    INTELL_VOICE_LOG_INFO("enter");
    engineHostProxy1_0_ = OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager::Get();
    if (engineHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_0_ is nullptr");
        return false;
    }

    uint32_t majorVer = 0;
    uint32_t minorVer = 0;
    engineHostProxy1_0_->GetVersion(majorVer, minorVer);
    INTELL_VOICE_LOG_INFO("major ver is %{public}u, minor ver is %{public}u", majorVer, minorVer);
    if (IntellVoiceUtil::GetHdiVersionId(majorVer, minorVer) == IntellVoiceUtil::GetHdiVersionId(1, 1)) {
        INTELL_VOICE_LOG_INFO("version is 1.1");
        auto castResult_V1_1 =
            OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceEngineManager::CastFrom(engineHostProxy1_0_);
        if (castResult_V1_1 != nullptr) {
            engineHostProxy1_1_ = castResult_V1_1;
        }
    } else if (IntellVoiceUtil::GetHdiVersionId(majorVer, minorVer) ==
        IntellVoiceUtil::GetHdiVersionId(1, MINOR_VERSION_2)) {
        INTELL_VOICE_LOG_INFO("version is 1.2");
        auto castResult_V1_2 =
            OHOS::HDI::IntelligentVoice::Engine::V1_2::IIntellVoiceEngineManager::CastFrom(engineHostProxy1_0_);
        if (castResult_V1_2 != nullptr) {
            engineHostProxy1_2_ = castResult_V1_2;
            engineHostProxy1_1_ = engineHostProxy1_2_;
        }
    } else if (IntellVoiceUtil::GetHdiVersionId(majorVer, minorVer) ==
        IntellVoiceUtil::GetHdiVersionId(1, MINOR_VERSION_3)) {
        INTELL_VOICE_LOG_INFO("version is 1.3");
        auto castResult_V1_3 =
            OHOS::HDI::IntelligentVoice::Engine::V1_3::IIntellVoiceEngineManager::CastFrom(engineHostProxy1_0_);
        if (castResult_V1_3 != nullptr) {
            engineHostProxy1_3_ = castResult_V1_3;
            engineHostProxy1_2_ = engineHostProxy1_3_;
            engineHostProxy1_1_ = engineHostProxy1_3_;
        }
    }

    return true;
}

bool EngineHostManager::RegisterEngineHDIDeathRecipient()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get engine manager");
        return false;
    }
    sptr<IRemoteObject> object = OHOS::HDI::hdi_objcast<
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager>(engineHostProxy1_0_);
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is nullptr");
        return false;
    }

    engineHdiDeathRecipient_ = new (std::nothrow) IntellVoiceDeathRecipient(
        std::bind(&EngineHostManager::OnEngineHDIDiedCallback));
    if (engineHdiDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create death recipient failed");
        return false;
    }

    return object->AddDeathRecipient(engineHdiDeathRecipient_);
}

void EngineHostManager::DeregisterEngineHDIDeathRecipient()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHdiDeathRecipient_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("recipient is nullptr");
        return;
    }

    if (engineHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_0_ is nullptr");
        return;
    }

    sptr<IRemoteObject> object = OHOS::HDI::hdi_objcast<
        OHOS::HDI::IntelligentVoice::Engine::V1_0::IIntellVoiceEngineManager>(engineHostProxy1_0_);
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is nullptr");
        return;
    }

    object->RemoveDeathRecipient(engineHdiDeathRecipient_);
}

void EngineHostManager::SetDataOprCallback()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (dataOprCb_ != nullptr) {
        INTELL_VOICE_LOG_INFO("already set data opr callback");
        return;
    }

    if (engineHostProxy1_1_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_1_ is nullptr");
        return;
    }

    dataOprCb_ = sptr<OHOS::HDI::IntelligentVoice::Engine::V1_1::IIntellVoiceDataOprCallback>(
        new (std::nothrow) DataOperationCallback());
    if (dataOprCb_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create data opr callback failed");
        return;
    }

    engineHostProxy1_1_->SetDataOprCallback(dataOprCb_);
}

std::shared_ptr<IAdapterHostManager> EngineHostManager::CreateEngineAdapter(
    const IntellVoiceEngineAdapterDescriptor &desc)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_0_ is nullptr");
        return nullptr;
    }

    auto adapter = std::make_shared<AdapterHostManager>();
    if (adapter == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create engine adapter");
        return nullptr;
    }

    if (!adapter->Init(desc)) {
        return nullptr;
    }

    return adapter;
}

void EngineHostManager::ReleaseEngineAdapter(const IntellVoiceEngineAdapterDescriptor &desc)
{
    if (engineHostProxy1_0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_0_ is nullptr");
        return;
    }
    engineHostProxy1_0_->ReleaseAdapter(desc);
}


int EngineHostManager::GetUploadFiles(int numMax, std::vector<UploadHdiFile> &files)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_2_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_2_ is nullptr");
        return -1;
    }

    return engineHostProxy1_2_->GetUploadFiles(numMax, files);
}

void EngineHostManager::OnEngineHDIDiedCallback()
{
    INTELL_VOICE_LOG_INFO("receive engine hdi death recipient");
    _Exit(0);
}

int32_t EngineHostManager::GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_2_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_2_ is nullptr");
        return -1;
    }

    return engineHostProxy1_2_->GetCloneFilesList(cloneFiles);
}

int32_t EngineHostManager::GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_2_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_2_ is nullptr");
        return -1;
    }

    return engineHostProxy1_2_->GetCloneFile(filePath, buffer);
}

int32_t EngineHostManager::SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_2_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_2_ is nullptr");
        return -1;
    }

    return engineHostProxy1_2_->SendCloneFile(filePath, buffer);
}

int32_t EngineHostManager::ClearUserWakeupData(const std::string &wakeupPhrase)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engineHostProxy1_2_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engineHostProxy1_2_ is nullptr");
        return -1;
    }
    return engineHostProxy1_2_->ClearUserWakeupData(wakeupPhrase);
}
}
}
