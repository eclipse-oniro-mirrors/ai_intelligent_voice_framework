/*
 * Copyright (c) 2023 Huawei Device Co., Ltd. 2023-2023.
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
#include <fstream>
#include <thread>
#include "securec.h"
#include "intell_voice_log.h"
#include "update_adapter_listener.h"
#include "time_util.h"
#include "scope_guard.h"
#include "adapter_callback_service.h"
#include "engine_callback_message.h"
#include "history_info_mgr.h"
#include "update_engine_utils.h"
#include "engine_host_manager.h"
#include "intell_voice_definitions.h"
#include "update_engine.h"

#define LOG_TAG "UpdateEngine"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;

namespace OHOS {
namespace IntellVoiceEngine {
UpdateEngine::UpdateEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

UpdateEngine::~UpdateEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
    callback_ = nullptr;
}

void UpdateEngine::OnCommitEnrollComplete(int32_t result)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("commit enroll complete, result %{public}d", result);
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_INFO("already detach");
        return;
    }

    updateResult_ = (result == 0 ? UpdateState::UPDATE_STATE_COMPLETE_SUCCESS :
        UpdateState::UPDATE_STATE_COMPLETE_FAIL);
    if (updateResult_ == UpdateState::UPDATE_STATE_COMPLETE_SUCCESS) {
        ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        /* save new version number */
        UpdateEngineUtils::SaveWakeupVesion();
        std::string wakeupPhrase = EngineUtil::GetParameter("wakeup_phrase");
        if (wakeupPhrase.empty()) {
            INTELL_VOICE_LOG_ERROR("wakeup phrase is empty");
        } else {
            HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_PHRASE, wakeupPhrase);
        }

        INTELL_VOICE_LOG_INFO("update save version");
    }

    EngineCallbackMessage::CallFunc(HANDLE_UPDATE_COMPLETE, static_cast<int32_t>(updateResult_), param_);
}

void UpdateEngine::OnUpdateEvent(int32_t msgId, int32_t result)
{
    if (msgId == INTELL_VOICE_ENGINE_MSG_COMMIT_ENROLL_COMPLETE) {
        wptr<UpdateEngine> thisWptr(this);
        std::thread([thisWptr, result]() {
            sptr<UpdateEngine> thisSptr = thisWptr.promote();
            if (thisSptr != nullptr) {
                thisSptr->OnCommitEnrollComplete(result);
            } else {
                INTELL_VOICE_LOG_WARN("updateEngine is null");
            }
        }).detach();
    }
}

bool UpdateEngine::Init(const std::string &param, bool reEnroll)
{
    if (!EngineUtil::CreateAdapterInner(EngineHostManager::GetInstance(), UPDATE_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return false;
    }

    if (param == "WhisperVprUpdate") {
        EngineUtil::SetParameter("WhisperVpr=true");
    }
    if (reEnroll) {
        EngineUtil::SetParameter("WhisperReEnroll=true");
    }

    SetCallback(nullptr);
    EngineUtil::SetLanguage();
    EngineUtil::SetArea();
    SetDspFeatures();

    if (!param.empty()) {
        EngineUtil::SetImproveParam();
        SetParameter(param);
    }

    IntellVoiceEngineInfo info = {
        .wakeupPhrase = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WAKEUP_PHRASE),
        .minBufSize = 1280,
        .sampleChannels = 1,
        .bitsPerSample = 16,
        .sampleRate = 16000,
    };
    int ret = Attach(info);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("attach err");
        EngineUtil::ReleaseAdapterInner(EngineHostManager::GetInstance());
        return false;
    }

    param_= param;
    return true;
}

void UpdateEngine::SetCallback(sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);

    callback_ = sptr<IIntellVoiceEngineCallback>(new (std::nothrow) UpdateAdapterListener(
        std::bind(&UpdateEngine::OnUpdateEvent, this, std::placeholders::_1, std::placeholders::_2)));
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return;
    }

    adapter_->SetCallback(callback_);
}

int32_t UpdateEngine::Attach(const IntellVoiceEngineInfo &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("attach");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    IntellVoiceEngineAdapterInfo adapterInfo = {
        .wakeupPhrase = info.wakeupPhrase,
        .minBufSize = info.minBufSize,
        .sampleChannels = info.sampleChannels,
        .bitsPerSample = info.bitsPerSample,
        .sampleRate = info.sampleRate,
    };
    return adapter_->Attach(adapterInfo);
}

int32_t UpdateEngine::Detach(void)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_WARN("already detach");
        return 0;
    }

    int ret =  adapter_->Detach();
    EngineUtil::ReleaseAdapterInner(EngineHostManager::GetInstance());

    if (updateResult_ == UpdateState::UPDATE_STATE_DEFAULT) {
        INTELL_VOICE_LOG_WARN("detach defore receive commit enroll msg");
        EngineCallbackMessage::CallFunc(HANDLE_UPDATE_COMPLETE,
            static_cast<int32_t>(UpdateState::UPDATE_STATE_DEFAULT), param_);
    }
    return ret;
}

int32_t UpdateEngine::Start(bool isLast)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    return 0;
}

int32_t UpdateEngine::Stop()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    return EngineUtil::Stop();
}

int32_t UpdateEngine::SetParameter(const std::string &keyValueList)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    return EngineUtil::SetParameter(keyValueList);
}

std::string UpdateEngine::GetParameter(const std::string &key)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    return EngineUtil::GetParameter(key);
}
}
}