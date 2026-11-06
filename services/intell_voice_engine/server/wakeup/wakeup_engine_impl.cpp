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
#include "wakeup_engine_impl.h"

#include <sys/time.h>
#include "audio_asr.h"
#include "audio_system_manager.h"
#include "audio_policy_manager.h"
#include "ipc_skeleton.h"
#include "v1_2/intell_voice_engine_types.h"
#include "adapter_callback_service.h"
#include "intell_voice_info.h"
#include "intell_voice_log.h"
#include "history_info_mgr.h"
#include "intell_voice_util.h"
#include "string_util.h"
#include "intell_voice_engine_manager.h"
#include "engine_callback_message.h"
#include "engine_host_manager.h"
#include "swing_service_wrapper.h"

#define LOG_TAG "WakeupEngineImpl"

using namespace OHOS::IntellVoice;
using namespace OHOS::AudioStandard;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t MIN_BUFFER_SIZE = 640;
static constexpr uint32_t INTERVAL = 100;
static constexpr int32_t CHANNEL_CNT = 1;
static constexpr int32_t BITS_PER_SAMPLE = 16;
static constexpr int32_t SAMPLE_RATE = 16000;
static const std::string WAKEUP_SOURCE_CHANNEL = "wakeup_source_channel";
static const std::string WAKEUP_MULTI_CHANNEL = "wakeup_multi_channel";
static const std::string IS_CALLBACK_EXIST = "is_callback_exist";
static const std::string WAKEUP_TIMESTAMP = "wakeup_timestamp";
static const std::string TIMESTAMP_SOURCE_START = "source_start";
static const std::string TIMESTAMP_SOURCE_END = "source_end";
static constexpr std::string_view DEFAULT_WAKEUP_PHRASE = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
static constexpr int64_t RECOGNIZING_TIMEOUT_US = 10 * 1000 * 1000; // 10s
static constexpr int64_t RECOGNIZE_COMPLETE_TIMEOUT_US = 2 * 1000 * 1000; // 2s
static constexpr int64_t READ_CAPTURER_TIMEOUT_US = 10 * 1000 * 1000; // 10s
static constexpr uint32_t MAX_WAKEUP_TASK_NUM = 200;
static const std::string WAKEUP_THREAD_NAME = "WakeupEngThread";
static constexpr int64_t SEC_PER_DAY = 86400;
static constexpr int64_t MS_PER_SEC = 1000;       // milliseconds per second
static constexpr int64_t US_PER_MS = 1000;        // microseconds per millisecond

WakeupEngineImpl::WakeupEngineImpl() : ModuleStates(State(IDLE), "WakeupEngineImpl", "WakeupThread"),
    TaskExecutor(WAKEUP_THREAD_NAME, MAX_WAKEUP_TASK_NUM), BaseWakeupEngineImpl()
{
}

bool WakeupEngineImpl::Init()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!InitStates()) {
        INTELL_VOICE_LOG_ERROR("init state failed");
        return false;
    }
    capturerOptions_.streamInfo.samplingRate = AudioSamplingRate::SAMPLE_RATE_16000;
    capturerOptions_.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    capturerOptions_.streamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    capturerOptions_.streamInfo.channels = AudioChannel::MONO;
    capturerOptions_.capturerInfo.sourceType = SourceType::SOURCE_TYPE_WAKEUP;
    capturerOptions_.capturerInfo.capturerFlags = 0;
    adapterListener_ = std::make_shared<WakeupAdapterListener>(
        std::bind(&WakeupEngineImpl::OnWakeupEvent, this, std::placeholders::_1));
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
        return false;
    }
    TaskExecutor::StartThread();
    return true;
}

WakeupEngineImpl::~WakeupEngineImpl()
{
}

