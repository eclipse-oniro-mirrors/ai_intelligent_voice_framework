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
#include "implicit_enroll_engine.h"
#include <fstream>
#include "ipc_skeleton.h"
#include "iremote_object.h"
#include "securec.h"
#include "intell_voice_info.h"
#include "intell_voice_log.h"
#include "intell_voice_util.h"
#include "enroll_adapter_listener.h"
#include "time_util.h"
#include "json/json.h"
#include "scope_guard.h"
#include "adapter_callback_service.h"
#include "engine_callback_message.h"
#include "intell_voice_engine_manager.h"
#include "update_engine_utils.h"
#include "engine_host_manager.h"
#include "history_info_mgr.h"

#define LOG_TAG "ImplicitEnrollEngine"

using namespace OHOS::IntellVoice;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using OHOS::HDI::IntelligentVoice::Engine::V1_3::IMPLICIT_ENROLL_ADAPTER_TYPE;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t MIN_BUFFER_SIZE = 1280; // 16 * 2 * 40ms
static constexpr uint32_t INTERVAL = 125; // 125 * 40ms = 5s
static constexpr uint32_t WITHCMD_INTERVAL = 200; // 200 * 40ms = 8s
static constexpr uint32_t MAX_ENROLL_TASK_NUM = 100;
static constexpr std::string_view DEFAULT_WAKEUP_PHRASE = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
static constexpr int64_t RECOGNIZING_TIMEOUT_US = 5 * 60 * 1000 * 1000; //5min
static const std::string ENGINE_NAME = "ImplicitEnrollEngine";
static const std::string ENROLL_THREAD_NAME = "ImplicitEnrollEngThread";
static const std::string STATE_THREAD_NAME = "ImplicitEnrollStateThread";
static const std::string DIALOG_STAGE = "dialog";
static const std::string VERIFY_STAGE = "verify";
static const std::string STAGE_TXT = "stage=";
static const std::string START_TIME_TXT = "startTime=";
static const std::string END_TIME_TXT = "endTime=";

ImplicitEnrollEngine::ImplicitEnrollEngine() : ModuleStates(State(IDLE), ENGINE_NAME, STATE_THREAD_NAME),
    TaskExecutor(ENROLL_THREAD_NAME, MAX_ENROLL_TASK_NUM)
{
    INTELL_VOICE_LOG_INFO("enter");
}

ImplicitEnrollEngine::~ImplicitEnrollEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
    Detach();
    callback_ = nullptr;
}


bool ImplicitEnrollEngine::Init(const std::string &param, bool reEnroll)
{
    (void)param;
    if (!InitStates()) {
        INTELL_VOICE_LOG_ERROR("init state failed");
        return false;
    }
    adapterListener_ = std::make_shared<ImplicitEnrollAdapterListener>(
        std::bind(&ImplicitEnrollEngine::OnEnrollEvent, this, std::placeholders::_1));
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapterListener_ is nullptr");
        return false;
    }
    StateMsg msg(INIT);
    if (Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("init wakeup engine impl failed");
        return false;
    }

    return true;
}


bool ImplicitEnrollEngine::InitStates()
{
    for (int i = IDLE; i <= RECOGNIZING; i++) {
        ForState(State(i))
            .ACT(SET_LISTENER, HandleSetListener);
    }

    ForState(IDLE)
        .ACT(INIT, HandleInit);

    ForState(INITIALIZED)
        .ACT(START_RECOGNIZE, HandleStart)
        .ACT(SET_PARAM, HandleSetParam)
        .ACT(RELEASE, HandleRelease);

    ForState(RECOGNIZING)
        .WaitUntil(RECOGNIZING_TIMEOUT,
            std::bind(&ImplicitEnrollEngine::HandleRecognizingTimeout, this, std::placeholders::_1,
                std::placeholders::_2),
            RECOGNIZING_TIMEOUT_US)
        .ACT(RECOGNIZE_COMPLETE, HandleEnrollComplete)
        .ACT(RELEASE, HandleRelease);

    return IsStatesInitSucc();
}

