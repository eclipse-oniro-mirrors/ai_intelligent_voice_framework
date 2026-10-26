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
#include "only_first_wakeup_engine_impl.h"

#include "audio_system_manager.h"
#include "intell_voice_info.h"
#include "intell_voice_log.h"
#include "intell_voice_util.h"
#include "string_util.h"
#include "engine_callback_message.h"
#include <sys/syscall.h>
#include <sys/resource.h>

#define LOG_TAG "OnlyFirstWakeupEngineImpl"

using namespace OHOS::IntellVoice;
using namespace OHOS::AudioStandard;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t MIN_BUFFER_SIZE = 640;
static constexpr uint32_t INTERVAL = 96;
static const std::string WAKEUP_SOURCE_CHANNEL = "wakeup_source_channel";
static constexpr int64_t RECOGNIZE_COMPLETE_TIMEOUT_US = 2 * 1000 * 1000; //2s
static constexpr int64_t READ_CAPTURER_TIMEOUT_US = 10 * 1000 * 1000; //10s
static constexpr int def_priority = 0; // prio 20 = 20 - (0)
static constexpr int high_priority = -20; // prio 40 = 20 - (-20)
OnlyFirstWakeupEngineImpl::OnlyFirstWakeupEngineImpl()
    : ModuleStates(State(IDLE), "OnlyFirstWakeupEngineImpl", "WakeupThread")
{
    InitStates();
    capturerOptions_.streamInfo.samplingRate = AudioSamplingRate::SAMPLE_RATE_16000;
    capturerOptions_.streamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    capturerOptions_.streamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    capturerOptions_.streamInfo.channels = AudioChannel::MONO;
    capturerOptions_.capturerInfo.sourceType = SourceType::SOURCE_TYPE_WAKEUP;
    capturerOptions_.capturerInfo.capturerFlags = 0;
}

OnlyFirstWakeupEngineImpl::~OnlyFirstWakeupEngineImpl()
{
}

bool OnlyFirstWakeupEngineImpl::InitStates()
{
    FromState(IDLE, READ_CAPTURER)
        .ACT(GET_PARAM, HandleGetParam);

    ForState(IDLE)
        .ACT(INIT, HandleInit);

    ForState(INITIALIZED)
        .ACT(START_RECOGNIZE, HandleStart);

    ForState(RECOGNIZED)
        .WaitUntil(RECOGNIZE_COMPLETE_TIMEOUT, std::bind(&OnlyFirstWakeupEngineImpl::HandleStopCapturer, this,
            std::placeholders::_1, std::placeholders::_2), RECOGNIZE_COMPLETE_TIMEOUT_US)
        .ACT(START_CAPTURER, HandleStartCapturer)
        .ACT(STOP_CAPTURER, HandleStopCapturer)
        .ACT(RECORD_START, HandleRecordStart);

    ForState(READ_CAPTURER)
        .WaitUntil(READ_CAPTURER_TIMEOUT, std::bind(&OnlyFirstWakeupEngineImpl::HandleStopCapturer, this,
            std::placeholders::_1, std::placeholders::_2), READ_CAPTURER_TIMEOUT_US)
        .ACT(READ, HandleRead)
        .ACT(STOP_CAPTURER, HandleStopCapturer);

    FromState(INITIALIZED, READ_CAPTURER)
        .ACT(RELEASE, HandleRelease);

    return IsStatesInitSucc();
}

int32_t OnlyFirstWakeupEngineImpl::Handle(const StateMsg &msg)
{
    if (!IsStatesInitSucc()) {
        INTELL_VOICE_LOG_ERROR("failed to init state");
        return -1;
    }

    return ModuleStates::HandleMsg(msg);
}

bool OnlyFirstWakeupEngineImpl::CreateWakeupSourceStopCallback()
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

void OnlyFirstWakeupEngineImpl::DestroyWakeupSourceStopCallback()
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