bool WakeupEngineImpl::InitStates()
{
    for (int i = IDLE; i <= READ_CAPTURER; i++) {
        ForState(State(i))
            .ACT(SET_LISTENER, HandleSetListener)
            .ACT(SET_PARAM, HandleSetParam)
            .ACT(GET_PARAM, HandleGetParam);
    }

    ForState(IDLE)
        .ACT(INIT, HandleInit)
        .ACT(RESET_ADAPTER, HandleResetAdapter);

    ForState(INITIALIZING)
        .ACT(INIT_DONE, HandleInitDone);

    ForState(INITIALIZED)
        .ACT(START_RECOGNIZE, HandleStart);

    ForState(RECOGNIZING)
        .WaitUntil(RECOGNIZING_TIMEOUT,
            std::bind(&WakeupEngineImpl::HandleRecognizingTimeout, this,
                std::placeholders::_1, std::placeholders::_2),
            RECOGNIZING_TIMEOUT_US)
        .ACT(STOP_RECOGNIZE, HandleStop)
        .ACT(RECOGNIZE_COMPLETE, HandleRecognizeComplete);

    ForState(RECOGNIZED)
        .WaitUntil(RECOGNIZE_COMPLETE_TIMEOUT,
            std::bind(&WakeupEngineImpl::HandleStopCapturerTimeout, this, std::placeholders::_1,
                std::placeholders::_2, OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_RECOGNIZED_COMPLETE),
            RECOGNIZE_COMPLETE_TIMEOUT_US)
        .ACT(GET_WAKEUP_PCM, HandleGetWakeupPcm)
        .ACT(START_CAPTURER, HandleStartCapturer);

    ForState(READ_CAPTURER)
        .WaitUntil(READ_CAPTURER_TIMEOUT,
            std::bind(&WakeupEngineImpl::HandleStopCapturerTimeout, this, std::placeholders::_1,
                std::placeholders::_2, OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_ONESHOT_PROCESSE),
            READ_CAPTURER_TIMEOUT_US)
        .ACT(READ, HandleRead)
        .ACT(STOP_CAPTURER, HandleStopCapturer);

    FromState(INITIALIZING, READ_CAPTURER)
        .ACT(RELEASE_ADAPTER, HandleRelease)
        .ACT(RELEASE, HandleRelease);

    return IsStatesInitSucc();
}

int32_t WakeupEngineImpl::Handle(const StateMsg &msg)
{
    if (!IsStatesInitSucc()) {
        INTELL_VOICE_LOG_ERROR("failed to init state");
        return -1;
    }

    return ModuleStates::HandleMsg(msg);
}

OHOS::AudioStandard::AudioChannel WakeupEngineImpl::GetWakeupSourceChannel()
{
    auto ret = EngineCallbackMessage::CallFunc(TRIGGERMGR_GET_PARAMETER, WAKEUP_SOURCE_CHANNEL);
    std::string channel = "";
    if (ret.has_value()) {
        try {
            channel = std::any_cast<std::string>(*ret);
        } catch (const std::bad_any_cast&) {
            INTELL_VOICE_LOG_ERROR("msg bus bad any cast");
            return AudioChannel::MONO;
        }
    } else {
        INTELL_VOICE_LOG_ERROR("msg bus return no value");
        return AudioChannel::MONO;
    }

    if (channel == "") {
        INTELL_VOICE_LOG_INFO("no channle info");
        return AudioChannel::MONO;
    }

    int32_t channelCnt = 0;
    if (!StringUtil::StringToInt(channel, channelCnt)) {
        INTELL_VOICE_LOG_ERROR("failed to get channel cnt");
        return AudioChannel::MONO;
    }

    if ((channelCnt < AudioChannel::MONO) || (channelCnt > AudioChannel::CHANNEL_4)) {
        INTELL_VOICE_LOG_INFO("invalid channel cnt:%{public}d", channelCnt);
        return AudioChannel::MONO;
    }

    INTELL_VOICE_LOG_INFO("channel cnt:%{public}d", static_cast<int32_t>(channelCnt));
    return static_cast<OHOS::AudioStandard::AudioChannel>(channelCnt);
}

bool WakeupEngineImpl::SetCallbackInner()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return false;
    }

    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter listener is nullptr");
        return false;
    }

    callback_ = sptr<IIntellVoiceEngineCallback>(new (std::nothrow) AdapterCallbackService(adapterListener_));
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return false;
    }

    adapter_->SetCallback(callback_);
    return true;
}

int32_t WakeupEngineImpl::AttachInner(const IntellVoiceEngineInfo &info)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    isPcmFromExternal_ = info.isPcmFromExternal;

    IntellVoiceEngineAdapterInfo adapterInfo = {
        .wakeupPhrase = info.wakeupPhrase,
        .minBufSize = info.minBufSize,
        .sampleChannels = info.sampleChannels,
        .bitsPerSample = info.bitsPerSample,
        .sampleRate = info.sampleRate,
    };
    return adapter_->Attach(adapterInfo);
}

void WakeupEngineImpl::SetParamOnAudioStart(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter");
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }

#ifdef SUPPORT_WINDOW_MANAGER
    SetFoldStatus();
#endif

    audioSystemManager->SetAsrAecMode(AsrAecMode::STANDARD);
    if (uuid == PROXIMAL_WAKEUP_MODEL_UUID) {
        audioSystemManager->SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode::NEAR_FIELD);
    } else {
        audioSystemManager->SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode::STANDARD);
    }
}