int32_t ImplicitEnrollEngine::Handle(const StateMsg &msg)
{
    if (!IsStatesInitSucc()) {
        INTELL_VOICE_LOG_ERROR("failed to init state");
        return -1;
    }

    return ModuleStates::HandleMsg(msg);
}

int32_t ImplicitEnrollEngine::HandleInit(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!EngineUtil::CreateAdapterInner(EngineHostManager::GetInstance(), 
        static_cast<HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineAdapterType>(IMPLICIT_ENROLL_ADAPTER_TYPE)))
    {
        INTELL_VOICE_LOG_ERROR("failed to create adapter");
        return -1;
    }
    wakeupPhrase_ = std::string(DEFAULT_WAKEUP_PHRASE);
    SetCallbackInner();
    EngineUtil::SetLanguage();
    EngineUtil::SetArea();

    IntellVoiceEngineInfo engineInfo = {
        .wakeupPhrase = wakeupPhrase_,
        .minBufSize = 1280,
        .sampleChannels = 1,
        .bitsPerSample = 16,
        .sampleRate = 16000,
    };
    int ret = Attach(engineInfo);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("attach err");
        EngineUtil::ReleaseAdapterInner(EngineHostManager::GetInstance());
        return -1;
    }
    nextState = State(INITIALIZED);
    return 0;
}

int32_t ImplicitEnrollEngine::HandleRecognizingTimeout(const StateMsg & /* msg */, State &nextState)
{
    INTELL_VOICE_LOG_INFO("enter");
    EngineCallbackMessage::CallFunc(RELEASE_IMPLICIT_ENROLL_ENGINE);
    nextState = State(IDLE);
    return 0;
}

int32_t ImplicitEnrollEngine::HandleSetListener(const StateMsg &msg, State & /* nextState */)
{
    sptr<IIntelligentVoiceUpdateCallback> *callback
        = reinterpret_cast<sptr<IIntelligentVoiceUpdateCallback> *>(msg.inMsg);
    if (callback == nullptr || *callback == nullptr || adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("listenerMsg or adapter listener is nullptr");
        return -1;
    }
    adapterListener_->SetCallback(*callback);
    return 0;
}

int32_t ImplicitEnrollEngine::HandleStart(const StateMsg &msg, State &nextState)
{
    StringParam *param = reinterpret_cast<StringParam *>(msg.inMsg);
    if (param == nullptr) {
        INTELL_VOICE_LOG_INFO("param is nullptr");
        return -1;
    }
    if (!CheckParams(param->strParam)) {
        INTELL_VOICE_LOG_ERROR("invalid param");
        return -1;
    } 
    TaskExecutor::AddAsyncTask([this, stage = stage_, startTime = startTime_, endTime = endTime_]() {
        INTELL_VOICE_LOG_INFO("start enroll, stage: %{public}s", stage.c_str());
        EngineUtil::SetParameter(STAGE_TXT + stage);
        if (stage == VERIFY_STAGE) {
            EngineUtil::SetParameter(START_TIME_TXT + std::to_string(startTime));
            EngineUtil::SetParameter(END_TIME_TXT + std::to_string(endTime));
        }
        if (adapter_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("adapter is nullptr");
            return;
        }
        StartInfo info = {
            .isLast = true,
        };
        adapter_->Start(info);
    }, "ImplicitEnrollEngine::HandleStart", false);
    nextState = State(RECOGNIZING);
    return 0;
}

int32_t ImplicitEnrollEngine::HandleSetParam(const StateMsg &msg, State & /* nextState */)
{
    StringParam *param = reinterpret_cast<StringParam *>(msg.inMsg);
    if (param == nullptr) {
        INTELL_VOICE_LOG_INFO("param is nullptr");
        return -1;
    }
    TaskExecutor::AddSyncTask([this, paramStr = param->strParam]() {
            INTELL_VOICE_LOG_INFO("clear enroll data, %{public}s", paramStr.c_str());
            EngineUtil::SetParameter(paramStr);
        });
    return 0;
}

