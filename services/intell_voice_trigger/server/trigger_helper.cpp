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
#include "trigger_helper.h"

#include <chrono>
#include <thread>
#ifdef SUPPORT_FAN_STATUS_LISTEN
#include <dlfcn.h>
#endif
#ifdef SUPPORT_TELEPHONY_SERVICE
#include "telephony_observer_client.h"
#include "state_registry_errors.h"
#include "telephony_types.h"
#include "call_manager_inner_type.h"
#endif
#include "audio_policy_manager.h"
#include "history_info_mgr.h"
#include "intell_voice_util.h"
#ifdef ONLY_FIRST_STAGE
#include "audio_system_client_engine_manager.h"
#endif
#include "iservice_registry.h"
#include "intell_voice_log.h"
#include "trigger_connector_mgr.h"
#include "wakeup_track.h"

#ifdef SUPPORT_WINDOW_MANAGER
#include "intell_voice_util.h"
#include "intell_voice_definitions.h"
#include "trigger_db_helper.h"
#endif

#undef LOG_TAG
#define LOG_TAG "TriggerHelper"

#ifdef SUPPORT_FAN_STATUS_LISTEN
using PerfReportFunc = int32_t (*)(uint32_t resId, const char* buffer, uint64_t len);
#endif

using namespace OHOS::HDI::IntelligentVoice::Trigger::V1_0;
using namespace OHOS::AudioStandard;
#ifdef SUPPORT_TELEPHONY_SERVICE
using namespace OHOS::Telephony;
#endif
#ifdef SUPPORT_WINDOW_MANAGER
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::IntellVoiceEngine;
#endif
using namespace OHOS::AudioStandard;
#ifdef POWER_MANAGER_ENABLE
using namespace OHOS::PowerMgr;
#endif
using namespace std;

namespace {
constexpr int BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401;
}

namespace OHOS {
namespace IntellVoiceTrigger {

static constexpr uint32_t HIBERNATE_WAIT_TIME = 500; // 500ms
const std::string ABILITY_CAMERA_CONCURRENCY = "camera_concurrency";
const std::string ABILITY_EXTENSION_CONCURRENCY = "extension_concurrency";
const std::string ABILITY_VOIP_CONCURRENCY = "voip_concurrency";
const std::string ABILITY_CALL_CONCURRENCY = "call_concurrency";
const std::string ABILITY_FAN_STATUS_LISTEN = "fan_status_listen";

#ifdef ONLY_FIRST_STAGE
static const std::string AUDIO_EFFECT_KEY = "audio_effect";
static const std::string WAKEUP_INPUT_GAIN_KEY = "wakeup_input_gain";
static const int32_t DEFAULT_WAKEUP_INPUT_GAIN = 10;
static const int32_t MX_WAKEUP_INPUT_GAIN = 22;
#endif

TriggerModelData::TriggerModelData(int32_t uuid)
{
    uuid_ = uuid;
}

TriggerModelData::~TriggerModelData()
{
}

void TriggerModelData::SetCallback(std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback)
{
    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback is nullptr");
        return;
    }
    callback_ = callback;
}

std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> TriggerModelData::GetCallback()
{
    return callback_;
}

void TriggerModelData::SetModel(std::shared_ptr<GenericTriggerModel> model)
{
    if (SameModel(model)) {
        INTELL_VOICE_LOG_INFO("same model not need to update");
        return;
    }
    model_ = model;
    model_->Print();
}

shared_ptr<GenericTriggerModel> TriggerModelData::GetModel()
{
    return model_;
}

bool TriggerModelData::SameModel(std::shared_ptr<GenericTriggerModel> model)
{
    if (model == nullptr || model_ == nullptr) {
        return false;
    }
    return model_->GetData() == model->GetData();
}

void TriggerModelData::SetState(ModelState state)
{
    state_ = state;
}

ModelState TriggerModelData::GetState() const
{
    return state_;
}

void TriggerModelData::SetModelHandle(int32_t handle)
{
    modelHandle_ = handle;
}

int32_t TriggerModelData::GetModelHandle() const
{
    return modelHandle_;
}

void TriggerModelData::SetRequested(bool requested)
{
    requested_ = requested;
}

bool TriggerModelData::GetRequested() const
{
    return requested_;
}

void TriggerModelData::Clear()
{
    callback_ = nullptr;
    state_ = MODEL_NOTLOADED;
}

void TriggerModelData::ClearCallback()
{
    callback_ = nullptr;
}

TriggerHelper::TriggerHelper()
{
}

TriggerHelper::~TriggerHelper()
{
#ifdef SUPPORT_FAN_STATUS_LISTEN
    if (perfGeniusHandle_ != nullptr) {
        dlclose(perfGeniusHandle_);
        perfGeniusHandle_ = nullptr;
    }
#endif
    modelDataMap_.clear();
}

std::shared_ptr<TriggerHelper> TriggerHelper::Create()
{
    return std::shared_ptr<TriggerHelper>(new (std::nothrow) TriggerHelper());
}

int32_t TriggerHelper::StartGenericRecognition(int32_t uuid, std::shared_ptr<GenericTriggerModel> model,
    shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    lock_guard<std::mutex> lock(mutex_);
    auto modelData = GetTriggerModelData(uuid);
    if (modelData == nullptr) {
        modelData = CreateTriggerModelData((uuid));
        if (modelData == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to create trigger model data");
            return -1;
        }
    }

    bool unload = !modelData->SameModel(model);
    int32_t ret = InitRecognition(modelData, unload);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to initialize recognition");
        return -1;
    }

    modelData->SetModel(model);
    modelData->SetCallback(callback);
    modelData->SetRequested(true);

    if (IsConflictSceneActive()) {
        INTELL_VOICE_LOG_INFO("conflict state, no need to start");
        return 0;
    }

    ret = PrepareForRecognition(modelData);
    if (ret != 0) {
        return ret;
    }

    return StartRecognition(modelData);
}

int32_t TriggerHelper::StopGenericRecognition(int32_t uuid, shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    lock_guard<std::mutex> lock(mutex_);
    auto modelData = GetTriggerModelData(uuid);
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get trigger model data");
        return -1;
    }
    modelData->SetRequested(false);
    int32_t ret = StopRecognition(modelData);
    if (ret != 0) {
        return ret;
    }
    modelData->ClearCallback();
    return ret;
}

void TriggerHelper::UnloadGenericTriggerModel(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter");
    lock_guard<std::mutex> lock(mutex_);
    auto modelData = GetTriggerModelData(uuid);
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_WARN("no trigger model data");
        return;
    }

    if (modelData->GetState() == MODEL_NOTLOADED) {
        INTELL_VOICE_LOG_INFO("model is not loaded");
        return;
    }
    StopRecognition(modelData);
    UnloadModel(modelData);
    modelDataMap_.erase(uuid);
}

int32_t TriggerHelper::SetParameter(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("enter");
    lock_guard<std::mutex> lock(mutex_);
    if (!GetModule()) {
        return -1;
    }

    return module_->SetParams(key, value);
}

int32_t TriggerHelper::SetParameterForInputPipe(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!GetModule()) {
        return -1;
    }

    return module_->SetParams(key, value);
}

std::string TriggerHelper::GetParameter(const std::string &key)
{
    INTELL_VOICE_LOG_INFO("enter");
    lock_guard<std::mutex> lock(mutex_);
    if (!GetModule()) {
        return "";
    }

    std::string value;
#ifdef SUPPORT_WINDOW_MANAGER
    if (GetParameterInner(key, value)) {
        return value;
    }
#endif

    auto ret = module_->GetParams(key, value);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get parameter");
        return "";
    }

    return value;
}

bool TriggerHelper::GetModule()
{
    if (module_ != nullptr) {
        return true;
    }
    const auto &connectMgr = TriggerConnectorMgr::GetInstance();
    if (connectMgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("connectMgr is nullptr");
        return false;
    }
    moduleDesc_ = connectMgr->ListConnectorModuleDescriptors();
    if (moduleDesc_.size() == 0) {
        INTELL_VOICE_LOG_ERROR("moduleDesc_ is empty");
        return false;
    }

    module_ = connectMgr->GetConnectorModule(moduleDesc_[0].adapterName, shared_from_this());
    if (module_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get connector module");
        return false;
    }
    return true;
}

