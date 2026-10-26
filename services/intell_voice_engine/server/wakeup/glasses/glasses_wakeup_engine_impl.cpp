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
#include "glasses_wakeup_engine_impl.h"
#include "intell_voice_log.h"
#include "idevmgr_hdi.h"
#include "intell_voice_definitions.h"
 
#define LOG_TAG "GlassesWakeupEngineImpl"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace std;
 
namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int64_t RECOGNIZING_TIMEOUT_US = 10 * 1000 * 1000; //10s
static constexpr int64_t RECOGNIZE_COMPLETE_TIMEOUT_US = 2 * 1000 * 1000; //2s
static constexpr int64_t READ_CAPTURER_TIMEOUT_US = 10 * 1000 * 1000; //10s
static constexpr uint32_t MAX_HEADSET_TASK_NUM = 200;
static const std::string GLASSES_THREAD_NAME = "GlassesThread";
 
GlassesWakeupEngineImpl::GlassesWakeupEngineImpl() : ModuleStates(State(IDLE), "GlassesWakeupEngineImpl"),
    TaskExecutor(GLASSES_THREAD_NAME, MAX_HEADSET_TASK_NUM), BaseWakeupEngineImpl()
{
}
 
GlassesWakeupEngineImpl::~GlassesWakeupEngineImpl()
{
}
 
bool GlassesWakeupEngineImpl::Init()
{
    if (!InitStates()) {
        INTELL_VOICE_LOG_ERROR("init state failed");
        return false;
    }
    adapterListener_ = std::make_shared<WakeupAdapterListener>(
        std::bind(&GlassesWakeupEngineImpl::OnWakeupEvent, this, std::placeholders::_1));
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
        return false;
    }
    TaskExecutor::StartThread();
    mCloudReporter = std::make_shared<AccessoriesCloudReporter>("ai_glass");
    return true;
}
 
bool GlassesWakeupEngineImpl::InitStates()
{
    for (int i = IDLE; i <= READ_CAPTURER; i++) {
        ForState(State(i))
            .ACT(SET_LISTENER, HandleSetListener)
            .ACT(SET_PARAM, HandleSetParam);
    }
 
    ForState(IDLE)
        .ACT(INIT, HandleInit)
        .ACT(RESET_ADAPTER, HandleResetAdapter);
 
    ForState(INITIALIZED)
        .ACT(START_RECOGNIZE, HandleStart);

    ForState(RECOGNIZING)
        .WaitUntil(RECOGNIZING_TIMEOUT, std::bind(&GlassesWakeupEngineImpl::HandleRecognizingTimeout,
            this, std::placeholders::_1, std::placeholders::_2), RECOGNIZING_TIMEOUT_US)
        .ACT(STOP_RECOGNIZE, HandleStop)
        .ACT(RECOGNIZE_COMPLETE, HandleRecognizeComplete);

    ForState(RECOGNIZED)
        .WaitUntil(RECOGNIZE_COMPLETE_TIMEOUT, std::bind(&GlassesWakeupEngineImpl::HandleStopCapturer,
            this, std::placeholders::_1, std::placeholders::_2), RECOGNIZE_COMPLETE_TIMEOUT_US)
        .ACT(START_CAPTURER, HandleStartCapturer);
 
    ForState(READ_CAPTURER)
        .WaitUntil(READ_CAPTURER_TIMEOUT, std::bind(&GlassesWakeupEngineImpl::HandleStopCapturer,
            this, std::placeholders::_1, std::placeholders::_2), READ_CAPTURER_TIMEOUT_US)
        .ACT(READ, HandleRead)
        .ACT(STOP_CAPTURER, HandleStopCapturer);
 
    FromState(INITIALIZING, READ_CAPTURER)
        .ACT(RELEASE_ADAPTER, HandleRelease)
        .ACT(RELEASE, HandleRelease);
 
    return IsStatesInitSucc();
}
 
int32_t GlassesWakeupEngineImpl::Handle(const StateMsg &msg)
{
    if (!IsStatesInitSucc()) {
        INTELL_VOICE_LOG_ERROR("failed to init state");
        return -1;
    }
 
    return ModuleStates::HandleMsg(msg);
}
 