int32_t ImplicitEnrollEngine::HandleRelease(const StateMsg & /* msg */, State &nextState)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_WARN("already detach");
        return 0;
    }
    int32_t ret = adapter_->Detach();
    ReleaseAdapterInner(EngineHostManager::GetInstance());
    nextState = State(IDLE);
    return ret;
}

void ImplicitEnrollEngine::OnEnrollEvent(
    const OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent &event)
{
    int32_t result = static_cast<int32_t>(event.result);
    if (event.msgId == INTELL_VOICE_ENGINE_MSG_INIT_DONE) {
        wptr<ImplicitEnrollEngine> thisWptr(this);
        std::thread([thisWptr, result]() {
            INTELL_VOICE_LOG_INFO("on Init Done, result:%{public}d", result);
            sptr<ImplicitEnrollEngine> thisSptr = thisWptr.promote();
            if (thisSptr == nullptr) {
                INTELL_VOICE_LOG_ERROR("ImplicitEnrollEngine is null");
                return;
            }
            if ((result != 0)) {
                // init failed, release engine
                INTELL_VOICE_LOG_ERROR("hdf init failed, %{public}d", result);
                EngineCallbackMessage::CallFunc(RELEASE_IMPLICIT_ENROLL_ENGINE);
                return;
            }
            INTELL_VOICE_LOG_INFO("hdf init done");
            thisSptr->StartThread();
        }).detach();
    } else if (event.msgId == INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE) {
        INTELL_VOICE_LOG_INFO("enroll complete callback %{public}d", result);
        TaskExecutor::AddAsyncTask([this, result, info = event.info]() {
            OnEnrollComplete(result, info);
        }, "ImplicitEnrollEngine::OnEnrollComplete", false);
    }
}

void ImplicitEnrollEngine::OnEnrollComplete(int32_t result, const std::string &info)
{
    INTELL_VOICE_LOG_INFO("on enroll complete, result:%{public}d", result);

    OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent event;
    event.msgId = INTELL_VOICE_ENGINE_MSG_ENROLL_COMPLETE;
    event.result = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineErrors>(result);
    event.info = info;
    StateMsg msg(RECOGNIZE_COMPLETE, reinterpret_cast<void *>(&event),
        sizeof(OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineCallBackEvent));
    Handle(msg);
    if (stage_ == VERIFY_STAGE) {
        bool isSucc = event.result == 0;
        IntellVoiceEngineManager::SetEnrollResult(INTELL_VOICE_IMPLICIT_ENROLL, isSucc);
        if (!isSucc) {
            return;
        }
        EngineUtil::ProcDspModel(ReadDspModel(OHOS::HDI::IntelligentVoice::Engine::V1_0::DSP_MODLE));
        UpdateEngineUtils::SaveWakeupVesion();
        INTELL_VOICE_LOG_INFO("enroll save version");
        HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_ENGINE_BUNDLE_NAME, bundleName_);
        INTELL_VOICE_LOG_INFO("set wakeup bundle name:%{public}s", bundleName_.c_str());
        HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_ENGINE_ABILITY_NAME, abilityName_);
        INTELL_VOICE_LOG_INFO("set wakeup ability name:%{public}s", abilityName_.c_str());
        HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_PHRASE, wakeupPhrase_);
    }
}


int32_t ImplicitEnrollEngine::HandleEnrollComplete(const StateMsg &msg, State &nextState)
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
    nextState = State(INITIALIZED);
    return 0;
}