void WakeupEngineImpl::SetParamOnAudioStop()
{
    INTELL_VOICE_LOG_INFO("enter");
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    auto audioStreamManager = AudioStreamManager::GetInstance();
    if (audioStreamManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioStreamManager is nullptr");
        return;
    }
    std::vector<std::shared_ptr<AudioCapturerChangeInfo>> audioCapturerChangeInfos;
    int32_t ret = audioStreamManager->GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("GetCurrentCapturerChangeInfos failed");
    }
    for (auto it = audioCapturerChangeInfos.begin(); it != audioCapturerChangeInfos.end(); it++) {
        if ((*it) == nullptr) {
            INTELL_VOICE_LOG_ERROR("AudioCapturerChangeInfo ptr is nullptr");
            continue;
        }
        if ((*it)->capturerInfo.sourceType == SOURCE_TYPE_VOICE_RECOGNITION) {
            INTELL_VOICE_LOG_INFO("SOURCE_TYPE_VOICE_RECOGNITION exists");
            return;
        }
    }
    audioSystemManager->SetAsrAecMode(AsrAecMode::BYPASS);
    audioSystemManager->SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode::BYPASS);
}

bool WakeupEngineImpl::CreateWakeupSourceStopCallback()
{
    if (wakeupSourceStopCallback_ != nullptr) {
        INTELL_VOICE_LOG_INFO("wakeup close cb is already created");
        return true;
    }

    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return false;
    }

    wakeupSourceStopCallback_ = std::make_shared<WakeupSourceStopCallback>();
    if (wakeupSourceStopCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("wakeup source stop callback is nullptr");
        return false;
    }

    audioSystemManager->SetWakeUpSourceCloseCallback(wakeupSourceStopCallback_);
    return true;
}

void WakeupEngineImpl::DestroyWakeupSourceStopCallback()
{
    if (wakeupSourceStopCallback_ == nullptr) {
        INTELL_VOICE_LOG_INFO("wakeup close cb is already destroyed");
        return;
    }

    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }

    audioSystemManager->SetWakeUpSourceCloseCallback(nullptr);
    wakeupSourceStopCallback_ = nullptr;
}

bool WakeupEngineImpl::StartAudioSource()
{
    pcmReadCnt_ = 0;
    capturerOptions_.streamInfo.channels = GetWakeupSourceChannel();
    auto listener = std::make_unique<AudioSourceListener>(
        [&](uint8_t *buffer, uint32_t size, bool isEnd) {
            ReadBufferCallback(buffer, size, isEnd);
        },
        [&]() {
            INTELL_VOICE_LOG_INFO("end of pcm");
            audioSourceEnd_ = get_current_time_ms();
            if (adapter_ != nullptr) {
                adapter_->SetParameter("end_of_pcm=true");
            }
        });
    if (listener == nullptr) {
        INTELL_VOICE_LOG_ERROR("create listener failed");
        return false;
    }

    WakeupSourceProcess::Init(capturerOptions_.streamInfo.channels);

    audioSource_ = std::make_unique<AudioSource>(MIN_BUFFER_SIZE * static_cast<uint32_t>(
        capturerOptions_.streamInfo.channels), INTERVAL, std::move(listener), capturerOptions_);
    if (audioSource_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create audio source failed");
        WakeupSourceProcess::Release();
        return false;
    }

    if (!audioSource_->Start()) {
        INTELL_VOICE_LOG_ERROR("start capturer failed");
        audioSource_ = nullptr;
        WakeupSourceProcess::Release();
        return false;
    }
    audioSourceStart_ = get_current_time_ms();
    return true;
}

void WakeupEngineImpl::StopAudioSource()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (audioSource_ == nullptr) {
        INTELL_VOICE_LOG_INFO("audio source is nullptr, no need to stop");
        return;
    }
    SetParamOnAudioStop();
    audioSource_->Stop();
    audioSource_ = nullptr;
    WakeupSourceProcess::Release();
}

void WakeupEngineImpl::OnWakeupEvent(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)
{
    if (event.msgId == INTELL_VOICE_ENGINE_MSG_INIT_DONE) {
        TaskExecutor::AddAsyncTask([this, result = event.result]() { OnInitDone(result); },
            "WakeupEngineImpl::InitDone", false);
    } else if (event.msgId == INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        TaskExecutor::AddAsyncTask([this, result = event.result, info = event.info]() {
            OnWakeupRecognition(result, info);
            }, "WakeupEngineImpl::RecognizeComplete", false);
    } else {
        INTELL_VOICE_LOG_WARN("invalid msg id:%{public}d", event.msgId);
    }
}

void WakeupEngineImpl::OnInitDone(int32_t result)
{
    INTELL_VOICE_LOG_INFO("on Init Done, result:%{public}d", result);
    StateMsg msg(INIT_DONE, &result, sizeof(int32_t));
    Handle(msg);
}

void WakeupEngineImpl::OnWakeupRecognition(int32_t result, const std::string &info)
{
    INTELL_VOICE_LOG_INFO("on wakeup recognition, result:%{public}d", result);
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent event;
    event.msgId = INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE;
    event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(result);
    event.info = info;
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_RECOGNIZED,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    StateMsg msg(RECOGNIZE_COMPLETE, reinterpret_cast<void *>(&event),
        sizeof(OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent));
    Handle(msg);
}