int32_t TriggerHelper::InitRecognition(std::shared_ptr<TriggerModelData> modelData, bool unload)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (modelData->GetState() == MODEL_NOTLOADED) {
        return 0;
    }
    int32_t ret = StopRecognition(modelData);
    if (unload) {
        ret = UnloadModel(modelData);
        modelData->Clear();
    }

    return ret;
}

int32_t TriggerHelper::PrepareForRecognition(std::shared_ptr<TriggerModelData> modelData)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!GetModule()) {
        return -1;
    }

    if (LoadModel(modelData) != 0) {
        return -1;
    }
    return 0;
}

int32_t TriggerHelper::StartRecognition(shared_ptr<TriggerModelData> modelData)
{
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_ERROR("modelData is nullptr");
        return -1;
    }
    if (modelData->GetState() != MODEL_LOADED) {
        return 0;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (module_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("module_ is nullptr");
        return -1;
    }

#ifdef SUPPORT_WINDOW_MANAGER
    FoldStatusOperation(modelData);
#endif

#ifdef ONLY_FIRST_STAGE
    HandleWakeupInputGain();
#endif

    auto ret = module_->Start(modelData->GetModelHandle());
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to start recognition");
        return ret;
    }
    modelData->SetState(MODEL_STARTED);
    return ret;
}

#ifdef ONLY_FIRST_STAGE
void TriggerHelper::HandleWakeupInputGain()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::string country = IntellVoiceUtils::HistoryInfoMgr::GetInstance().GetStringKVPair(
        IntellVoiceUtils::KEY_COUNTRY_INFO_NAME);
    int32_t gain = DEFAULT_WAKEUP_INPUT_GAIN;
    if (country == "MX") {
        gain = MX_WAKEUP_INPUT_GAIN;
    }
    INTELL_VOICE_LOG_INFO("country:%{public}s, wakeup input gain:%{public}d", country.c_str(), gain);
    AudioSystemClientEngineManager::GetInstance().SetExtraParameters(AUDIO_EFFECT_KEY,
        {{WAKEUP_INPUT_GAIN_KEY, std::to_string(gain)}});
}
#endif

int32_t TriggerHelper::StopRecognition(shared_ptr<TriggerModelData> modelData)
{
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_ERROR("modelData is nullptr");
        return -1;
    }
    if (modelData->GetState() != MODEL_STARTED) {
        return 0;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (module_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("module_ is nullptr");
        return -1;
    }
    auto ret = module_->Stop(modelData->GetModelHandle());
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to stop");
        return ret;
    }

    modelData->SetState(MODEL_LOADED);
    return ret;
}

int32_t TriggerHelper::LoadModel(shared_ptr<TriggerModelData> modelData)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_ERROR("modelData is nullptr");
        return -1;
    }
    if (modelData->GetState() != MODEL_NOTLOADED) {
        INTELL_VOICE_LOG_WARN("model is already loaded");
        return 0;
    }

    if (module_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("module_ is nullptr");
        return -1;
    }

    int32_t handle;
    auto ret = module_->LoadModel(modelData->GetModel(), handle);
    if (ret != 0) {
        INTELL_VOICE_LOG_WARN("failed to load model, ret: %{public}d", ret);
        return ret;
    }
    modelData->SetModelHandle(handle);
    modelData->SetState(MODEL_LOADED);
    INTELL_VOICE_LOG_INFO("exit, handle: %{public}d", handle);
    return ret;
}

int32_t TriggerHelper::UnloadModel(shared_ptr<TriggerModelData> modelData)
{
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_ERROR("modelData is nullptr");
        return -1;
    }
    if (modelData->GetState() != MODEL_LOADED) {
        return 0;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (module_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("module_ is nullptr");
        return -1;
    }
    auto ret = module_->UnloadModel(modelData->GetModelHandle());
    modelData->SetState(MODEL_NOTLOADED);
    return ret;
}

shared_ptr<TriggerModelData> TriggerHelper::GetTriggerModelData(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, uuid is :%{public}d", uuid);
    auto it = modelDataMap_.find(uuid);
    if ((it == modelDataMap_.end()) || (it->second == nullptr)) {
        return nullptr;
    }

    return it->second;
}

shared_ptr<TriggerModelData> TriggerHelper::CreateTriggerModelData(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, uuid is :%{public}d", uuid);
    auto modelData = std::make_shared<TriggerModelData>(uuid);
    if (modelData == nullptr) {
        INTELL_VOICE_LOG_INFO("modelData is nullptr");
        return nullptr;
    }
    modelDataMap_.insert(std::make_pair(uuid, modelData));
    return modelData;
}

void TriggerHelper::OnRecognition(int32_t modelHandle, const IntellVoiceRecognitionEvent &event)
{
    INTELL_VOICE_LOG_INFO("enter, modelHandle:%{public}d", modelHandle);
    lock_guard<std::mutex> lock(mutex_);
    std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback = nullptr;
    for (auto iter : modelDataMap_) {
        if (iter.second == nullptr) {
            INTELL_VOICE_LOG_ERROR("uuid: %{public}d, model data is nullptr", iter.first);
            continue;
        }

        if (iter.second->GetModelHandle() == modelHandle) {
            iter.second->SetState(MODEL_LOADED);
            callback = iter.second->GetCallback();
            break;
        }
    }

    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger recognition callback is nullptr, modelHandle: %{public}d", modelHandle);
        return;
    }

    auto genericEvent = std::make_shared<GenericTriggerEvent>();
    if (genericEvent == nullptr) {
        INTELL_VOICE_LOG_ERROR("genericEvent is nullptr");
        return;
    }
    genericEvent->modelHandle_ = modelHandle;
    callback->OnGenericTriggerDetected(genericEvent);
}

bool TriggerHelper::IsConflictSceneActive()
{
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    INTELL_VOICE_LOG_INFO("callActive: %{public}d,systemHibernate: %{public}d",
        callActive_, systemHibernate_);
    INTELL_VOICE_LOG_INFO("captureActive: %{public}d, concurrentcapture: %{public}d",
    audioCaptureActive_, audioConcurrentCapture_);

    if(mVoipConcurrency_ && !mCallConcurrency_) {
        return systemHibernate_ || (audioScene_ != AUDIO_SCENE_DEFAULT && audioScene_ != AUDIO_SCENE_PHONE_CHAT
        && audioScene_ != AUDIO_SCENE_RINGING) || (!audioConcurrentCapture_ && audioCaptureActive_);
    }

    if(mVoipConcurrency_ && mCallConcurrency_) {
        return systemHibernate_ || (audioScene_ != AUDIO_SCENE_DEFAULT && audioScene_ != AUDIO_SCENE_PHONE_CHAT &&
        audioScene_ != AUDIO_SCENE_PHONE_CALL && audioScene_ != AUDIO_SCENE_RINGING) || 
       (!audioConcurrentCapture_ && audioCaptureActive_);
    }

    return callActive_ || systemHibernate_ || (audioScene_ != AUDIO_SCENE_DEFAULT) || 
       (!audioConcurrentCapture_ && audioCaptureActive_);
#else
    INTELL_VOICE_LOG_INFO("callActive: %{public}d, audioCaptureActive: %{public}d, systemHibernate: %{public}d",
        callActive_, audioCaptureActive_, systemHibernate_);
    return (callActive_ || audioCaptureActive_ || systemHibernate_ || (audioScene_ != AUDIO_SCENE_DEFAULT));
#endif
}

void TriggerHelper::OnUpdateAllRecognitionState()
{
    for (auto iter : modelDataMap_) {
        if (iter.second == nullptr) {
            INTELL_VOICE_LOG_ERROR("uuid: %{public}d, model data is nullptr", iter.first);
            continue;
        }
        bool needStart =
            (iter.second->GetRequested() && (!IsConflictSceneActive()));
        if (needStart == (iter.second->GetState() == MODEL_STARTED)) {
            INTELL_VOICE_LOG_INFO("no operation, needStart:%{public}d", needStart);
            continue;
        }
        if (needStart) {
            if (PrepareForRecognition(iter.second) != 0) {
                return;
            }
            IntellVoiceUtils::WakeupTrack::reportWakeupStatus(
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_OPEN,
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_AUDIO_SCENE_CHANGE);
            StartRecognition(iter.second);
        } else {
            IntellVoiceUtils::WakeupTrack::reportWakeupStatus(
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_CLOSE,
                OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_AUDIO_SCENE_CHANGE);
            StopRecognition(iter.second);
            if (systemHibernate_) {
                UnloadModel(iter.second);
                INTELL_VOICE_LOG_INFO("begin to wait");
                std::this_thread::sleep_for(std::chrono::milliseconds(HIBERNATE_WAIT_TIME));
                INTELL_VOICE_LOG_INFO("finish to wait");
            }
        }
    }
}

