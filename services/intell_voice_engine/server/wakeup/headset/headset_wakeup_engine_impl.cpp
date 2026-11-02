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
#include "headset_wakeup_engine_impl.h"
#include "idevmgr_hdi.h"
#include "audio_system_manager.h"
#include "adapter_callback_service.h"
#include "intell_voice_log.h"
#include "history_info_mgr.h"
#include "intell_voice_util.h"
#include "intell_voice_definitions.h"
#include "intell_voice_engine_manager.h"
#include "headset_host_manager.h"
#include "headset_wakeup_wrapper.h"

#define LOG_TAG "HeadsetWakeupEngineImpl"

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;
using namespace std;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int64_t RECOGNIZING_TIMEOUT_US = 10 * 1000 * 1000; // 10s
static constexpr int64_t RECOGNIZE_COMPLETE_TIMEOUT_US = 2 * 1000 * 1000; // 2s
static constexpr int64_t READ_CAPTURER_TIMEOUT_US = 10 * 1000 * 1000; // 10s
static constexpr uint32_t MAX_HEADSET_TASK_NUM = 200;
static const std::string HEADSET_THREAD_NAME = "HeadsetThread";

HeadsetWakeupEngineImpl::HeadsetWakeupEngineImpl() : ModuleStates(State(IDLE), "HeadsetWakeupEngineImpl"),
    TaskExecutor(HEADSET_THREAD_NAME, MAX_HEADSET_TASK_NUM), BaseWakeupEngineImpl()
{
}

HeadsetWakeupEngineImpl::~HeadsetWakeupEngineImpl()
{
}

bool HeadsetWakeupEngineImpl::Init()
{
    if (!InitStates()) {
        INTELL_VOICE_LOG_ERROR("init state failed");
        return false;
    }

    adapterListener_ = std::make_shared<WakeupAdapterListener>(
        std::bind(&HeadsetWakeupEngineImpl::OnWakeupEvent, this, std::placeholders::_1));
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
        return false;
    }

    TaskExecutor::StartThread();
    mCloudReporter = std::make_shared<AccessoriesCloudReporter>("headset");
    return true;
}

bool HeadsetWakeupEngineImpl::InitStates()
{
    for (int i = IDLE; i <= READ_CAPTURER; i++) {
        ForState(State(i))
            .ACT(SET_LISTENER, HandleSetListener)
            .ACT(SET_PARAM, HandleSetParam);
    }

    ForState(IDLE)
        .ACT(INIT, HandleInit)
        .ACT(RESET_ADAPTER, HandleResetAdapter);

    ForState(INITIALIZING)
        .ACT(INIT_DONE, HandleInitDone);

    ForState(INITIALIZED)
        .ACT(START_RECOGNIZE, HandleStart);

    ForState(RECOGNIZING)
        .WaitUntil(RECOGNIZING_TIMEOUT, std::bind(&HeadsetWakeupEngineImpl::HandleRecognizingTimeout,
            this, std::placeholders::_1, std::placeholders::_2), RECOGNIZING_TIMEOUT_US)
        .ACT(STOP_RECOGNIZE, HandleStop)
        .ACT(RECOGNIZE_COMPLETE, HandleRecognizeComplete);

    ForState(RECOGNIZED)
        .WaitUntil(RECOGNIZE_COMPLETE_TIMEOUT, std::bind(&HeadsetWakeupEngineImpl::HandleStopCapturer,
            this, std::placeholders::_1, std::placeholders::_2), RECOGNIZE_COMPLETE_TIMEOUT_US)
        .ACT(START_CAPTURER, HandleStartCapturer);

    ForState(READ_CAPTURER)
        .WaitUntil(READ_CAPTURER_TIMEOUT, std::bind(&HeadsetWakeupEngineImpl::HandleStopCapturer,
            this, std::placeholders::_1, std::placeholders::_2), READ_CAPTURER_TIMEOUT_US)
        .ACT(READ, HandleRead)
        .ACT(STOP_CAPTURER, HandleStopCapturer);

    FromState(INITIALIZING, READ_CAPTURER)
        .ACT(RELEASE_ADAPTER, HandleRelease)
        .ACT(RELEASE, HandleRelease);

    return IsStatesInitSucc();
}

