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

#include "enroll_intell_voice_engine.h"
#include "i_intell_voice_engine.h"
#include "intell_voice_manager.h"
#include "intell_voice_log.h"

#define LOG_TAG "EnrollIntellVoiceEngine"
using namespace std;
using namespace OHOS::IntellVoiceEngine;

namespace OHOS {
namespace IntellVoice {
static constexpr int32_t MIN_BUFFER_SIZE = 1280; // 16 * 2 * 40ms
static constexpr int32_t CHANNEL_CNT = 1;
static constexpr int32_t BITS_PER_SAMPLE = 16;
static constexpr int32_t SAMPLE_RATE = 16000;

EnrollIntellVoiceEngine::EnrollIntellVoiceEngine(const EnrollIntelligentVoiceEngineDescriptor &descriptor)
{
    INTELL_VOICE_LOG_INFO("enter");

    descriptor_ = make_unique<EnrollIntelligentVoiceEngineDescriptor>();
    if (descriptor_ != nullptr) {
        descriptor_->wakeupPhrase = descriptor.wakeupPhrase;
    }
    auto mgr = IntellVoiceManager::GetInstance();
    if (mgr != nullptr) {
        result_ = mgr->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine_);
        if (result_ != INTELLIGENT_VOICE_SUCCESS) {
            INTELL_VOICE_LOG_ERROR("create enroll engine failed, ret:%{public}d", result_);
        }
    } else {
        INTELL_VOICE_LOG_ERROR("mgr is nullptr");
        result_ = INTELLIGENT_VOICE_SYSTEM_ERROR;
    }
}

EnrollIntellVoiceEngine::~EnrollIntellVoiceEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
    auto mgr = IntellVoiceManager::GetInstance();
    if (mgr != nullptr) {
        mgr->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
    }
}

int32_t EnrollIntellVoiceEngine::Init(const EngineConfig &config)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceEngine is null");
        return -1;
    }

    engine_->SetParameter("language=" + config.language);
    engine_->SetParameter("area=" + config.region);
    if (!config.enrollScenario.empty()) {
        engine_->SetParameter("enrollScenario=" + config.enrollScenario);
    }

    IntellVoiceEngineInfo info = {};
    if (descriptor_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("descriptor_ is nullptr");
        return -1;
    }
    info.wakeupPhrase = descriptor_->wakeupPhrase;
    info.isPcmFromExternal = false;
    info.minBufSize = MIN_BUFFER_SIZE;
    info.sampleChannels = CHANNEL_CNT;
    info.bitsPerSample = BITS_PER_SAMPLE;
    info.sampleRate = SAMPLE_RATE;

    return engine_->Attach(info);
}

int32_t EnrollIntellVoiceEngine::Start(const bool &isLast)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    return engine_->Start(isLast);
}

int32_t EnrollIntellVoiceEngine::Stop()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    return engine_->Stop();
}

int32_t EnrollIntellVoiceEngine::Commit()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    string keyValueList = "CommitEnrollment=true";
    return engine_->SetParameter(keyValueList);
}

int32_t EnrollIntellVoiceEngine::SetSensibility(const int32_t &sensibility)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    string keyValueList = "Sensibility=" + to_string(sensibility);
    return engine_->SetParameter(keyValueList);
}

int32_t EnrollIntellVoiceEngine::SetWakeupHapInfo(const WakeupHapInfo &info)
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

int32_t EnrollIntellVoiceEngine::SetParameter(const string &key, const string &value)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }
    string keyValueList = key + "=" + value;
    return engine_->SetParameter(keyValueList);
}

std::string EnrollIntellVoiceEngine::GetParameter(const std::string &key)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return "";
    }
    return engine_->GetParameter(key);
}

int32_t EnrollIntellVoiceEngine::Evaluate(const std::string &word, EvaluationResult &result)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (descriptor_ != nullptr) {
        descriptor_->wakeupPhrase = word;
    }
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine_ is nullptr");
        return -1;
    }
    return engine_->Evaluate(word, result);
}

int32_t EnrollIntellVoiceEngine::Release()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        return -1;
    }

    return engine_->Detach();
}

int32_t EnrollIntellVoiceEngine::SetCallback(shared_ptr<IIntellVoiceEngineEventCallback> callback)
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
}
}