int32_t GlassesWakeupEngineImpl::HandleInit(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    GlassesWakeupWrapper::GetInstance().SetCallback([this](DeviceCallBackEvent data) {
        IntellVoiceEngineCallBackEvent event;
        event.msgId = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(data.msgId);
        event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(data.result);
        event.info = data.info;
        adapterListener_->OnIntellVoiceHdiEvent(event);
    });
    nextState = State(INITIALIZED);
    return 0;
}

int32_t GlassesWakeupEngineImpl::HandleSetListener(const StateMsg &msg, State & /* nextState */)
{
    SetListenerMsg *listenerMsg = reinterpret_cast<SetListenerMsg *>(msg.inMsg);
    if (listenerMsg == nullptr || adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("listenerMsg or adapter listener is nullptr");
        return -1;
    }
 
    adapterListener_->SetCallback(listenerMsg->callback);
    return 0;
}

int32_t GlassesWakeupEngineImpl::HandleStart(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (needSecondStageWakeup_) {
        nextState = State(RECOGNIZING);
    } else {
        if (!StartAudioSource()) {
            INTELL_VOICE_LOG_ERROR("start audio source failed");
            nextState = State(INITIALIZED);
            return -1;
        }
        nextState = State(RECOGNIZED);
    }
    return 0;
}

void GlassesWakeupEngineImpl::OnWakeupEvent(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("enter, msgId:%{public}d, result:%{public}d", event.msgId, event.result);
    if (event.msgId == OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        TaskExecutor::AddAsyncTask(
            [this, result = event.result, info = event.info]() {
                OnWakeupRecognition(result, info);
            }, "GlassesWakeupEngineImpl::RecgonizeComplete", false);
    } else {
        INTELL_VOICE_LOG_INFO("msgId error");
    }
}
 
void GlassesWakeupEngineImpl::OnWakeupRecognition(int32_t result, const std::string &info)
{
    INTELL_VOICE_LOG_INFO("enter, result:%{public}d", result);
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent event;
    event.msgId = OHOS::HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE;
    event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(result);
    event.info = info;
    StateMsg msg(RECOGNIZE_COMPLETE, reinterpret_cast<void *>(&event),
        sizeof(OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent));
    Handle(msg);
}

void GlassesWakeupEngineImpl::ReadThread()
{
    bool isEnd = false;
    while (isReading_.load()) {
        std::vector<uint8_t> audioStream;
        bool hasAwakeWord = true;
        int ret = GlassesWakeupWrapper::GetInstance().ReadGlassesStream(audioStream, hasAwakeWord);
        if (ret != 0) {
            INTELL_VOICE_LOG_INFO("finish reading, ret:%{public}d, isEnd:%{public}d, hasAwakeWord:%{public}d",
                ret, isEnd, hasAwakeWord);
            break;
        }
        if (mCloudReporter) {
            mCloudReporter->WriteStream({ audioStream });
        }
        WakeupSourceProcess::Write({ audioStream });
    }
}
 
bool GlassesWakeupEngineImpl::StartAudioSource()
{
    INTELL_VOICE_LOG_INFO("enter");
    isReading_.store(true);
    WakeupSourceProcess::Init(1);
 
    std::thread t1(std::bind(&GlassesWakeupEngineImpl::ReadThread, this));
    readThread_ = std::move(t1);
    return true;
}
 
void GlassesWakeupEngineImpl::StopAudioSource()
{
    if (!isReading_.load()) {
        INTELL_VOICE_LOG_INFO("already stop");
        return;
    }
    isReading_.store(false);
    readThread_.join();
    GlassesWakeupWrapper::GetInstance().StopReadingStream();
    WakeupSourceProcess::Release();
    if(!mCloudReporter) {
        return;
    }
    if (mCloudReporter->CreateReportDir()) {
        (void)mCloudReporter->SavePcm();
        if (!mCloudReporter->SetReportInfo()) {
            INTELL_VOICE_LOG_ERROR("SetReportInfo fail");
        }
    }
}
 
