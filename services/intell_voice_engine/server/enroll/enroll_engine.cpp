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
#include "enroll_engine.h"
#include <fstream>
#include "ipc_skeleton.h"
#include "securec.h"
#include "intell_voice_info.h"
#include "intell_voice_log.h"
#include "intell_voice_util.h"
#include "enroll_adapter_listener.h"
#include "time_util.h"
#include "scope_guard.h"
#include "adapter_callback_service.h"
#include "intell_voice_engine_manager.h"
#include "update_engine_utils.h"
#include "engine_host_manager.h"
#include "history_info_mgr.h"
#include "engine_offline_audio_processor.h"
#include "offline_audio_effect_manager.h"

#define LOG_TAG "EnrollEngine"

using namespace OHOS::IntellVoice;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t MIN_BUFFER_SIZE = 1280; // 16 * 2 * 40ms
static constexpr uint32_t INTERVAL = 125; // 125 * 40ms = 5s
static constexpr uint32_t WITHCMD_INTERVAL = 200; // 200 * 40ms = 8s
static constexpr uint32_t MAX_ENROLL_TASK_NUM = 100;
static const std::string ENROLL_THREAD_NAME = "EnrollEngThread";
static const std::string PCM_PREFIX = "/data/service/el0/intelligent_voice/wakeup/enrollAudioBackup/";

EnrollEngine::EnrollEngine() : TaskExecutor(ENROLL_THREAD_NAME, MAX_ENROLL_TASK_NUM)
{
    INTELL_VOICE_LOG_INFO("enter");

    capturerOptions_.streamInfo.samplingRate = AudioSamplingRate::SAMPLE_RATE_16000;
    capturerOptions_.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    capturerOptions_.streamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    capturerOptions_.streamInfo.channels = AudioChannel::MONO;
    capturerOptions_.capturerInfo.sourceType = SourceType::SOURCE_TYPE_VOICE_RECOGNITION;
    capturerOptions_.capturerInfo.capturerFlags = 0;
    TaskExecutor::StartThread();
}

EnrollEngine::~EnrollEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
    callback_ = nullptr;
}

void EnrollEngine::OnEnrollEvent(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)
{
    if (event.msgId == INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE) {
        TaskExecutor::AddAsyncTask([this, result = event.result, info = event.info]() {
            OnEnrollComplete(result, info);
            }, "EnrollEngine::OnEnrollEvent", false);
    } else if (event.msgId == INTELL_VOICE_ENGINE_MSG_COMMIT_ENROLL_COMPLETE) {
        enrollResult_.store(static_cast<int32_t>(event.result));
        IntellVoiceEngineManager::SetEnrollResult(INTELL_VOICE_ENROLL,
            (static_cast<int32_t>(event.result) == 0 ? true : false));

        bool tse_support = IntellVoiceUtil::GetSupportAbility(ABILITY_TSE);
        if (tse_support) {
            // Create processor instance
            auto processor = std::make_shared<IntelligentVoice::Engine::IntellVoiceEngine::OfflineAudioProcessor>();
            INTELL_VOICE_LOG_INFO("GetSupportAbility_tse_support: true, audio_asr_tse_reg_enter");
            std::string fPath1 = PCM_PREFIX + "enroll_1.pcm";
            std::string fPath2 = PCM_PREFIX + "enroll_2.pcm";
            std::string fPath3 = PCM_PREFIX + "enroll_3.pcm";
            std::string fPath4 = PCM_PREFIX + "enroll_phrase.txt";
            TaskExecutor::AddAsyncTask([processor, fPath1, fPath2, fPath3, fPath4, this]() {
                tseResult_ = processor->ProcessOfflineAudioWhile(fPath1, fPath2, fPath3, fPath4);
            }, "OfflineAudioProcessor::ProcessOfflineAudioWhile", false);
        } else {
            INTELL_VOICE_LOG_INFO("GetSupportAbility_tse_support: false");
        }
    }
}

void EnrollEngine::OnEnrollComplete(int32_t result, const std::string &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent event;
    event.msgId = INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE;
    event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(result);
    event.info = info;
    if (adapterListener_ != nullptr) {
        adapterListener_->Notify(event);
    }
    StopAudioSource();
}