int32_t HeadsetWakeupEngineImpl::Handle(const StateMsg &msg)
{
    if (!IsStatesInitSucc()) {
        INTELL_VOICE_LOG_ERROR("failed to init state");
        return -1;
    }

    return ModuleStates::HandleMsg(msg);
}

bool HeadsetWakeupEngineImpl::Create(std::shared_ptr<StateInfoListener> stateListener)
{
    ModuleStates::SetStateListener(stateListener);
    ModuleStates::SetProcessType(PROCESS_WAKEUP_HEADSET);
    auto devmgr = IDeviceManager::Get();
    if (devmgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("get devmgr failed");
        return false;
    }
    devmgr->UnloadDevice(HEADSET_SERVICE_NAME);
    devmgr->LoadDevice(HEADSET_SERVICE_NAME);
    if (!HeadsetHostManager::GetInstance().Init()) {
        INTELL_VOICE_LOG_ERROR("init headset host failed");
        return false;
    }
    HeadsetHostManager::GetInstance().RegisterEngineHDIDeathRecipient();
    return Init();
}

bool HeadsetWakeupEngineImpl::Destroy()
{
    StateMsg msg(RELEASE);
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("release headset wakeup engine impl failed");
    }
    HeadsetHostManager::GetInstance().DeregisterEngineHDIDeathRecipient();
    auto devmgr = IDeviceManager::Get();
    if (devmgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("get devmgr failed");
        return false;
    }
    devmgr->UnloadDevice(HEADSET_SERVICE_NAME);
    return true;
}

void HeadsetWakeupEngineImpl::OnTriggerDetected(int32_t /* uuid */,
    const std::shared_ptr<WakeupDeviceDetail>& wakeupDevice, const EventDetail &event)
{
    std::thread([macAddress = wakeupDevice->macAddress,
        parameters = wakeupDevice->parameters, endIndex = event.endFrameIndex]() {
            IntellVoiceUtil::StartAbilityOnDetected("headset_event", macAddress, parameters, endIndex);
        }).detach();
    if (mCloudReporter) {
        mCloudReporter->Init();
        mCloudReporter->SetEndIndex(event.endFrameIndex);
    }
    StateMsg msg(START_RECOGNIZE);
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_WARN("start failed");
    }
}

bool HeadsetWakeupEngineImpl::SetCallbackInner()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return false;
    }

    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
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

int32_t HeadsetWakeupEngineImpl::AttachInner(const IntellVoiceEngineInfo &info)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    IntellVoiceEngineAdapterInfo adapterInfo = {
    };

    return adapter_->Attach(adapterInfo);
}

void HeadsetWakeupEngineImpl::OnWakeupEvent(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("enter, msgId:%{public}d, result:%{public}d", event.msgId, event.result);
    if (event.msgId == INTELL_VOICE_ENGINE_MSG_INIT_DONE) {
        TaskExecutor::AddAsyncTask([this, result = event.result]() { OnInitDone(result); },
            "HeadsetWakeupEngineImpl::InitDone", false);
    } else if (
        event.msgId == static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(
            OHOS::HDI::IntelligentVoice::Engine::V1_2::INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE)) {
        TaskExecutor::AddAsyncTask([this, result = event.result, info = event.info]() {
            OnWakeupRecognition(result, info);
            }, "HeadsetWakeupEngineImpl::RecgonizeComplete", false);
    } else {
    }
}

void HeadsetWakeupEngineImpl::OnInitDone(int32_t result)
{
    INTELL_VOICE_LOG_INFO("enter, result:%{public}d", result);
    StateMsg msg(INIT_DONE, &result, sizeof(int32_t));
    Handle(msg);
}