#ifdef SUPPORT_TELEPHONY_SERVICE
void TriggerHelper::AttachTelephonyObserver()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(telephonyMutex_);
    if (isTelephonyDetached_) {
        INTELL_VOICE_LOG_INFO("telephony is already detached");
        return;
    }
    telephonyObserver0_ = std::make_unique<TelephonyStateObserver>(shared_from_this()).release();
    if (telephonyObserver0_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("telephonyObserver0_ is nullptr");
        return;
    }
    auto res = TelephonyObserverClient::GetInstance().AddStateObserver(
        telephonyObserver0_, -1, TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE, true);
    if (res != TELEPHONY_SUCCESS) {
        INTELL_VOICE_LOG_ERROR("telephonyObserver0_ add failed");
    }
}

void TriggerHelper::DetachTelephonyObserver()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(telephonyMutex_);
    isTelephonyDetached_ = true;

    if (telephonyObserver0_ != nullptr) {
        Telephony::TelephonyObserverClient::GetInstance().RemoveStateObserver(
            -1, Telephony::TelephonyObserverBroker::OBSERVER_MASK_CALL_STATE);
        telephonyObserver0_ = nullptr;
    }
}

void TriggerHelper::OnCallStateUpdated(int32_t callState)
{
    {
        lock_guard<std::mutex> lock(mutex_);
        if (callState < static_cast<int32_t>(TelCallState::CALL_STATUS_UNKNOWN) ||
            callState > static_cast<int32_t>(TelCallState::CALL_STATUS_IDLE)) {
            INTELL_VOICE_LOG_ERROR("callstate err: %{public}d", callState);
            return;
        }

        bool curCallActive = (callState != static_cast<int32_t>(TelCallState::CALL_STATUS_DISCONNECTED) &&
              callState != static_cast<int32_t>(TelCallState::CALL_STATUS_IDLE) &&
              callState != static_cast<int32_t>(TelCallState::CALL_STATUS_UNKNOWN));
        INTELL_VOICE_LOG_INFO("state: %{public}d, callActive: %{public}d, curCallActive: %{public}d",
            callState,
            callActive_,
            curCallActive);
        if (callActive_ == curCallActive) {
            return;
        }

        callActive_ = curCallActive;
        OnUpdateAllRecognitionState();
    }
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    if (audioInputPipeCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioInputPipeCallback null");
        return;
    }
    if (mVoipConcurrency_) {
        INTELL_VOICE_LOG_INFO("support voip concurrency");
        return;
    }
    std::shared_ptr<AudioInputPipeInfo> pipeinfo;
    audioInputPipeCallback_->OnInputPipeChange(PIPE_CHANGE_TYPE_PIPE_STREAM, pipeinfo);
#endif
}

void TriggerHelper::TelephonyStateObserver::OnCallStateUpdated(
    int32_t slotId, int32_t callState, const std::u16string &phoneNumber)
{
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnCallStateUpdated(callState);
}
#endif

void TriggerHelper::OnAudioSceneChange(const AudioScene audioScene)
{
    {    
        lock_guard<std::mutex> lock(mutex_);
        if (audioScene_ == audioScene) {
            return;
        }

        audioScene_ = audioScene;
        OnUpdateAllRecognitionState();
    }
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    if (mVoipConcurrency_) {
        INTELL_VOICE_LOG_INFO("audio scene change, enter wakeup concurrency");
        std::shared_ptr<AudioInputPipeInfo> pipeinfo;
        audioInputPipeCallback_->OnInputPipeChange(PIPE_CHANGE_TYPE_PIPE_STREAM, pipeinfo);
        return;
    }
#endif
}

void TriggerHelper::AudioSceneChangeCallback::OnAudioSceneChange(const AudioScene audioScene)
{
    INTELL_VOICE_LOG_INFO("OnAudioScene change scene: %{public}d", audioScene);

    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnAudioSceneChange(audioScene);
}

#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
void TriggerHelper::ClearConcurrentControlList(bool isWhiteList, bool isBlackList)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (isWhiteList) {
        std::lock_guard<std::mutex> lock(whitelistMutex_);
        concurrentWhitelist_.clear();
        concurrentTypeSet_.clear();
    } else if (isBlackList) {
        std::lock_guard<std::mutex> lock(blacklistMutex_);
        concurrentBlacklist_.clear();
    }
}

void TriggerHelper::ParseSourceTypes(const Json::Value& sources,
                            std::set<SourceType>& outSourceTypes,
                            std::set<SourceType>& globalTypeSet) {
    if (!sources.isArray()) {
        return;
    }
    for (const auto& src : sources) {
        if (!src.isString()) {
            continue;
        }
        std::string srcStr = src.asString();
        try {
            SourceType type = static_cast<SourceType>(std::stoi(srcStr));
            outSourceTypes.insert(type);
            globalTypeSet.insert(type);
        } catch (const std::invalid_argument&) {
            INTELL_VOICE_LOG_WARN("white list invalid_argument");
        } catch (const std::out_of_range&) {
            INTELL_VOICE_LOG_WARN("white list out_of_range");
        }
    }
    return;
}

void TriggerHelper::SetConcurrencyControlList(concurrencyControlList listType)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::string value;
    if(listType == concurrencyControlList::WHITELIST) {
        concurrentWhitelist_.clear();
        concurrentTypeSet_.clear();
        value = IntellVoiceUtils::HistoryInfoMgr::GetInstance().GetStringKVPair("concurrentWhiteList");
    } else if(listType == concurrencyControlList::BLACKLIST) {
        concurrentBlacklist_.clear();
        value = IntellVoiceUtils::HistoryInfoMgr::GetInstance().GetStringKVPair("concurrentBlackList");
    }
    
    if (value.empty()) {
        INTELL_VOICE_LOG_ERROR("list is empty");
        return;
    }
    INTELL_VOICE_LOG_INFO("v:%{public}s", value.c_str());

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream iss(value);
    if (!Json::parseFromStream(builder, iss, &root, &errs)) {
        INTELL_VOICE_LOG_ERROR("Failed to parse JSON: %{public}s", errs.c_str());
        return;
    }
    for (const auto& item : root) {
        std::string bundleName = item["bundleName"].asString();
        std::set<SourceType> sourceTypes;
        std::set<SourceType> Types;
        if(listType == concurrencyControlList::WHITELIST) {
            ParseSourceTypes(item["sourceTypes"], sourceTypes, concurrentTypeSet_);
            concurrentWhitelist_[std::move(bundleName)] = std::move(sourceTypes);
        } else if(listType == concurrencyControlList::BLACKLIST) {
            ParseSourceTypes(item["sourceTypes"], sourceTypes, Types);
            concurrentBlacklist_[std::move(bundleName)] = std::move(sourceTypes);
        }
    }
}