bool EnrollEngine::Init(const std::string &param, bool reEnroll)
{
    if (!EngineUtil::CreateAdapterInner(EngineHostManager::GetInstance(), ENROLL_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return false;
    }
    return true;
}

void EnrollEngine::SetCallback(sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }

    sptr<IIntelligentVoiceEngineCallback> callback = iface_cast<IIntelligentVoiceEngineCallback>(object);
    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback is nullptr");
        return;
    }

    adapterListener_ = std::make_shared<EnrollAdapterListener>(callback,
        std::bind(&EnrollEngine::OnEnrollEvent, this, std::placeholders::_1));
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter listener is nullptr");
        return;
    }

    callback_ = sptr<IIntellVoiceEngineCallback>(new (std::nothrow) AdapterCallbackService(adapterListener_));
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return;
    }

    adapter_->SetCallback(callback_);
}

int32_t EnrollEngine::Attach(const IntellVoiceEngineInfo &info)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("attach");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    SetDspFeatures();
    EngineUtil::SetImproveParam();

    isPcmFromExternal_ = info.isPcmFromExternal;
    wakeupPhrase_ = info.wakeupPhrase;

    IntellVoiceEngineAdapterInfo adapterInfo = {
        .wakeupPhrase = info.wakeupPhrase,
        .minBufSize = info.minBufSize,
        .sampleChannels = info.sampleChannels,
        .bitsPerSample = info.bitsPerSample,
        .sampleRate = info.sampleRate,
    };
    return adapter_->Attach(adapterInfo);
}

int32_t EnrollEngine::Detach(void)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    StopAudioSource();
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_WARN("already detach");
        return 0;
    }

    if (enrollResult_.load() == 0) {
        EngineUtil::ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_PHRASE, wakeupPhrase_);
        /* save new version number */
        UpdateEngineUtils::SaveWakeupVesion();
        INTELL_VOICE_LOG_INFO("enroll save version");
    }

    int32_t ret = adapter_->Detach();
    ReleaseAdapterInner(EngineHostManager::GetInstance());
    return ret;
}

int32_t EnrollEngine::Start(bool isLast)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_MICROPHONE_PERMISSION);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        return ret;
    }

    if (audioSource_ != nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSource_ existed, wait for last start to finish");
        return -1;
    }

    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    StartInfo info = {
        .isLast = isLast,
    };
    if (adapter_->Start(info)) {
        INTELL_VOICE_LOG_ERROR("start adapter failed");
        return -1;
    }

    if (isPcmFromExternal_) {
        INTELL_VOICE_LOG_INFO("pcm is from external");
        return 0;
    }

    SelectInputDevice(DeviceType::DEVICE_TYPE_MIC);

    if (!StartAudioSource()) {
        INTELL_VOICE_LOG_ERROR("start audio source failed");
        adapter_->Stop();
        return -1;
    }

    INTELL_VOICE_LOG_INFO("exit");
    return 0;
}

int32_t EnrollEngine::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    StopAudioSource();

    return EngineUtil::Stop();
}

int32_t EnrollEngine::SetParameter(const std::string &keyValueList)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (SetParameterInner(keyValueList)) {
        INTELL_VOICE_LOG_INFO("inner parameter");
        return 0;
    }

    INTELL_VOICE_LOG_INFO("EnrollEngine SetParameter:%{public}s", keyValueList.c_str());
    if (keyValueList == "enrollScenario=withcmd") {
            isWithCmdScenario_ = true;
    }

    if (keyValueList == "asr_ns_mode=NORMAL_REG") {
        isNormalRegMode_ = true;
    }

    if (keyValueList == "asr_ns_mode=TOPMIC_REG") {
        isTopMicRegMode_ = true;
    }
    return EngineUtil::SetParameter(keyValueList);
}

bool EnrollEngine::SetParameterInner(const std::string &keyValueList)
{
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();

    std::map<std::string, std::string> kvpairs;
    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);
    for (auto it : kvpairs) {
        if (it.first == std::string("wakeup_bundle_name")) {
            INTELL_VOICE_LOG_INFO("set wakeup bundle name:%{public}s", it.second.c_str());
            historyInfoMgr.SetStringKVPair(KEY_WAKEUP_ENGINE_BUNDLE_NAME, it.second);
            return true;
        }
        if (it.first == std::string("wakeup_ability_name")) {
            INTELL_VOICE_LOG_INFO("set wakeup ability name:%{public}s", it.second.c_str());
            historyInfoMgr.SetStringKVPair(KEY_WAKEUP_ENGINE_ABILITY_NAME, it.second);
            return true;
        }
        if (it.first == std::string("language")) {
            INTELL_VOICE_LOG_DEBUG("set language:%{public}s", it.second.c_str());
            historyInfoMgr.SetStringKVPair(KEY_LANGUAGE, it.second);
            continue;
        }
        if (it.first == std::string("area")) {
            INTELL_VOICE_LOG_DEBUG("set area:%{public}s", it.second.c_str());
            historyInfoMgr.SetStringKVPair(KEY_AREA, it.second);
            continue;
        }
        if (it.first == std::string("Sensibility")) {
            INTELL_VOICE_LOG_INFO("set Sensibility:%{public}s", it.second.c_str());
            historyInfoMgr.SetStringKVPair(KEY_SENSIBILITY, it.second);
            continue;
        }
    }

    return false;
}