bool OnlyFirstWakeupEngineImpl::StartAudioSource()
{
    auto listener = std::make_unique<AudioSourceListener>(
        [&](uint8_t *buffer, uint32_t size, bool isEnd) {
            ReadBufferCallback(buffer, size, isEnd);
        },
        [&]() {
               INTELL_VOICE_LOG_INFO("read %{public}d frame", INTERVAL);
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

    return true;
}

void OnlyFirstWakeupEngineImpl::StopAudioSource()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (audioSource_ == nullptr) {
        INTELL_VOICE_LOG_INFO("audio source is nullptr, no need to stop");
        return;
    }
    audioSource_->Stop();
    audioSource_ = nullptr;
    WakeupSourceProcess::Release();
}

void OnlyFirstWakeupEngineImpl::OnInitDone(int32_t result)
{
    INTELL_VOICE_LOG_INFO("on Init Done, result:%{public}d", result);
    StateMsg msg(INIT_DONE, &result, sizeof(int32_t));
    Handle(msg);
}

int32_t OnlyFirstWakeupEngineImpl::HandleGetParam(const StateMsg &msg, State & /* nextState */)
{
    StringParam *key = reinterpret_cast<StringParam *>(msg.inMsg);
    StringParam *value = reinterpret_cast<StringParam *>(msg.outMsg);
    if ((key == nullptr) || (value == nullptr)) {
        INTELL_VOICE_LOG_INFO("key or value is nullptr");
        return -1;
    }

    if (key->strParam == WAKEUP_SOURCE_CHANNEL) {
        value->strParam = std::to_string(static_cast<uint32_t>(capturerOptions_.streamInfo.channels));
    } else {
        value->strParam = "";
    }

    INTELL_VOICE_LOG_INFO("key:%{public}s, value:%{public}s", key->strParam.c_str(), value->strParam.c_str());
    return 0;
}

int32_t OnlyFirstWakeupEngineImpl::HandleInit(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");

    nextState = State(INITIALIZED);
    return 0;
}

int32_t OnlyFirstWakeupEngineImpl::WakeupSource(void)
{
    if (!CreateWakeupSourceStopCallback()) {
        INTELL_VOICE_LOG_ERROR("create wakeup source stop callback failed");
        return -1;
    }

    if (!StartAudioSource()) {
        INTELL_VOICE_LOG_ERROR("start audio source failed");
        return -1;
    }

    return 0;
}

int32_t OnlyFirstWakeupEngineImpl::HandleStart(const StateMsg &msg, State &nextState)
{
    int32_t *msgBody = reinterpret_cast<int32_t *>(msg.inMsg);
    int32_t ret = 0;
    if (msgBody == nullptr) {
        INTELL_VOICE_LOG_ERROR("msgBody is nullptr");
        return -1;
    }

    channelId_ = CHANNEL_ID_0;
    INTELL_VOICE_LOG_INFO("enter, channel id is %{public}d", channelId_);
    IntellVoiceUtil::StartAbility("single_level_event");
    INTELL_VOICE_LOG_INFO("single_level_event");

    if (setpriority(PRIO_PROCESS, syscall(SYS_gettid), high_priority) != 0) {
        INTELL_VOICE_LOG_ERROR("set high priority err");
    }

    ret = WakeupSource();
    if (ret == 0) {
        nextState = State(RECOGNIZED);
    }

    if (setpriority(PRIO_PROCESS, syscall(SYS_gettid), def_priority) != 0) {
        INTELL_VOICE_LOG_ERROR("set def priority err");
    }
    INTELL_VOICE_LOG_INFO("exit");
    return ret;
}

int32_t OnlyFirstWakeupEngineImpl::HandleStartCapturer(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
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

    nextState = State(READ_CAPTURER);
    return 0;
}

int32_t OnlyFirstWakeupEngineImpl::HandleRead(const StateMsg &msg, State & /* nextState */)
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

int32_t OnlyFirstWakeupEngineImpl::HandleStopCapturer(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    StopAudioSource();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t OnlyFirstWakeupEngineImpl::HandleRelease(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    DestroyWakeupSourceStopCallback();
    StopAudioSource();
    nextState = State(IDLE);
    return 0;
}

void OnlyFirstWakeupEngineImpl::ReadBufferCallback(uint8_t *buffer, uint32_t size, bool isEnd)
{
    std::vector<std::vector<uint8_t>> audioData;
    auto ret = IntellVoiceUtil::DeinterleaveAudioData(reinterpret_cast<int16_t *>(buffer),
        size / sizeof(int16_t), static_cast<int32_t>(capturerOptions_.streamInfo.channels), audioData);
    if ((!ret) || ((audioData.size() != static_cast<uint32_t>(capturerOptions_.streamInfo.channels))) ||
        (channelId_ >= audioData.size())) {
        INTELL_VOICE_LOG_ERROR("failed to deinterleave, ret:%{public}d, id:%{public}d", ret, channelId_);
        return;
    }

    WakeupSourceProcess::Write(audioData);
}

int32_t OnlyFirstWakeupEngineImpl::HandleRecordStart(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t *value = reinterpret_cast<int32_t *>(msg.inMsg);
    if (value == nullptr) {
        INTELL_VOICE_LOG_ERROR("invaid value");
        return -1;
    }

    if ((*value != 0) && (*value != 1)) {
        INTELL_VOICE_LOG_ERROR("invaid value %d", *value);
        return 0;
    }

    recordStart_ = *value;
    if (recordStart_ == 0) {
        StopAudioSource();
    }

    nextState = State(INITIALIZED);
    return 0;
}
}
}