void TriggerHelper::AttachAudioEngineListener() {
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(capturerMutex_);
    if (isCapturerDetached_) {
        INTELL_VOICE_LOG_INFO("capture event listener is already detached");
        return;
    }
    audioInputPipeCallback_ = std::make_shared<AudioInputPipeListener>(shared_from_this());
    if (audioInputPipeCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Memory Allocation Failed !!");
        return;
    }
    auto audioEngineManager = DelayedSingleton<AudioEngineClientManager>::GetInstance();
    if (audioEngineManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioEngineManager is nullptr");
        return;
    }
    int32_t ret = audioEngineManager->RegisterInputPipeChangeCallback(audioInputPipeCallback_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("RegisterInputPipeChangeCallback failed");
        return;
    }
    INTELL_VOICE_LOG_INFO("RegisterInputPipeChangeCallback success");
    if(IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_CAMERA_CONCURRENCY)) {
        mCameraConcurrency_ = true;
    }
    // 同样逻辑新加concurrent extension
    if(IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_EXTENSION_CONCURRENCY)) {
        mExtensionConcurrency_ = true;
    }
    if(IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_VOIP_CONCURRENCY)) {
        mVoipConcurrency_ = false;
    }
    if(IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_CALL_CONCURRENCY)) {
        mCallConcurrency_ = true;
        ExtensionConcurrentTypeSet_.insert(SOURCE_TYPE_VOICE_CALL);
    }

    std::vector<std::shared_ptr<AudioInputPipeInfo>> inputPipeInfos;
    ret = audioEngineManager->GetCurrentInputPipeChangeInfos(inputPipeInfos);
    if(ret != 0) {
        INTELL_VOICE_LOG_WARN("Get pipeinfos fail");
        return;
    }
    for(auto& pipeinfo : inputPipeInfos) {
        if(pipeinfo == nullptr) {
            INTELL_VOICE_LOG_WARN("null pipeinfo");
            continue;
        }
        mFirstProc = true;
        audioInputPipeCallback_->OnInputPipeChange(PIPE_CHANGE_TYPE_PIPE_STREAM,pipeinfo);
    }
}

void TriggerHelper::DetachAudioEngineListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(capturerMutex_);
    isCapturerDetached_ = true;
    auto audioEngineManager = DelayedSingleton<AudioEngineClientManager>::GetInstance();
    if (audioEngineManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioEngineManager is nullptr");
        return;
    }
    int32_t ret = audioEngineManager->UnregisterInputPipeChangeCallback(audioInputPipeCallback_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("UnregisterInputPipeChangeCallback failed");
    }
}

void TriggerHelper::AudioInputPipeListener::ExecuteStateTransition(
    int oldState, int newState, const std::shared_ptr<TriggerModelData> &modelData)
{
    auto method = methodTable[oldState][newState];
    std::string concurrencyKey;
    switch (method) {
        case captureMethod::SAME_STATE:
            INTELL_VOICE_LOG_INFO("same state");
            concurrencyKey = helper_->whitelistWhisperEnbale_? "enablewhisper" : "disablewhisper";
            if(helper_->mExtensionConcurrency_) {
                helper_->SetParameterForInputPipe("concurrent_capture", concurrencyKey);
            }
            break;
        case captureMethod::DO_CONCURRENT_CLOSE_SOURCE:
            INTELL_VOICE_LOG_INFO("do concurrent stop source");
            helper_->SetParameterForInputPipe("concurrent_capture", "stop");
            break;
        case captureMethod::DO_CONCURRENT_OPEN_SOURCE:
            INTELL_VOICE_LOG_INFO("do concurrent open source");
            concurrencyKey = helper_->whitelistWhisperEnbale_? "start_withwhisper" : "start_nowhisper";
            if (helper_->mExtensionConcurrency_) {
                helper_->SetParameterForInputPipe("concurrent_capture", concurrencyKey);
            } else {
                helper_->SetParameterForInputPipe("concurrent_capture", "start");
            }
            break;
        case captureMethod::DO_NO_CONCURRENT_STOP_SOURCE:
            INTELL_VOICE_LOG_INFO("do no concurrent stop source");
            helper_->OnUpdateAllRecognitionState();
            break;
        case captureMethod::DO_NO_CONCURRENT_OPEN_SOURCE:
            INTELL_VOICE_LOG_INFO("do no concurrent open source");
            helper_->OnUpdateAllRecognitionState();
            break;
        case captureMethod::DO_CONCURRENT_STOP_AND_NO_CONCURRENT_OPEN_SOURCE:
            INTELL_VOICE_LOG_INFO("do concurrent stop and no concurrent open");
            helper_->SetParameterForInputPipe("concurrent_capture", "stop");
            helper_->OnUpdateAllRecognitionState();
            break;
        case captureMethod::DO_NO_CONCURRENT_STOP_AND_CONCURRENT_OPEN_SOURCE:
            INTELL_VOICE_LOG_INFO("do no concurrent stop and concurrent open");
            if (modelData->GetState() != MODEL_STARTED) {
                helper_->StartRecognition(modelData);
            }
            concurrencyKey = helper_->whitelistWhisperEnbale_? "start_withwhisper" : "start_nowhisper";
            if(helper_->mExtensionConcurrency_) {
                helper_->SetParameterForInputPipe("concurrent_capture", concurrencyKey);
            } else {
                helper_->SetParameterForInputPipe("concurrent_capture", "start");
            }
            break;
    }
}

std::string TriggerHelper::GetBundleName(int uid) {
    std::string bundleName = "";
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    sptr<IRemoteObject> remoteObject = systemAbilityManager->CheckSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (remoteObject == nullptr) {
        INTELL_VOICE_LOG_ERROR("bundle manager service is unavailable");
        return bundleName;
    }
    sptr<AppExecFwk::IBundleMgr> bundleMgrProxy = OHOS::iface_cast<AppExecFwk::IBundleMgr>(remoteObject);
    if(!bundleMgrProxy->GetBundleNameForUid(uid, bundleName)) {
        INTELL_VOICE_LOG_WARN("get bundle name failed");
        return "";
    }
    return bundleName;
}