int32_t GlassesWakeupEngineImpl::HandleStartCapturer(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t *msgBody = reinterpret_cast<int32_t *>(msg.inMsg);
    if (msgBody == nullptr) {
        INTELL_VOICE_LOG_ERROR("msgBody is nullptr");
        return -1;
    }
    channels_ = 1;
    nextState = State(READ_CAPTURER);
    return 0;
}
 
int32_t GlassesWakeupEngineImpl::HandleRead(const StateMsg &msg, State & /* nextState */)
{
    CapturerData *capturerData = reinterpret_cast<CapturerData *>(msg.outMsg);
    auto ret = WakeupSourceProcess::Read(capturerData->data, channels_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("read capturer data failed");
        return ret;
    }
 
    ResetTimerDelay();
    return 0;
}
 
int32_t GlassesWakeupEngineImpl::HandleStopCapturer(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    StopAudioSource();
    nextState = State(INITIALIZED);
    return 0;
}
 
int32_t GlassesWakeupEngineImpl::HandleRecognizingTimeout(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    nextState = State(INITIALIZED);
    return 0;
}

int32_t GlassesWakeupEngineImpl::HandleStop(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    StopAudioSource();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t GlassesWakeupEngineImpl::HandleRecognizeComplete(const StateMsg &msg, State &nextState)
{
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *event =
        reinterpret_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *>(msg.inMsg);
    if (event == nullptr) {
        INTELL_VOICE_LOG_ERROR("result is nullptr");
        return -1;
    }
    mCloudReporter->SetEndIndex(event->info);
    if (adapterListener_ != nullptr) {
        adapterListener_->Notify(*event);
    }
 
    if (event->result != 0) {
        INTELL_VOICE_LOG_ERROR("wakeup failed");
        nextState = State(INITIALIZED);
    } else { 
        if (!StartAudioSource()) {
            INTELL_VOICE_LOG_ERROR("start audio source failed");
            return -1;
        }
        nextState = State(RECOGNIZED);
    }
    return 0;
}

int32_t GlassesWakeupEngineImpl::HandleResetAdapter(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    GlassesWakeupWrapper::GetInstance().SetCallback([this](DeviceCallBackEvent data) {
        IntellVoiceEngineCallBackEvent event;
        event.msgId = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(data.msgId);
        event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(data.result);
        event.info = data.info;
        adapterListener_->OnIntellVoiceHdiEvent(event);
    });
    nextState = State(INITIALIZED);
    return 0;
}
 
int32_t GlassesWakeupEngineImpl::HandleRelease(const StateMsg & /* msg */, State &nextState)
{
    StopAudioSource();
    GlassesWakeupWrapper::GetInstance().SetCallback(nullptr);
    nextState = State(IDLE);
    return 0;
}
 
int32_t GlassesWakeupEngineImpl::HandleSetParam(const StateMsg & /* msg */, State & /* nextState */)
{
    INTELL_VOICE_LOG_INFO("no need process");
    return 0;
}

bool GlassesWakeupEngineImpl::Create(std::shared_ptr<StateInfoListener> stateListener)
{
    ModuleStates::SetStateListener(stateListener);
    ModuleStates::SetProcessType(PROCESS_WAKEUP_GLASSES);
    return Init();
}
 
bool GlassesWakeupEngineImpl::Destroy()
{
    StateMsg msg(RELEASE);
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("release glasses wakeup engine impl failed");
    }
    TaskExecutor::StopThread();
    return true;
}
 
void GlassesWakeupEngineImpl::OnTriggerDetected(int32_t /* uuid */,
    const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice, const EventDetail &event)
{
    std::thread([macAddress = wakeupDevice->macAddress,
        parameters = wakeupDevice->parameters, endIndex = event.endFrameIndex]() {
            IntellVoiceUtil::StartAbilityOnDetected("ai_glasses_event", macAddress, parameters, endIndex);
        }).detach();

    needSecondStageWakeup_ = wakeupDevice->needSecondStageWakeup;
    if (mCloudReporter) {
        mCloudReporter->Init();
        mCloudReporter->SetEndIndex(event.endFrameIndex);
    }
    StateMsg msg(START_RECOGNIZE);
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_WARN("start failed");
    }
}
}
}