void HeadsetWakeupEngineImpl::OnWakeupRecognition(int32_t result, const std::string &info)
{
    INTELL_VOICE_LOG_INFO("enter, result:%{public}d", result);
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent event;
    event.msgId = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(
        HDI::IntelligentVoice::Engine::V1_2::INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE);
    event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(result);
    event.info = info;
    StateMsg msg(RECOGNIZE_COMPLETE, reinterpret_cast<void *>(&event),
        sizeof(OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent));
    Handle(msg);
}

int32_t HeadsetWakeupEngineImpl::HandleInit(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!EngineUtil::CreateAdapterInner(HeadsetHostManager::GetInstance(), WAKEUP_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }

    if (!SetCallbackInner()) {
        INTELL_VOICE_LOG_ERROR("failed to set callback");
        return -1;
    }

    EngineUtil::SetLanguage();
    EngineUtil::SetArea();
    adapter_->SetParameter("model_path=/vendor/etc/audio/encoder.om");
    std::string *msgBody = reinterpret_cast<std::string *>(msg.inMsg);
    if (msgBody != nullptr) {
        adapter_->SetParameter("modelType=" + *msgBody);
    }
    IntellVoiceEngineInfo info = {};

    if (AttachInner(info) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to attach");
        return -1;
    }

    nextState = State(INITIALIZING);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleInitDone(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t *result = reinterpret_cast<int32_t *>(msg.inMsg);
    if ((result == nullptr) || (*result != 0)) {
        INTELL_VOICE_LOG_ERROR("init done failed");
        return -1;
    }

    nextState = State(INITIALIZED);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleSetListener(const StateMsg &msg, State & /* nextState */)
{
    SetListenerMsg *listenerMsg = reinterpret_cast<SetListenerMsg *>(msg.inMsg);
    if (listenerMsg == nullptr || adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("listenerMsg or adapter listener is nullptr");
        return -1;
    }

    adapterListener_->SetCallback(listenerMsg->callback);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleStart(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!needSecondStageWakeup) {
        INTELL_VOICE_LOG_INFO("no need to recognize");
        nextState = State(RECOGNIZED);
        return 0;
    }
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    StartInfo info = {
        .isLast = true,
    };
    if (adapter_->Start(info)) {
        INTELL_VOICE_LOG_ERROR("start adapter failed");
        return -1;
    }

    if (!StartAudioSource()) {
        INTELL_VOICE_LOG_ERROR("start audio source failed");
        adapter_->Stop();
        return -1;
    }

    INTELL_VOICE_LOG_INFO("exit");
    nextState = State(RECOGNIZING);
    return 0;
}

void HeadsetWakeupEngineImpl::ReadThread()
{
    bool isEnd = false;
    while (isReading_.load()) {
        std::vector<uint8_t> audioStream;
        bool hasAwakeWord = true;
        int ret = HeadsetWakeupWrapper::GetInstance().ReadHeadsetStream(audioStream, hasAwakeWord);
        if (ret != 1) {
            INTELL_VOICE_LOG_INFO("finish reading, ret:%{public}d, isEnd:%{public}d, hasAwakeWord:%{public}d",
                ret, isEnd, hasAwakeWord);
            if (!isEnd) {
                adapter_->SetParameter("end_of_pcm=true");
            }
            break;
        }
        if (hasAwakeWord && !isEnd) {
            adapter_->WriteAudio(audioStream);
        }
        if (!hasAwakeWord && !isEnd) {
            isEnd = true;
            adapter_->SetParameter("end_of_pcm=true");
        }
        if (mCloudReporter) {
            mCloudReporter->WriteStream({ audioStream });
        }
        WakeupSourceProcess::Write({ audioStream });
    }
}

bool HeadsetWakeupEngineImpl::StartAudioSource()
{
    INTELL_VOICE_LOG_INFO("enter");

    isReading_.store(true);

    WakeupSourceProcess::Init(1);

    std::thread t1(std::bind(&HeadsetWakeupEngineImpl::ReadThread, this));
    readThread_ = std::move(t1);
    return true;
}

void HeadsetWakeupEngineImpl::StopAudioSource()
{
    if (!isReading_.load()) {
        INTELL_VOICE_LOG_INFO("already stop");
        return;
    }
    isReading_.store(false);
    readThread_.join();
    HeadsetWakeupWrapper::GetInstance().StopReadingStream();
    WakeupSourceProcess::Release();
    if (!mCloudReporter) {
        return;
    }
    if (mCloudReporter->CreateReportDir()) {
        (void)mCloudReporter->SavePcm();
        if (!mCloudReporter->SetReportInfo()) {
            INTELL_VOICE_LOG_ERROR("SetReportInfo fail");
        }
    }
}

int32_t HeadsetWakeupEngineImpl::HandleStop(const StateMsg & /* msg */, State &nextState)
{
    StopAudioSource();
    EngineUtil::Stop();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleRecognizeComplete(const StateMsg &msg, State &nextState)
{
    EngineUtil::Stop();
    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *event =
        reinterpret_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent *>(msg.inMsg);
    if (event == nullptr) {
        INTELL_VOICE_LOG_ERROR("event is nullptr");
        return -1;
    }

    adapterListener_->Notify(*event);

    if (event->result != 0) {
        INTELL_VOICE_LOG_INFO("wakeup failed");
        HeadsetWakeupWrapper::GetInstance().NotifyVerifyResult(false);
        StopAudioSource();
        nextState = State(INITIALIZED);
    } else {
        HeadsetWakeupWrapper::GetInstance().NotifyVerifyResult(true);
        nextState = State(RECOGNIZED);
    }
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleStartCapturer(const StateMsg &msg, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    int32_t *msgBody = reinterpret_cast<int32_t *>(msg.inMsg);
    if (msgBody == nullptr) {
        INTELL_VOICE_LOG_ERROR("msgBody is nullptr");
        return -1;
    }
    channels_ = *msgBody;
    nextState = State(READ_CAPTURER);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleRead(const StateMsg &msg, State & /* nextState */)
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

int32_t HeadsetWakeupEngineImpl::HandleStopCapturer(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    StopAudioSource();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleRecognizingTimeout(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    HeadsetWakeupWrapper::GetInstance().NotifyVerifyResult(false);
    StopAudioSource();
    EngineUtil::Stop();
    nextState = State(INITIALIZED);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleResetAdapter(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!EngineUtil::CreateAdapterInner(HeadsetHostManager::GetInstance(), WAKEUP_ADAPTER_TYPE)) {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }

    adapter_->SetCallback(callback_);
    EngineUtil::SetLanguage();
    EngineUtil::SetArea();
    adapter_->SetParameter("model_path=/vendor/etc/audio/encoder.om");

    IntellVoiceEngineAdapterInfo adapterInfo = {
    };

    if (adapter_->Attach(adapterInfo) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to attach");
        EngineUtil::ReleaseAdapterInner(HeadsetHostManager::GetInstance());
        return -1;
    }

    nextState = State(INITIALIZING);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleRelease(const StateMsg & /* msg */, State &nextState)
{
    StopAudioSource();
    if (adapter_ != nullptr) {
        adapter_->Detach();
        ReleaseAdapterInner(HeadsetHostManager::GetInstance());
    }
    nextState = State(IDLE);
    return 0;
}

int32_t HeadsetWakeupEngineImpl::HandleSetParam(const StateMsg &msg, State & /* nextState */)
{
    StringParam *param = reinterpret_cast<StringParam *>(msg.inMsg);
    if (param == nullptr) {
        INTELL_VOICE_LOG_INFO("param is nullptr");
        return -1;
    }

    return EngineUtil::SetParameter(param->strParam);
}
}
}