bool TriggerHelper::AudioInputPipeListener::CheckCallConflict()
{
    if (!helper_->mVoipConcurrency_ && helper_->callActive_) {
        return true;
    }

    if (helper_->mVoipConcurrency_ && !helper_->mCallConcurrency_ &&
        helper_->audioScene_ != AUDIO_SCENE_DEFAULT &&
        helper_->audioScene_ != AUDIO_SCENE_PHONE_CHAT &&
        helper_->audioScene_ != AUDIO_SCENE_RINGING) {
        return true;
    }

    if (helper_->mVoipConcurrency_ && helper_->mCallConcurrency_ &&
        helper_->audioScene_ != AUDIO_SCENE_DEFAULT &&
        helper_->audioScene_ != AUDIO_SCENE_PHONE_CHAT &&
        helper_->audioScene_ != AUDIO_SCENE_PHONE_CALL &&
        helper_->audioScene_ != AUDIO_SCENE_RINGING) {
        return true;
    }
    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckCallOnly() {
    if (!helper_->mCallConcurrency_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(helper_->playMutex_);
    if(helper_->audioScene_ == AUDIO_SCENE_PHONE_CALL && helper_->hasStreamBesidesVoice_) {
            INTELL_VOICE_LOG_INFO("voice and out stream, close wakeup");
            return true;
    }
    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckBlacklistConflict(const std::string &bundleName,
    const CapturerStreamInfo &streamInfo)
{
    if (!helper_->mExtensionConcurrency_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(helper_->blacklistMutex_);
    if (helper_->concurrentBlacklist_.empty()) {
        INTELL_VOICE_LOG_ERROR("no blacklist");
        helper_->SetConcurrencyControlList(concurrencyControlList::BLACKLIST);
    }

    auto bundleIter = helper_->concurrentBlacklist_.find(bundleName);
    if (bundleIter != helper_->concurrentBlacklist_.end() &&
        bundleIter->second.find(streamInfo.source_) != bundleIter->second.end() &&
        streamInfo.state_ == CAPTURER_RUNNING) {
        return true;
    }
    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckInWhitelist(const std::string &bundleName,
    const CapturerStreamInfo &streamInfo)
{
    std::lock_guard<std::mutex> lock(helper_->whitelistMutex_);
    if (helper_->concurrentTypeSet_.empty() || helper_->concurrentWhitelist_.empty()) {
        INTELL_VOICE_LOG_ERROR("no whitelist");
        helper_->SetConcurrencyControlList(concurrencyControlList::WHITELIST);
    }

    if (helper_->concurrentTypeSet_.find(streamInfo.source_) == helper_->concurrentTypeSet_.end()) {
        return false;
    }

    auto bundleIter = helper_->concurrentWhitelist_.find(bundleName);
    if (bundleIter != helper_->concurrentWhitelist_.end()) {
        if(bundleIter->second.find(streamInfo.source_) != bundleIter->second.end()) {
            return true;
        }
    }

    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckSourceWhisperEnable(const CapturerStreamInfo &streamInfo)
{
    if (streamInfo.state_ != CAPTURER_RUNNING ||
        streamInfo.mute_ ||
        streamInfo.source_ == SOURCE_TYPE_ULTRASONIC) {
        return true;
    }
 
    if (helper_->mCameraConcurrency_ &&
        streamInfo.source_ == SOURCE_TYPE_UNPROCESSED && streamInfo.mixWithWakeUp_) {
        return true;
    }
    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckSourceConcurrency(const CapturerStreamInfo &streamInfo)
{
    if (streamInfo.state_ != CAPTURER_RUNNING ||
        streamInfo.mute_ ||
        streamInfo.source_ == SOURCE_TYPE_ULTRASONIC) {
        return true;
    }

    if (helper_->mExtensionConcurrency_ &&
        helper_->ExtensionConcurrentTypeSet_.find(streamInfo.source_) != helper_->ExtensionConcurrentTypeSet_.end()) {
        return true;
    }

    if (helper_->mCameraConcurrency_ &&
        streamInfo.source_ == SOURCE_TYPE_UNPROCESSED && streamInfo.mixWithWakeUp_) {
        return true;
    }
    return false;
}

bool TriggerHelper::AudioInputPipeListener::CheckVoipOnly(const CapturerStreamInfo &streamInfo)
{
    if (!helper_->mVoipConcurrency_) {
        return false;
    }
    std::lock_guard<std::mutex> lock(helper_->playMutex_);
    if (streamInfo.state_ == CAPTURER_RUNNING && streamInfo.source_ == SOURCE_TYPE_VOICE_COMMUNICATION && 
        helper_->hasStreamBesidesVoip_) {
        INTELL_VOICE_LOG_INFO("voip and out stream, close wakeup");
        return true;
    }
    
    return false;
}

void TriggerHelper::AudioInputPipeListener::GetConcurrentState(bool &isConcurrent,
    std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>>  &inputPipeStateMap)
{
    if (CheckCallConflict()) {
        isConcurrent = false;
        return;
    }
    if(CheckCallOnly()) {
        isConcurrent = false;
        return;
    }

    for (const auto &info : inputPipeStateMap) {
        for (const auto &stream : info.second) {
            const CapturerStreamInfo &streamInfo = stream.second;
            std::string bundleName = helper_->GetBundleName(streamInfo.uid_);
            if (bundleName.empty()) {
                bundleName = "ERROR! CANT GET BUNDLENAME";
            }
            INTELL_VOICE_LOG_INFO("bundle name:%{public}s", bundleName.c_str());

            if (CheckBlacklistConflict(bundleName, streamInfo)) {
                isConcurrent = false;
                return;
            }

            if (CheckVoipOnly(streamInfo)) {
                isConcurrent = false;
                return;
            }

            if (CheckSourceConcurrency(streamInfo)) {
                isConcurrent = true;
                continue;
            }

            if (helper_->mExtensionConcurrency_) {
                isConcurrent = false;
                return;
            }
            if (CheckInWhitelist(bundleName, streamInfo)) {
                isConcurrent = true;
                continue;
            } else {
                isConcurrent = false;
                return;
            }
        }
    }

    isConcurrent = true;
}

bool TriggerHelper::AudioInputPipeListener::GetWhisperEnable(std::map<uint32_t, 
    std::map<uint32_t, CapturerStreamInfo>>& inputPipeStateMap)
{
    for (const auto &info : inputPipeStateMap) {
        for (const auto &stream : info.second) {
            const CapturerStreamInfo &streamInfo = stream.second;
            std::string bundleName = helper_->GetBundleName(streamInfo.uid_);

            if(CheckInWhitelist(bundleName, streamInfo)) {;
                continue;
            }
            if (CheckSourceWhisperEnable(streamInfo)) {
                continue;
            }
            return false;
        }
    }
    return true;
}

void TriggerHelper::AudioInputPipeListener::GetPipeInfo(
    std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>>& inputPipeinfo) 
{
    auto audioEngineManager = DelayedSingleton<AudioEngineClientManager>::GetInstance();
    if (audioEngineManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioEngineManager is nullptr");
        return;
    }
    std::vector<std::shared_ptr<AudioInputPipeInfo>> inputPipeInfoVec;
    auto ret = audioEngineManager->GetCurrentInputPipeChangeInfos(inputPipeInfoVec);
    if(ret != 0) {
        INTELL_VOICE_LOG_WARN("Get pipeinfos fail");
        return;
    }
    if(inputPipeInfoVec.empty()) {
        INTELL_VOICE_LOG_WARN("no input pipeinfo");
    }
    for(auto& pipeinfo : inputPipeInfoVec) {
        if(pipeinfo == nullptr) {
            INTELL_VOICE_LOG_WARN("null pipeinfo");
            continue;
        }
        auto streams = pipeinfo->GetStreams();
        auto flag = pipeinfo->GetRouteFlag();
        if(pipeinfo->GetAdapter()!=HDI_ADAPTER_TYPE_PRIMARY ||
            flag == AUDIO_FLAG_NONE || flag == AUDIO_INPUT_FLAG_OFFLOAD || flag == AUDIO_INPUT_FLAG_WAKEUP) {
            INTELL_VOICE_LOG_INFO("invalid capture");
            continue;
        }
        inputPipeinfo[flag] = streams;
    }
}

void TriggerHelper::AudioInputPipeListener::OnInputPipeChange(
    AudioPipeChangeType changeType,
            const std::shared_ptr<AudioInputPipeInfo> &changedPipeInfo)
{
    if (helper_ == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(helper_->mutex_);
    if(helper_->mFirstProc) {
        helper_->SetParameterForInputPipe("concurrent_capture", "stop");
        helper_->mFirstProc = false;
    }
    std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>> inputPipeinfo;
    GetPipeInfo(inputPipeinfo);
    bool isWhitelistWhisperEnbale = true;
    bool isConcurrent = true;
    bool isSourceRunning = false;
    if(helper_->mCallConcurrency_ && helper_->audioScene_ == AUDIO_SCENE_PHONE_CALL) {
        isSourceRunning = true;
    }
    for (auto& pipeState : inputPipeinfo) {
        INTELL_VOICE_LOG_INFO("flag: 0x%{public}x", pipeState.first);
        for(auto& stream : pipeState.second) {
            INTELL_VOICE_LOG_INFO("stmID: %{public}d, Type: %{public}d, stat: %{public}d, uid: %{public}u, m: %{public}d, mw:%{public}d",
            stream.first, stream.second.source_, stream.second.state_, 
            stream.second.uid_, stream.second.mute_, stream.second.mixWithWakeUp_);
                if (stream.second.state_ == CAPTURER_RUNNING) {
                    isSourceRunning = true;
                }
            }
    }
    GetConcurrentState(isConcurrent, inputPipeinfo);
    if(helper_->mExtensionConcurrency_) {
        isWhitelistWhisperEnbale = GetWhisperEnable(inputPipeinfo);
    }
    INTELL_VOICE_LOG_DEBUG("old state: %{public}d %{public}d  new state: %{public}d %{public}d",
                          helper_->audioConcurrentCapture_, helper_->audioCaptureActive_, isConcurrent,
                          isSourceRunning);
    int oldState = stateToInt(helper_->audioConcurrentCapture_, helper_->audioCaptureActive_);
    int newState = stateToInt(isConcurrent, isSourceRunning);
    if((isConcurrent == helper_->audioConcurrentCapture_) && (isSourceRunning == helper_->audioCaptureActive_) &&
        (!helper_->mExtensionConcurrency_ || (isWhitelistWhisperEnbale == helper_->whitelistWhisperEnbale_))) {
        INTELL_VOICE_LOG_DEBUG("same state no change");
        return;
    }
    helper_->audioConcurrentCapture_ = isConcurrent;
    helper_->audioCaptureActive_ = isSourceRunning;
    helper_->whitelistWhisperEnbale_ = isWhitelistWhisperEnbale;
    for (auto modelIter : helper_->modelDataMap_) {
        if(modelIter.second == nullptr) {
            INTELL_VOICE_LOG_ERROR("model data is nullpter uuid%{public}d", modelIter.first);
            continue;
        }
        ExecuteStateTransition(oldState, newState, modelIter.second);
    }
}
#else
void TriggerHelper::AttachAudioCaptureListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    audioCapturerSourceChangeCallback_ = std::make_shared<AudioCapturerSourceChangeCallback>(shared_from_this());
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager != nullptr) {
        audioSystemManager->SetAudioCapturerSourceCallback(audioCapturerSourceChangeCallback_);
    } else {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
    }
}

void TriggerHelper::DetachAudioCaptureListener()
{
    INTELL_VOICE_LOG_INFO("enter");

    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager != nullptr) {
        audioSystemManager->SetAudioCapturerSourceCallback(nullptr);
    } else {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is null");
    }
}

void TriggerHelper::OnCapturerStateChange(bool isActive)
{
    lock_guard<std::mutex> lock(mutex_);
    if (audioCaptureActive_ == isActive) {
        return;
    }

    audioCaptureActive_ = isActive;
    OnUpdateAllRecognitionState();
}

void TriggerHelper::AudioCapturerSourceChangeCallback::OnCapturerState(bool isActive)
{
    INTELL_VOICE_LOG_INFO("OnCapturerState active: %{public}d", isActive);

    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnCapturerStateChange(isActive);
}
#endif
void TriggerHelper::AudioRendererStateChangeCallbackImpl::OnRendererStateChange(
    const std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>> &audioRendererChangeInfos)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }
    std::map<int32_t, bool> stateMap;
    {
        std::lock_guard<std::mutex> lock(helper_->playMutex_);
        helper_->hasStreamBesidesVoip_ = false;
        helper_->hasStreamBesidesVoice_ = false;
        for (const auto &info : audioRendererChangeInfos) {
            if (info == nullptr) {
                INTELL_VOICE_LOG_ERROR("info is nullptr");
                continue;
            }
            bool isPlaying = false;
            if (info->rendererState == AudioStandard::RENDERER_RUNNING) {
                isPlaying = true;
                if(info->rendererInfo.streamUsage != AudioStandard::STREAM_USAGE_VOICE_COMMUNICATION
                && info->rendererInfo.streamUsage != AudioStandard::STREAM_USAGE_VIDEO_COMMUNICATION) {
                    helper_->hasStreamBesidesVoip_ = true;
                }
                if(info->rendererInfo.streamUsage != AudioStandard::STREAM_USAGE_VOICE_MODEM_COMMUNICATION) {
                    helper_->hasStreamBesidesVoice_ = true;
                }
            }
            if (stateMap.count(info->rendererInfo.streamUsage) == 0 || !stateMap[info->rendererInfo.streamUsage]) {
                stateMap[info->rendererInfo.streamUsage] = isPlaying;
            }
        }
    }
    for (auto iter : stateMap) {
        std::string key = iter.second ? "start_stream" : "stop_stream";
        if (rendererStateMap_.count(iter.first) == 0) {
            rendererStateMap_[iter.first] = iter.second;
            INTELL_VOICE_LOG_INFO("first change, usage:%{public}d, isPlaying:%{public}d",
                iter.first, iter.second);
            helper_->SetParameter(key, std::to_string(iter.first));
        } else {
            if (rendererStateMap_[iter.first] != iter.second) {
                INTELL_VOICE_LOG_INFO("state change, usage:%{public}d, isPlaying:%{public}d",
                    iter.first, iter.second);
                rendererStateMap_[iter.first] = iter.second;
                helper_->SetParameter(key, std::to_string(iter.first));
            }
        }
    }
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    if (helper_->mVoipConcurrency_) {
        std::shared_ptr<AudioInputPipeInfo> pipeinfo;
        helper_->audioInputPipeCallback_->OnInputPipeChange(PIPE_CHANGE_TYPE_PIPE_STREAM, pipeinfo);
        return;
    }
#endif
}

#ifdef POWER_MANAGER_ENABLE
void TriggerHelper::AttachHibernateObserver()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(hiberateMutex_);
    if (isHibernateDetached_) {
        INTELL_VOICE_LOG_INFO("system hibernate is already detached");
        return;
    }

    hibernateCallback_ = std::make_unique<HibernateCallback>(shared_from_this()).release();
    if (hibernateCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("hibernateCallback_ is nullptr");
        return;
    }
    auto res =  PowerMgrClient::GetInstance().RegisterSyncHibernateCallback(hibernateCallback_);
    if (!res) {
        INTELL_VOICE_LOG_ERROR("hibernateCallback_ register failed");
    }

    sleepCallback_ = std::make_unique<SleepCallback>(shared_from_this()).release();
    if (sleepCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("sleepCallback_ is nullptr");
        return;
    }
    res =  PowerMgrClient::GetInstance().RegisterSyncSleepCallback(sleepCallback_, SleepPriority::DEFAULT);
    if (!res) {
        INTELL_VOICE_LOG_ERROR("sleepCallback_ register failed");
    }
}

void TriggerHelper::DetachHibernateObserver()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(hiberateMutex_);

    isHibernateDetached_ = true;
    if (hibernateCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("hibernateCallback_ is nullptr");
        return;
    }
    auto res =  PowerMgrClient::GetInstance().UnRegisterSyncHibernateCallback(hibernateCallback_);
    if (!res) {
        INTELL_VOICE_LOG_ERROR("hibernateCallback_ unregister failed");
    }

    if (sleepCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("sleepCallback_ is nullptr");
        return;
    }
    res =  PowerMgrClient::GetInstance().UnRegisterSyncSleepCallback(sleepCallback_);
    if (!res) {
        INTELL_VOICE_LOG_ERROR("sleepCallback_ unregister failed");
    }
}


void TriggerHelper::HibernateCallback::OnSyncHibernate()
{
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnHibernateStateUpdated(true);
}

void TriggerHelper::HibernateCallback::OnSyncWakeup(bool /* hibernateResult */)
{
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnHibernateStateUpdated(false);
}

void TriggerHelper::SleepCallback::OnSyncSleep(bool onForceSleep)
{
    if (!onForceSleep) {
        INTELL_VOICE_LOG_INFO("not onForceSleep");
        return;
    }

    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnHibernateStateUpdated(true);
}

void TriggerHelper::SleepCallback::OnSyncWakeup(bool onForceSleep)
{
    if (!onForceSleep) {
        INTELL_VOICE_LOG_INFO("not onForceSleep");
        return;
    }

    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnHibernateStateUpdated(false);
}

void TriggerHelper::OnHibernateStateUpdated(bool isHibernate)
{
    lock_guard<std::mutex> lock(mutex_);
    if (systemHibernate_ == isHibernate) {
        return;
    }
    systemHibernate_ = isHibernate;
    OnUpdateAllRecognitionState();
}
#endif

void TriggerHelper::AttachAudioRendererEventListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(rendererMutex_);
    if (isRendererDetached_) {
        INTELL_VOICE_LOG_INFO("renderer event listener is already detached");
        return;
    }
    audioRendererStateChangeCallback_ = std::make_shared<AudioRendererStateChangeCallbackImpl>(shared_from_this());
    if (audioRendererStateChangeCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Memory Allocation Failed !!");
        return;
    }

    auto audioStreamManager = AudioStreamManager::GetInstance();
    if (audioStreamManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioStreamManager is nullptr");
        return;
    }
    int32_t ret = audioStreamManager->RegisterAudioRendererEventListener(getpid(),
        audioRendererStateChangeCallback_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("RegisterAudioRendererEventListener failed");
        return;
    }
    INTELL_VOICE_LOG_INFO("RegisterAudioRendererEventListener success");

    std::vector<std::shared_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
    audioStreamManager->GetCurrentRendererChangeInfos(audioRendererChangeInfos);
    audioRendererStateChangeCallback_->OnRendererStateChange(audioRendererChangeInfos);
}