std::string WakeupEngineImpl::SetTriggerParameter(const std::string &keyValueList)
{
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();
    std::map<std::string, std::string> kvpairs;
    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);

    std::vector<std::string> processedKeys;

    for (const auto &pair : kvpairs) {
        if (std::find(TRIGGER_PARAMS.begin(), TRIGGER_PARAMS.end(), pair.first) != TRIGGER_PARAMS.end()) {
            INTELL_VOICE_LOG_INFO("set trigger %{public}s:%{public}s", pair.first.c_str(), pair.second.c_str());
            historyInfoMgr.SetStringKVPair(pair.first, pair.second);
            EngineCallbackMessage::CallFunc(TRIGGERMGR_SET_PARAMETER, pair.first, pair.second);
            processedKeys.push_back(pair.first);
        }
    }

    std::string newKeyValueList;
    for (const auto &pair : kvpairs) {
        if (std::find(processedKeys.begin(), processedKeys.end(), pair.first) == processedKeys.end()) {
            if (!newKeyValueList.empty()) {
                newKeyValueList += ";";
            }
            newKeyValueList += pair.first + "=" + pair.second;
        }
    }
    return newKeyValueList;
}

int32_t WakeupEngineImpl::HandleSetParam(const StateMsg &msg, State & /* nextState */)
{
    StringParam *param = reinterpret_cast<StringParam *>(msg.inMsg);
    if (param == nullptr) {
        INTELL_VOICE_LOG_INFO("param is nullptr");
        return -1;
    }
    std::string engineParams = SetTriggerParameter(param->strParam);
    if (!engineParams.empty()) {
        return EngineUtil::SetParameter(engineParams);
    }
    return 0;
}

int32_t WakeupEngineImpl::HandleGetParam(const StateMsg &msg, State & /* nextState */)
{
    StringParam *key = reinterpret_cast<StringParam *>(msg.inMsg);
    StringParam *value = reinterpret_cast<StringParam *>(msg.outMsg);
    if ((key == nullptr) || (value == nullptr)) {
        INTELL_VOICE_LOG_INFO("key or value is nullptr");
        return -1;
    }

    if (key->strParam == WAKEUP_SOURCE_CHANNEL) {
        value->strParam = std::to_string(static_cast<uint32_t>(capturerOptions_.streamInfo.channels));
    } else if (key->strParam == IS_CALLBACK_EXIST) {
        if (adapterListener_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
            value->strParam = "false";
        } else {
            value->strParam = adapterListener_->GetCallbackStatus();
        }
    } else if (key->strParam == WAKEUP_TIMESTAMP) {
        auto ret = EngineCallbackMessage::CallFunc(TRIGGERMGR_GET_PARAMETER, WAKEUP_TIMESTAMP);
        std::string timestamp = "";
        if (ret.has_value()) {
            try {
                timestamp = std::any_cast<std::string>(*ret);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("msg bus bad any cast");
                return -1;
            }
        } else {
            INTELL_VOICE_LOG_ERROR("msg bus return no value");
            return -1;
        }
        value->strParam = timestamp + EngineUtil::GetParameter(WAKEUP_TIMESTAMP);
        if (audioSourceStart_ != 0 && audioSourceEnd_ != 0) {
            value->strParam += TIMESTAMP_SOURCE_START + ":" + std::to_string(audioSourceStart_) + ";"
                + TIMESTAMP_SOURCE_END + ":" + std::to_string(audioSourceEnd_) + ";";
            audioSourceStart_ = 0;
            audioSourceEnd_ = 0;
        }
    } else {
        value->strParam = EngineUtil::GetParameter(key->strParam);
    }
    INTELL_VOICE_LOG_INFO("key:%{public}s, value:%{public}s", key->strParam.c_str(), value->strParam.c_str());
    return 0;
}

int32_t WakeupEngineImpl::HandleInit(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!EngineUtil::CreateAdapterInner(EngineHostManager::GetInstance(), WAKEUP_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }

    if (!SetCallbackInner()) {
        INTELL_VOICE_LOG_ERROR("failed to set callback");
        return -1;
    }
    EngineUtil::SetWhisperVpr();
    SetWakeupModel();
    EngineUtil::SetLanguage();
    EngineUtil::SetArea();
    EngineUtil::SetSensibility();
    EngineUtil::SetTriggerParameter();
    EngineUtil::SetImproveParam();
    SetDspFeatures();

    IntellVoiceEngineInfo info = {
        .wakeupPhrase = GetWakeupPhrase(),
        .isPcmFromExternal = false,
        .minBufSize = MIN_BUFFER_SIZE,
        .sampleChannels = CHANNEL_CNT,
        .bitsPerSample = BITS_PER_SAMPLE,
        .sampleRate = SAMPLE_RATE,
    };
    if (AttachInner(info) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to attach");
        return -1;
    }

    UpdateDspModel();

    nextState = State(INITIALIZING);
    return 0;
}