std::string EnrollEngine::GetTSEParameter() {
    if (tseResult_ == 0) {
        return "1";
    } else {
        return "-1";
    }
}

std::string EnrollEngine::GetParameter(const std::string &key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (key == "IS_OWNER_VOICE_ENROLLMENT_COMPLETED") {
        return GetTSEParameter();
    }
    return EngineUtil::GetParameter(key);
}

int32_t EnrollEngine::WriteAudio(const uint8_t *buffer, uint32_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return EngineUtil::WriteAudio(buffer, size);
}

int32_t EnrollEngine::Evaluate(const std::string &word, EvaluationResult &result)
{
    std::lock_guard<std::mutex> lock(mutex_);
    OHOS::HDI::IntelligentVoice::Engine::V1_2::EvaluationResultInfo info;
    int32_t ret = EngineUtil::Evaluate(word, info);
    if (ret == 0) {
        result.score = info.score;
        result.resultCode = info.resultCode;
    }

    return ret;
}

void EnrollEngine::SetParamOnAudioStopOrError()
{
    if (!isNormalRegMode_ && !isTopMicRegMode_) {
        return;
    }
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    INTELL_VOICE_LOG_INFO("set aec_mode and ns_mode bypass");
    audioSystemManager->SetAsrAecMode(AsrAecMode::BYPASS);
    audioSystemManager->SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode::BYPASS);
}

bool EnrollEngine::StartAudioSource()
{
    callerTokenId_ = IPCSkeleton::GetCallingTokenID();
    uint32_t bufferCnt;
    if (!IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
        INTELL_VOICE_PERMISSION_START)) {
        INTELL_VOICE_LOG_ERROR("record permissionPrivacy failed");
        return false;
    }

    auto listener = std::make_unique<AudioSourceListener>([&] (uint8_t *buffer, uint32_t size, bool isEnd) {
        if ((adapter_ != nullptr) && (!isEnd)) {
            std::vector<uint8_t> audioBuff(&buffer[0], &buffer[size]);
            adapter_->WriteAudio(audioBuff);
        }}, [&] () {
            INTELL_VOICE_LOG_INFO("end of pcm");
            if (adapter_ != nullptr) {
                adapter_->SetParameter("end_of_pcm=true");
            }
        });
    if (listener == nullptr) {
        INTELL_VOICE_LOG_ERROR("create listener failed");
        return false;
    }
    if (isWithCmdScenario_)
        bufferCnt = WITHCMD_INTERVAL;
    else
        bufferCnt = INTERVAL;
    audioSource_ = std::make_unique<AudioSource>(MIN_BUFFER_SIZE, bufferCnt, std::move(listener),
        capturerOptions_);
    if (audioSource_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create audio source failed");
        return false;
    }

    if (isNormalRegMode_ || isTopMicRegMode_) {
        auto audioSystemManager = AudioSystemManager::GetInstance();
        if (audioSystemManager != nullptr) {
            INTELL_VOICE_LOG_INFO("set normal reg mode or top mic reg mode");
            audioSystemManager->SetAsrAecMode(AsrAecMode::STANDARD);
            audioSystemManager->SetAsrNoiseSuppressionMode(isNormalRegMode_ ?
                        AsrNoiseSuppressionMode::NORMAL_REG : AsrNoiseSuppressionMode::TOPMIC_REG);
        } else {
            INTELL_VOICE_LOG_ERROR("audioSystemManager is null");
        }
    }

    if (!audioSource_->Start()) {
        INTELL_VOICE_LOG_ERROR("start capturer failed");
        SetParamOnAudioStopOrError();
        audioSource_ = nullptr;
        IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
            INTELL_VOICE_PERMISSION_STOP);
        isNormalRegMode_ = false;
        isTopMicRegMode_ = false;
        return false;
    }
    return true;
}

void EnrollEngine::StopAudioSource()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (audioSource_ != nullptr) {
        INTELL_VOICE_LOG_INFO("stop audio source");
        SetParamOnAudioStopOrError();
        audioSource_->Stop();
        audioSource_ = nullptr;
        IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
            INTELL_VOICE_PERMISSION_STOP);
    }
    isNormalRegMode_ = false;
    isTopMicRegMode_ = false;
}
}
}