void TriggerHelper::AttachAudioSceneEventListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(sceneMutex_);
    if (isSceneDetached_) {
        INTELL_VOICE_LOG_INFO("csene event listener is already detached");
        return;
    }
    audioSceneChangeCallback_ = std::make_shared<AudioSceneChangeCallback>(shared_from_this());
    if (audioSceneChangeCallback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Memory Allocation Failed !!");
        return;
    }

    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    int32_t ret = audioSystemManager->SetAudioSceneChangeCallback(audioSceneChangeCallback_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("RegisterAudioSceneListener failed");
        return;
    }
    INTELL_VOICE_LOG_INFO("RegisterAudioSceneListener success");
    AudioScene audioScene = audioSystemManager->GetAudioScene();
    audioSceneChangeCallback_->OnAudioSceneChange(audioScene);
}

void TriggerHelper::DetachAudioSceneEventListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(sceneMutex_);
    isSceneDetached_ = true;

    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    int32_t ret = audioSystemManager->UnsetAudioSceneChangeCallback(audioSceneChangeCallback_);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("UnregisterAudioRendererEventListener failed");
    }
}

void TriggerHelper::DetachAudioRendererEventListener()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(rendererMutex_);
    isRendererDetached_ = true;
    auto audioStreamManager = AudioStreamManager::GetInstance();
    if (audioStreamManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioStreamManager is nullptr");
        return;
    }
    int32_t ret = audioStreamManager->UnregisterAudioRendererEventListener(getpid());
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("UnregisterAudioRendererEventListener failed");
    }
}