int32_t WakeupEngineImpl::HandleInitDone(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t *result = reinterpret_cast<int32_t *>(msg.inMsg);
    if ((result == nullptr) || (*result != 0)) {
        INTELL_VOICE_LOG_ERROR("init done failed");
        return -1;
    }
    std::thread([]() {
        IntellVoiceUtil::SendCommonEvent("wakeupEngineInited");
    }).detach();

    nextState = State(INITIALIZED);
    return 0;
}

int32_t WakeupEngineImpl::HandleSetListener(const StateMsg &msg, State & /* nextState */)
{
    SetListenerMsg *listenerMsg = reinterpret_cast<SetListenerMsg *>(msg.inMsg);
    if (listenerMsg == nullptr || adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("listenerMsg or adapter listener is nullptr");
        return -1;
    }

    adapterListener_->SetCallback(listenerMsg->callback);
    return 0;
}

int32_t WakeupEngineImpl::HandleStart(const StateMsg &msg, State &nextState)
{
    int32_t *msgBody = reinterpret_cast<int32_t *>(msg.inMsg);
    WakeupErrorCode errCode = OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END;
    std::string multiChannelValue = EngineUtil::GetParameter(WAKEUP_MULTI_CHANNEL);
    isMultiChannel_ = (multiChannelValue == "true");
    INTELL_VOICE_LOG_INFO("isMultiChannel_ is %{public}d", isMultiChannel_);

    if (msgBody == nullptr) {
        INTELL_VOICE_LOG_ERROR("msgBody is nullptr");
        errCode = OHOS::IntellVoiceUtils::SA_INPUT_ERROR;
        goto err;
    }

    if (*msgBody == PROXIMAL_WAKEUP_MODEL_UUID) {
        channelId_ = CHANNEL_ID_1;
        WakeupSourceProcess::SetReadBufferFlag(false);
        EngineUtil::SetParameter("WakeupType=3");
    } else {
        channelId_ = CHANNEL_ID_0;
        WakeupSourceProcess::SetReadBufferFlag(true);
        EngineUtil::SetParameter("WakeupType=0");
    }
    INTELL_VOICE_LOG_INFO("enter, channel id is %{public}d", channelId_);

    EngineUtil::SetScreenStatus();

    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        errCode = OHOS::IntellVoiceUtils::SA_HOST_ERROR;
        goto err;
    }
    {
        StartInfo info = {
            .isLast = true,
        };
        if (adapter_->Start(info)) {
            INTELL_VOICE_LOG_ERROR("start adapter failed");
            errCode = OHOS::IntellVoiceUtils::SA_HOST_ERROR;
            goto err;
        }
    }
    if (!CreateWakeupSourceStopCallback()) {
        INTELL_VOICE_LOG_ERROR("create wakeup source stop callback failed");
        adapter_->Stop();
        errCode = OHOS::IntellVoiceUtils::SA_HOST_ERROR;
        goto err;
    }

    SetParamOnAudioStart(*msgBody);

    if (!StartAudioSource()) {
        INTELL_VOICE_LOG_ERROR("start audio source failed");
        SetParamOnAudioStop();
        adapter_->Stop();
        errCode = OHOS::IntellVoiceUtils::SA_AUDIO_SOURCE_ERROR;
        goto err;
    }

    INTELL_VOICE_LOG_INFO("exit");
    nextState = State(RECOGNIZING);
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_HANDLE_DETECTED,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    return 0;

err:
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_HANDLE_DETECTED,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_FAIL, errCode);
    return -1;
}

int32_t WakeupEngineImpl::HandleStop(const StateMsg & /* msg */, State &nextState)
{
    StopAudioSource();
    EngineUtil::Stop();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t WakeupEngineImpl::HandleRecognizeComplete(const StateMsg &msg, State &nextState)
{
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *event =
        reinterpret_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *>(msg.inMsg);
    if (event == nullptr) {
        INTELL_VOICE_LOG_ERROR("result is nullptr");
        return -1;
    }

    if (adapterListener_ != nullptr) {
        adapterListener_->Notify(*event);
    }

    if (event->result != 0) {
        INTELL_VOICE_LOG_ERROR("wakeup failed");
        StopAudioSource();
        nextState = State(INITIALIZED);
    } else {
        nextState = State(RECOGNIZED);
    }
    EngineUtil::Stop();
    return 0;
}

int32_t WakeupEngineImpl::HandleStartCapturer(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_ONESHOT_START,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    auto ret = IntellVoiceUtil::VerifySystemPermission(OHOS_MICROPHONE_PERMISSION);
    if (ret != INTELLIGENT_VOICE_SUCCESS) {
        return ret;
    }

    int32_t *msgBody = reinterpret_cast<int32_t *>(msg.inMsg);
    if (msgBody == nullptr) {
        INTELL_VOICE_LOG_ERROR("msgBody is nullptr");
        return INTELLIGENT_VOICE_START_CAPTURER_FAILED;
    }
    channels_ = *msgBody;

    callerTokenId_ = IPCSkeleton::GetCallingTokenID();
    if (!IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
        INTELL_VOICE_PERMISSION_START)) {
        INTELL_VOICE_LOG_ERROR("record permissionPrivacy failed");
        return -1;
    }
    nextState = State(READ_CAPTURER);
    return 0;
}

