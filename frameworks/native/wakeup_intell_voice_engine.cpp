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

#include "wakeup_intell_voice_engine.h"
#include "i_intell_voice_engine.h"
#include "intell_voice_manager.h"
#include "intell_voice_log.h"

#define LOG_TAG "WakeupIntellVoiceEngine"

using namespace std;
using namespace OHOS::IntellVoiceEngine;

namespace OHOS {
namespace IntellVoice {
WakeupIntellVoiceEngine::WakeupIntellVoiceEngine(const WakeupIntelligentVoiceEngineDescriptor &descriptor)
{
    INTELL_VOICE_LOG_INFO("enter");

    descriptor_ = make_unique<WakeupIntelligentVoiceEngineDescriptor>();
    descriptor_->needReconfirm = descriptor.needReconfirm;
    descriptor_->wakeupPhrase = descriptor.wakeupPhrase;
    IntellVoiceManager::GetInstance()->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine_);
}

WakeupIntellVoiceEngine::~WakeupIntellVoiceEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

int32_t WakeupIntellVoiceEngine::SetSensibility(const int32_t &sensibility)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    string keyValueList = "Sensibility=" + to_string(sensibility);
    return engine_->SetParameter(keyValueList);
}

int32_t WakeupIntellVoiceEngine::SetWakeupHapInfo(const WakeupHapInfo &info)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t ret;
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    ret = engine_->SetParameter("wakeup_bundle_name=" + info.bundleName);
    ret += engine_->SetParameter("wakeup_ability_name=" + info.abilityName);
    return ret;
}

int32_t WakeupIntellVoiceEngine::SetParameter(const string &key, const string &value)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    string keyValueList = key + "=" + value;
    return engine_->SetParameter(keyValueList);
}

int32_t WakeupIntellVoiceEngine::Release()
{
    INTELL_VOICE_LOG_INFO("enter");
    return 0;
}

int32_t WakeupIntellVoiceEngine::SetCallback(shared_ptr<IIntellVoiceEngineEventCallback> callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }

    callback_ = sptr<EngineCallbackInner>(new (std::nothrow) EngineCallbackInner(callback));
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return -1;
    }
    engine_->SetCallback(callback_->AsObject());
    return 0;
}

int32_t WakeupIntellVoiceEngine::StartCapturer(int32_t channels)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    return engine_->StartCapturer(channels);
}

int32_t WakeupIntellVoiceEngine::Read(std::vector<uint8_t> &data)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_RET(engine_ == nullptr, -1, "engine is null");

    return engine_->Read(data);
}

int32_t WakeupIntellVoiceEngine::StopCapturer()
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_RET(engine_ == nullptr, -1, "engine is null");

    return engine_->StopCapturer();
}

int32_t WakeupIntellVoiceEngine::GetWakeupPcm(std::vector<uint8_t> &data)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_RET(engine_ == nullptr, -1, "engine is null");

    return engine_->GetWakeupPcm(data);
}

int32_t WakeupIntellVoiceEngine::NotifyHeadsetWakeEvent()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    return engine_->NotifyHeadsetWakeEvent();
}

int32_t WakeupIntellVoiceEngine::NotifyHeadsetHostEvent(int32_t event)
{
    INTELL_VOICE_LOG_INFO("enter, event:%{public}d", event);
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    return engine_->NotifyHeadsetHostEvent(static_cast<OHOS::IntellVoiceEngine::HeadsetHostEventType>(event));
}
}
}