#ifdef SUPPORT_WINDOW_MANAGER
std::string TriggerHelper::GetFoldStatusInfo()
{
    std::string value = isFoldable_ ? "is_foldable=true" : "is_foldable=false";
    value += ";";
    value += (curFoldStatus_ == FoldStatus::FOLDED) ? "fold_status=fold" : "fold_status=expand";
    return value;
}

bool TriggerHelper::GetParameterInner(const std::string &key, std::string &value)
{
    if (key == std::string("fold_status_info")) {
        value = GetFoldStatusInfo();
        INTELL_VOICE_LOG_INFO("get fold_status_info:%{public}s", value.c_str());
        return true;
    }

    return false;
}

void TriggerHelper::StartAllRecognition()
{
    for (auto iter : modelDataMap_) {
        if (iter.second == nullptr) {
            INTELL_VOICE_LOG_ERROR("audio_fold_status model data is null, uuid: %{public}d, ", iter.first);
            continue;
        }
        bool needStart = (iter.second->GetRequested() && (!IsConflictSceneActive()));
        if (!needStart) {
            INTELL_VOICE_LOG_INFO("audio_fold_status no need start uuid: %{public}d", iter.first);
            continue;
        }
        if (PrepareForRecognition(iter.second) != 0) {
            return;
        }
        StartRecognition(iter.second);
    }
}

void TriggerHelper::StopAllRecognition()
{
    for (auto iter : modelDataMap_) {
        if (iter.second == nullptr) {
            INTELL_VOICE_LOG_ERROR("audio_fold_status model data is null, uuid: %{public}d, ", iter.first);
            continue;
        }
        bool needStart = (iter.second->GetRequested() && (!IsConflictSceneActive()));
        if (!needStart) {
            INTELL_VOICE_LOG_INFO("audio_fold_status no need stop uuid: %{public}d", iter.first);
            continue;
        }

        StopRecognition(iter.second);
    }
}

void TriggerHelper::RestartAllRecognition()
{
    StopAllRecognition();
    StartAllRecognition();
}

void TriggerHelper::UpdateGenericTriggerModel(std::shared_ptr<GenericTriggerModel> model)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("trigger model is null");
        return;
    }

    if (!TriggerDbHelper::GetInstance().UpdateGenericTriggerModel(model)) {
        INTELL_VOICE_LOG_ERROR("failed to update generic model");
    }
}

std::shared_ptr<GenericTriggerModel> TriggerHelper::ReadWhisperModel()
{
    std::string modelPath = (curFoldStatus_ == FoldStatus::FOLDED) ? WHISPER_MODEL_PATH_VDE : WHISPER_MODEL_PATH;
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint32_t size = 0;
    if (!IntellVoiceUtil::ReadFile(modelPath, buffer, size)) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status read model failed");
        return nullptr;
    }
    std::vector<uint8_t> data(buffer.get(), buffer.get() + size);
    std::shared_ptr<GenericTriggerModel> model = std::make_shared<GenericTriggerModel>(
        OHOS::IntellVoiceEngine::PROXIMAL_WAKEUP_MODEL_UUID,
        TriggerModel::TriggerModelVersion::MODLE_VERSION_2,
        TriggerModelType::PROXIMAL_WAKEUP_TYPE);
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status model null");
        return nullptr;
    }
    model->SetData(data);
    return model;
}

void TriggerHelper::ReLoadWhisperModel(shared_ptr<TriggerModelData> modelData)
{
    if (modelData->uuid_ != OHOS::IntellVoiceEngine::PROXIMAL_WAKEUP_MODEL_UUID) {
        return;
    }

    std::shared_ptr<GenericTriggerModel> model = ReadWhisperModel();
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status read model failed");
        return;
    }
    UpdateGenericTriggerModel(model);
    if (!modelData->SameModel(model)) {
        UnloadModel(modelData);
        modelData->SetModel(model);
        LoadModel(modelData);
    }
    INTELL_VOICE_LOG_INFO("audio_fold_status reload model success");
}

void TriggerHelper::FoldStatusOperation(shared_ptr<TriggerModelData> modelData)
{
    if (!isFoldable_) {
        return;
    }

    ReLoadWhisperModel(modelData);

    auto firstElement = modelDataMap_.begin();
    if (firstElement->second->uuid_ == modelData->uuid_) {
        SetFoldStatus();
    }
}

void TriggerHelper::SetFoldStatus()
{
    std::string key = "is_folded";
    std::string value = (curFoldStatus_ == FoldStatus::FOLDED) ? "true" : "false";
    if (!GetModule()) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status get module failed");
        return;
    }

    if (module_->SetParams(key, value) != 0) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status set failed");
        return;
    }

    INTELL_VOICE_LOG_INFO("audio_fold_status set to hal success is_fold:%{public}s", value.c_str());
}

void TriggerHelper::OnFoldStatusChanged(FoldStatus foldStatus)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FoldStatus newFoldStatus = foldStatus;
    if (foldStatus == FoldStatus::HALF_FOLD) {
        newFoldStatus = FoldStatus::EXPAND;
    }
    if (curFoldStatus_ == newFoldStatus) {
        INTELL_VOICE_LOG_INFO("audio_fold_status same, no deed set");
        return;
    }
    curFoldStatus_ = newFoldStatus;
    bool isFolded = (curFoldStatus_ == FoldStatus::FOLDED) ? true : false;
    RestartAllRecognition();
    INTELL_VOICE_LOG_INFO("audio_fold_status change, status: %{public}d, is_fold: %{public}d", foldStatus, isFolded);
}

void TriggerHelper::FoldStatusListener::OnFoldStatusChanged(FoldStatus foldStatus)
{
    INTELL_VOICE_LOG_INFO("audio_fold_status on change status: %{public}d", foldStatus);
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper is nullptr");
        return;
    }

    helper_->OnFoldStatusChanged(foldStatus);
}

void TriggerHelper::RegisterFoldStatusListener()
{
    foldStatusListener_ = std::make_unique<FoldStatusListener>(shared_from_this()).release();
    if (foldStatusListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status listener is nullptr");
        return;
    }

    auto ret = OHOS::Rosen::DisplayManagerLite::GetInstance().RegisterFoldStatusListener(foldStatusListener_);
    if (ret != OHOS::Rosen::DMError::DM_OK) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status register listener failed");
        foldStatusListener_ = nullptr;
    } else {
        INTELL_VOICE_LOG_INFO("audio_fold_status register listener success");
    }
}

void TriggerHelper::AttachFoldStatusListener()
{
    INTELL_VOICE_LOG_INFO("audio_fold_status attach enter");
    std::lock_guard<std::mutex> lock(foldStatusMutex_);
    if (isFoldStatusDetached_) {
        INTELL_VOICE_LOG_INFO("audio_fold_status already detached");
        return;
    }

    bool isFoldable = OHOS::Rosen::DisplayManagerLite::GetInstance().IsFoldable();
    bool isFileExist = IntellVoiceUtil::IsFileExist(WHISPER_MODEL_PATH_VDE);
    isFoldable_ = (isFileExist && isFoldable) ? true : false;
    INTELL_VOICE_LOG_INFO("audio_fold_status isFoldable:%{public}d, isFileExist:%{public}d", isFoldable, isFileExist);
    if (!isFoldable_) {
        return;
    }

    RegisterFoldStatusListener();

    curFoldStatus_ = OHOS::Rosen::DisplayManagerLite::GetInstance().GetFoldStatus();
    if (curFoldStatus_ == FoldStatus::FOLDED) {
        RestartAllRecognition();
    }
    INTELL_VOICE_LOG_INFO("audio_fold_status attach success");
}