int32_t WakeupEngineImpl::HandleRead(const StateMsg &msg, State & /* nextState */)
{
    CapturerData *capturerData = reinterpret_cast<CapturerData *>(msg.outMsg);
    if (capturerData == nullptr) {
        INTELL_VOICE_LOG_ERROR("capturer data is nullptr");
        return -1;
    }

    auto ret = WakeupSourceProcess::Read(capturerData->data, channels_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("read capturer data failed");
        return ret;
    }

    ResetTimerDelay();
    return 0;
}

int32_t WakeupEngineImpl::HandleStopCapturerTimeout(const StateMsg & msg, State &nextState,
    OHOS::IntellVoiceUtils::StageCode stageCode)
{
    WakeupTrack::reportWakeupTrack(stageCode, OHOS::IntellVoiceUtils::WAKEUP_RESULT_FAIL,
        OHOS::IntellVoiceUtils::SA_TIMEOUT_ERROR);
    return HandleStopCapturer(msg, nextState);
}

int32_t WakeupEngineImpl::HandleStopCapturer(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_ONESHOT_END,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    StopAudioSource();
    EngineUtil::Stop();
    adapterListener_->ClearBackupEvent();
    if (CurrState() == State(READ_CAPTURER)) {
        IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
            INTELL_VOICE_PERMISSION_STOP);
    }
    nextState = State(INITIALIZED);
    return 0;
}

int32_t WakeupEngineImpl::HandleRecognizingTimeout(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    WakeupTrack::reportWakeupTrack(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_RECOGNIZING,
        OHOS::IntellVoiceUtils::WAKEUP_RESULT_FAIL, OHOS::IntellVoiceUtils::SA_TIMEOUT_ERROR);
    StopAudioSource();
    EngineUtil::Stop();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t WakeupEngineImpl::HandleGetWakeupPcm(const StateMsg &msg, State & /* nextState */)
{
    CapturerData *capturerData = reinterpret_cast<CapturerData *>(msg.outMsg);
    if (capturerData == nullptr) {
        INTELL_VOICE_LOG_ERROR("capturer data is nullptr");
        return -1;
    }
    auto ret = EngineUtil::GetWakeupPcm(capturerData->data);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get wakeup pcm failed");
        return ret;
    }
    return 0;
}

int32_t WakeupEngineImpl::HandleResetAdapter(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!EngineUtil::CreateAdapterInner(EngineHostManager::GetInstance(), WAKEUP_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }
    SetWakeupModel();
    adapter_->SetCallback(callback_);
    EngineUtil::SetWhisperVpr();
    EngineUtil::SetLanguage();
    EngineUtil::SetArea();
    EngineUtil::SetSensibility();
    EngineUtil::SetImproveParam();
    SetDspFeatures();

    IntellVoiceEngineAdapterInfo adapterInfo = {
        .wakeupPhrase = GetWakeupPhrase(),
        .minBufSize = MIN_BUFFER_SIZE,
        .sampleChannels = CHANNEL_CNT,
        .bitsPerSample = BITS_PER_SAMPLE,
        .sampleRate = SAMPLE_RATE,
    };
    if (adapter_->Attach(adapterInfo) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to attach");
        EngineUtil::ReleaseAdapterInner(EngineHostManager::GetInstance());
        return -1;
    }

    UpdateDspModel();

    nextState = State(INITIALIZING);
    return 0;
}

int32_t WakeupEngineImpl::HandleRelease(const StateMsg & /* msg */, State &nextState)
{
    DestroyWakeupSourceStopCallback();
    StopAudioSource();
    if (CurrState() == State(READ_CAPTURER)) {
        IntellVoiceUtil::RecordPermissionPrivacy(OHOS_MICROPHONE_PERMISSION, callerTokenId_,
            INTELL_VOICE_PERMISSION_STOP);
    }
    if (adapter_ != nullptr) {
        adapter_->Detach();
        ReleaseAdapterInner(EngineHostManager::GetInstance());
    }
    nextState = State(IDLE);
    return 0;
}