bool ImplicitEnrollEngine::CheckParams(const std::string &param)
{
    std::istringstream jsonStrm(param);
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;

    reader["collectComments"] = false;
    if (!Json::parseFromStream(reader, jsonStrm, &root, &errs)) {
        INTELL_VOICE_LOG_ERROR("input str is not json");
        return false;
    }
    if ((!root.isMember("stage")) || (!root["stage"].isString())) {
        INTELL_VOICE_LOG_ERROR("miss key: stage");
        return false;
    }
    stage_ = root["stage"].asString();

    if (stage_ == VERIFY_STAGE) {
        if ((!root.isMember("bundle_name")) || (!root["bundle_name"].isString())) {
            INTELL_VOICE_LOG_ERROR("miss key: bundle_name");
            return false;
        }
        bundleName_ = root["bundle_name"].asString();

        if ((!root.isMember("ability_name")) || (!root["ability_name"].isString())) {
            INTELL_VOICE_LOG_ERROR("miss key: ability_name");
            return false;
        }
        abilityName_ = root["ability_name"].asString();

        if ((!root.isMember("start_time")) || (!root["start_time"].isInt())) {
            INTELL_VOICE_LOG_ERROR("miss key: start_time");
            return false;
        }
        startTime_ = root["start_time"].asInt();

        if ((!root.isMember("end_time")) || (!root["end_time"].isInt())) {
            INTELL_VOICE_LOG_ERROR("miss key: end_time");
            return false;
        }
        endTime_ = root["end_time"].asInt();
    } else if (stage_ != DIALOG_STAGE) {
        INTELL_VOICE_LOG_ERROR("stage value error");
        return false;
    }
    return true;
}

void ImplicitEnrollEngine::SetCallbackInner()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (adapterListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter listener is nullptr");
        return;
    }

    callback_ = sptr<IIntellVoiceEngineCallback>(new (std::nothrow) AdapterCallbackService(adapterListener_));
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return;
    }
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }
    adapter_->SetCallback(callback_);
}

void ImplicitEnrollEngine::SetCallback(sptr<IRemoteObject> object)
{
    INTELL_VOICE_LOG_INFO("enter");
    sptr<IIntelligentVoiceUpdateCallback> callback = iface_cast<IIntelligentVoiceUpdateCallback>(object);
    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback is nullptr");
        return;
    }
    StateMsg msg(SET_LISTENER, reinterpret_cast<void *>(&callback), sizeof(callback));
    Handle(msg);
}

int32_t ImplicitEnrollEngine::Attach(const IntellVoiceEngineInfo &info)
{
    INTELL_VOICE_LOG_INFO("attach");
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }
    SetDspFeatures();
    EngineUtil::SetImproveParam();

    IntellVoiceEngineAdapterInfo adapterInfo = {
        .wakeupPhrase = info.wakeupPhrase,
        .minBufSize = info.minBufSize,
        .sampleChannels = info.sampleChannels,
        .bitsPerSample = info.bitsPerSample,
        .sampleRate = info.sampleRate,
    };
    return adapter_->Attach(adapterInfo);
}

int32_t ImplicitEnrollEngine::SetParameter(const std::string &keyValueList)
{
    INTELL_VOICE_LOG_INFO("ImplicitEnrollEngine SetParameter:%{public}s", keyValueList.c_str());
    std::map<std::string, std::string> kvpairs;
    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);
    for (auto it : kvpairs) {
        if (it.first == "start_enroll") {
            StringParam paramStr(it.second);
            StateMsg msg(START_RECOGNIZE, &paramStr, sizeof(paramStr));
            Handle(msg);
            break;
        }
        if (it.first == "clear_data") {
            StringParam paramStr(keyValueList);
            StateMsg msg(SET_PARAM, &paramStr, sizeof(paramStr));
            Handle(msg);
            break;
        }
    }
    return 0;
}

int32_t ImplicitEnrollEngine::Detach(void)
{
    int32_t ret = TaskExecutor::AddSyncTask([this]() -> int32_t {
        INTELL_VOICE_LOG_INFO("enter");
        StateMsg msg(RELEASE);
        return Handle(msg);
    });
    return ret;
}
}
}