void TriggerHelper::DetachFoldStatusListener()
{
    INTELL_VOICE_LOG_INFO("audio_fold_status detach enter");
    std::lock_guard<std::mutex> lock(foldStatusMutex_);
    isFoldStatusDetached_ = true;

    if (foldStatusListener_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status listener is null");
        return;
    }

    auto ret = OHOS::Rosen::DisplayManagerLite::GetInstance().UnregisterFoldStatusListener(foldStatusListener_);
    if (ret != OHOS::Rosen::DMError::DM_OK) {
        INTELL_VOICE_LOG_ERROR("audio_fold_status detach failed");
    }
    foldStatusListener_ = nullptr;
    INTELL_VOICE_LOG_INFO("audio_fold_status detach success");
}
#endif

#ifdef SUPPORT_FAN_STATUS_LISTEN

namespace {
std::shared_ptr<TriggerHelper> g_fanHelperInstance = nullptr;
}

void TriggerHelper::AttachFanStateListener()
{
    if (!IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_FAN_STATUS_LISTEN)) {
        return;
    }
    INTELL_VOICE_LOG_INFO("enter");
    {
        std::lock_guard<std::mutex> lock(fanStateMutex_);
        if (isFanListenerDetached_) {
            INTELL_VOICE_LOG_INFO("fan state listener is already detached");
            return;
        }

        if (perfGeniusHandle_ == nullptr) {
            perfGeniusHandle_ = dlopen("libperfgenius_client.z.so", RTLD_NOW);
            if (perfGeniusHandle_ == nullptr) {
                INTELL_VOICE_LOG_ERROR("dlopen libperfgenius_client.z.so failed");
                return;
            }
        }

        using RegisterPerfCallbackFunc = int32_t (*)(uint32_t, PerfReportFunc);
        auto registerFunc = reinterpret_cast<RegisterPerfCallbackFunc>(
            dlsym(perfGeniusHandle_, "RegisterPerfCallback"));
        if (registerFunc == nullptr) {
            INTELL_VOICE_LOG_ERROR("dlsym RegisterPerfCallback failed");
            dlclose(perfGeniusHandle_);
            perfGeniusHandle_ = nullptr;
            return;
        }

        constexpr int32_t REGISTER_RETRY_COUNT = 3;
        int32_t ret = -1;
        for (int retry = 0; retry < REGISTER_RETRY_COUNT; ++retry) {
            ret = registerFunc(FAN_RES_ID, TriggerHelper::OnFanStateChanged);
            if (ret == 0) {
                break;
            }
            INTELL_VOICE_LOG_WARN("RegisterPerfCallback failed, retry: %{public}d, ret: %{public}d", retry + 1, ret);
        }
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("RegisterPerfCallback failed after %{public}d retries, ret: %{public}d",
                REGISTER_RETRY_COUNT, ret);
            dlclose(perfGeniusHandle_);
            perfGeniusHandle_ = nullptr;
            return;
        }

        g_fanHelperInstance = shared_from_this();
    }
    // fanStateMutex_ 已释放，PerfCmdHandle 触发的回调可正常获取锁

    using PerfCmdHandleFunc = int32_t (*)(const std::vector<int32_t>&,
        const std::vector<int64_t>&, const std::vector<int64_t>&, const std::string&);
    auto cmdHandleFunc = reinterpret_cast<PerfCmdHandleFunc>(
        dlsym(perfGeniusHandle_, "PerfCmdHandle"));
    if (cmdHandleFunc != nullptr) {
        int32_t ret = cmdHandleFunc({static_cast<int32_t>(FAN_RES_ID)}, {0}, {0}, "");
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("PerfCmdHandle trigger initial report failed, ret: %{public}d", ret);
        }
    }

    INTELL_VOICE_LOG_INFO("AttachFanStateListener success");

    SyncFanState();
}

void TriggerHelper::SyncFanState()
{
    std::lock_guard<std::mutex> lock(fanStateMutex_);
    if (!fanRunning_) {
        INTELL_VOICE_LOG_INFO("fan is not running, no need to sync");
        return;
    }

    if (!GetModule()) {
        INTELL_VOICE_LOG_ERROR("module is nullptr, skip fan state sync");
        return;
    }

    std::string key = "fan_status_change";
    std::string value = "start";
    INTELL_VOICE_LOG_INFO("sync fan state to HAL: %{public}s", value.c_str());
    module_->SetParams(key, value);
}

void TriggerHelper::DetachFanStateListener()
{
    if (!IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_FAN_STATUS_LISTEN)) {
        return;
    }
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(fanStateMutex_);
    isFanListenerDetached_ = true;

    if (perfGeniusHandle_ == nullptr) {
        return;
    }

    using UnregisterPerfCallbackFunc = int32_t (*)(uint32_t, PerfReportFunc);
    auto unregisterFunc = reinterpret_cast<UnregisterPerfCallbackFunc>(
        dlsym(perfGeniusHandle_, "UnregisterPerfCallback"));
    if (unregisterFunc != nullptr) {
        int32_t ret = unregisterFunc(FAN_RES_ID, TriggerHelper::OnFanStateChanged);
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("UnregisterPerfCallback failed, ret: %{public}d", ret);
        }
    }

    g_fanHelperInstance = nullptr;
    dlclose(perfGeniusHandle_);
    perfGeniusHandle_ = nullptr;
    INTELL_VOICE_LOG_INFO("DetachFanStateListener done");
}

void TriggerHelper::HandleFanStateChanged(uint32_t resId, const char* buffer, uint64_t len)
{
    std::lock_guard<std::mutex> lock(fanStateMutex_);
    if (buffer == nullptr || len == 0) {
        INTELL_VOICE_LOG_ERROR("invalid fan state buffer");
        return;
    }

    if (len > PERF_REPORT_LEN_MAX) {
        INTELL_VOICE_LOG_ERROR("buffer length exceed max, len: %{public}llu", static_cast<unsigned long long>(len));
        return;
    }

    size_t fanStatusSize = sizeof(FanStatus);
    if (len % fanStatusSize != 0) {
        INTELL_VOICE_LOG_ERROR("buffer length not multiple of FanStatus size");
        return;
    }

    int32_t power_val = -1;
    size_t count = len / fanStatusSize;
    const FanStatus *fanStatusArr = reinterpret_cast<const FanStatus *>(buffer);
    for (size_t i = 0; i < count; i++) {
        if (fanStatusArr[i].type == DATA_TYPE_POWER) {
            power_val = fanStatusArr[i].data;
            break;
        }
    }

    bool curFanRunning = (power_val == DATA_TYPE_POWER_VAL_ON);
    INTELL_VOICE_LOG_INFO("fan power_val: %{public}d, fanRunning: %{public}d -> %{public}d",
        power_val, fanRunning_, curFanRunning);

    if (fanRunning_ == curFanRunning) {
        return;
    }
    fanRunning_ = curFanRunning;

    if (!GetModule()) {
        INTELL_VOICE_LOG_ERROR("module is nullptr, skip fan_status_change");
        return;
    }
    std::string key = "fan_status_change";
    std::string value = curFanRunning ? "start" : "stop";
    INTELL_VOICE_LOG_INFO("set fan_status_change: %{public}s", value.c_str());
    module_->SetParams(key, value);
}

int32_t TriggerHelper::OnFanStateChanged(uint32_t resId, const char* buffer, uint64_t len)
{
    INTELL_VOICE_LOG_INFO("fan state change callback, resId: %{public}u", resId);
    if (g_fanHelperInstance == nullptr) {
        INTELL_VOICE_LOG_ERROR("g_fanHelperInstance is nullptr");
        return -1;
    }
    g_fanHelperInstance->HandleFanStateChanged(resId, buffer, len);
    return 0;
}
#endif
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