bool WakeupEngineImpl::GetSwitchStatus(const std::string& key)
{
    auto ret = EngineCallbackMessage::CallFunc(QUERY_SWITCH_STATUS, key);
    if (!ret.has_value()) {
        INTELL_VOICE_LOG_ERROR("msg bus return no value");
        return false;
    }
    try {
        return std::any_cast<bool>(*ret);
    } catch (const std::bad_any_cast&) {
        INTELL_VOICE_LOG_ERROR("msg bus bad any cast");
        return false;
    }
}

void WakeupEngineImpl::SetWakeupModel()
{
    if (IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_SHORT_WORD)
        && GetSwitchStatus(SHORTWORD_KEY)) {
        adapter_->SetParameter("WakeupMode=1");
    } else {
        adapter_->SetParameter("WakeupMode=0");
    }
}

void WakeupEngineImpl::UpdateDspModel()
{
    if (IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_SHORT_WORD)) {
        if (GetSwitchStatus(SHORTWORD_KEY)) {
            SwingServiceWrapper::GetInstance().SubscribeSwingEvent(SHORT_WORD_EVENT_TYPE, SHORT_WORD_DOMAIN_ID);
            ProcDspModel(ReadDspModel(static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::ContentType>(
                OHOS::HDI::IntelligentVoice::Engine::V1_2::SHORT_WORD_DSP_MODEL)));
        } else {
            SwingServiceWrapper::GetInstance().UnSubscribeSwingEvent(SHORT_WORD_EVENT_TYPE, SHORT_WORD_DOMAIN_ID);
            ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        }
    } else if (IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_DUAL_MIC)) {
        if (GetSwitchStatus(DUALMIC_KEY)) {
            SwingServiceWrapper::GetInstance().SubscribeSwingEvent(DUAL_MIC_EVENT_TYPE, DUAL_MIC_DOMAIN_ID);
            ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        } else {
            SwingServiceWrapper::GetInstance().UnSubscribeSwingEvent(DUAL_MIC_EVENT_TYPE, DUAL_MIC_DOMAIN_ID);
            ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        }
    } else {
        ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
    }
}

std::string WakeupEngineImpl::GetWakeupPhrase()
{
    std::string wakeupPhrase = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WAKEUP_PHRASE);
    if (wakeupPhrase.empty()) {
        INTELL_VOICE_LOG_WARN("no phrase, use default phrase");
        return std::string(DEFAULT_WAKEUP_PHRASE);
    }

    return wakeupPhrase;
}

void WakeupEngineImpl::PrintFirstFrameInfo(const std::vector<uint8_t> &channelData)
{
    static constexpr size_t MIN_INFO_SIZE = 15; // offset 14 + sizeof(uint8_t)
    if (channelData.size() < MIN_INFO_SIZE) {
        return;
    }
    const auto *bytes = channelData.data();
    int16_t sentenceBegin = *reinterpret_cast<const int16_t *>(bytes + 0);
    int16_t sentenceEnd = *reinterpret_cast<const int16_t *>(bytes + 2);
    uint16_t dspType = *reinterpret_cast<const uint16_t *>(bytes + 8);
    uint8_t wakeupType = bytes[14];
    INTELL_VOICE_LOG_INFO("%{public}s wakeup trigger, wakeup type:%{public}u, "
        "extend frame:%{public}d, end frame:%{public}d",
        (dspType == 1) ? "normal" : "lp", wakeupType, sentenceBegin, sentenceEnd);
}

void WakeupEngineImpl::ReadBufferCallback(uint8_t *buffer, uint32_t size, bool isEnd)
{
    std::vector<std::vector<uint8_t>> audioData;
    auto ret = IntellVoiceUtil::DeinterleaveAudioData(reinterpret_cast<int16_t *>(buffer),
        size / sizeof(int16_t), static_cast<int32_t>(capturerOptions_.streamInfo.channels), audioData);
    if ((!ret) || ((audioData.size() != static_cast<uint32_t>(capturerOptions_.streamInfo.channels))) ||
        (channelId_ >= audioData.size())) {
        INTELL_VOICE_LOG_ERROR("failed to deinterleave, ret:%{public}d, id:%{public}d", ret, channelId_);
        return;
    }
    if ((adapter_ != nullptr) && !isEnd) {
        if (channelId_ == CHANNEL_ID_1) { // whisper wakeup, need to write channel0 and channel1 data
            std::vector<uint8_t> data(audioData[CHANNEL_ID_0].begin(), audioData[CHANNEL_ID_0].end());
            data.insert(data.end(), audioData[CHANNEL_ID_1].begin(), audioData[CHANNEL_ID_1].end());
            adapter_->WriteAudio(data);
        } else {
            if (isMultiChannel_ && (audioData.size() == CHANNEL_CNT_FOUR)) {
                if (pcmReadCnt_ == 0) {
                    PrintFirstFrameInfo(audioData[CHANNEL_ID_0]);
                }
                std::vector<uint8_t> data(audioData[CHANNEL_ID_0].begin(), audioData[CHANNEL_ID_0].end());
                data.insert(data.end(), audioData[CHANNEL_ID_1].begin(), audioData[CHANNEL_ID_1].end());
                data.insert(data.end(), audioData[CHANNEL_ID_3].begin(), audioData[CHANNEL_ID_3].end());
                adapter_->WriteAudio(data);
            } else {
                if (pcmReadCnt_ == 0) {
                    PrintFirstFrameInfo(audioData[channelId_]);
                }
                adapter_->WriteAudio(audioData[channelId_]);
            }
        }
        pcmReadCnt_++;
    }
    WakeupSourceProcess::Write(audioData);
}

bool WakeupEngineImpl::Create(std::shared_ptr<StateInfoListener> stateListener)
{
    ModuleStates::SetStateListener(stateListener);
    return Init();
}

void WakeupEngineImpl::OnTriggerDetected(int32_t uuid, const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice,
    const EventDetail &event)
{
    (void)event;
    std::string eventStr = GetEventValue(uuid);
    std::thread([eventStr, macAddress = wakeupDevice->macAddress, parameters = wakeupDevice->parameters]() {
        IntellVoiceUtil::StartAbilityOnDetected(eventStr, macAddress, parameters);
    }).detach();
    StateMsg msg(START_RECOGNIZE, &uuid, sizeof(int32_t));
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_WARN("start failed");
        EngineCallbackMessage::CallFunc(HANDLE_CLOSE_WAKEUP_SOURCE, true);
    }
}

std::string WakeupEngineImpl::GetEventValue(int32_t uuid)
{
    if (uuid == PROXIMAL_WAKEUP_MODEL_UUID) {
        ModuleStates::SetProcessType(PROCESS_WISPHER_PHONE);
        return "whisper_event";
    }
    ModuleStates::SetProcessType(PROCESS_WAKEUP_PHONE);
    return "recognition_event";
}

uint32_t WakeupEngineImpl::get_current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);

    int64_t sec = tv.tv_sec;
    int64_t day_sec = sec % SEC_PER_DAY;
    return day_sec * MS_PER_SEC + tv.tv_usec / US_PER_MS;
}

#ifdef SUPPORT_WINDOW_MANAGER
void WakeupEngineImpl::SetFoldStatus()
{
    bool isFoldable = false;
    FoldStatus newStatus = FoldStatus::EXPAND;
    int32_t ret = GetFoldStatusInfo(isFoldable, newStatus);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get fold status info err");
        return;
    }

    INTELL_VOICE_LOG_INFO("audio_fold_status isFoldable:%{public}d, curStatus:%{public}d, newStatus:%{public}d",
        isFoldable, curFoldStatus_, newStatus);
    if (!isFoldable || newStatus == curFoldStatus_) {
        return;
    }

    curFoldStatus_ = newStatus;
    AsrAecMode aecMode = (newStatus == FoldStatus::FOLDED) ? AsrAecMode::FOLDED : AsrAecMode::EXPAND;
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    audioSystemManager->SetAsrAecMode(aecMode);
    INTELL_VOICE_LOG_INFO("audio_fold_status set to fwk success, aecMode:%{public}d", aecMode);
}

int32_t WakeupEngineImpl::GetFoldStatusInfo(bool &isFoldable, FoldStatus &status)
{
    isFoldable = false;
    status = FoldStatus::EXPAND;

    auto triggerMgr = TriggerManager::GetInstance();
    if (triggerMgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger manager is nullptr");
        return -1;
    }

    std::string keyValueList = triggerMgr->GetParameter("fold_status_info");
    if (keyValueList == "") {
        INTELL_VOICE_LOG_ERROR("no status info");
        return -1;
    }

    std::map<std::string, std::string> kvpairs;
    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);
    for (auto it : kvpairs) {
        if (it.first == std::string("is_foldable")) {
            INTELL_VOICE_LOG_INFO("audio_fold_status is_foldable:%{public}s", it.second.c_str());
            if (it.second == "true") {
                isFoldable = true;
            }
        }
        if (it.first == std::string("fold_status")) {
            INTELL_VOICE_LOG_INFO("audio_fold_status fold_status:%{public}s", it.second.c_str());
            if (it.second == "fold") {
                status = FoldStatus::FOLDED;
            }
        }
    }
    INTELL_VOICE_LOG_INFO("audio_fold_status is_foldable:%{public}d, fold_status:%{public}d", isFoldable, status);
    return 0;
}
#endif
}
}
