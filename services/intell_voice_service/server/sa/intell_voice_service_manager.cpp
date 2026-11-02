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
#include "intell_voice_service_manager.h"

#include <vector>
#include <fstream>
#include <cstdio>
#include <any>
#include "intell_voice_log.h"
#include "intell_voice_util.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "intell_voice_generic_factory.h"
#include "memory_guard.h"
#include "string_util.h"
#include "history_info_mgr.h"
#include "wakeup_track.h"

#define LOG_TAG "IntellVoiceServiceManager"

using namespace OHOS::IntellVoiceTrigger;
using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int32_t WAIT_INTERVAL = 500; // 500ms
static constexpr uint32_t MAX_TASK_NUM = 2048;
static constexpr uint32_t WAIT_SWITCH_TIME = 2000;  // 2000ms
static constexpr uint32_t WAIT_RECORD_START_TIME = 10000; // 10s
static const std::string STOP_ALL_RECOGNITION = "stop_all_recognition";
static const std::string SHORT_WORD_SWITCH = "short_word_switch";
template<typename T, typename E>
IntellVoiceServiceManager<T, E>::IntellVoiceServiceManager() : TaskExecutor("ServMgrThread", MAX_TASK_NUM)
{
    TaskExecutor::StartThread();
#if defined(ENGINE_ENABLE) || defined(FIRST_STAGE_ONESHOT_ENABLE)
    RegisterEngineCallbacks();
#endif
#ifdef TRIGGER_ENABLE
    RegisterTriggerCallbacks();
#endif
#ifdef USE_FFRT
    taskQueue_ = std::make_shared<ffrt::queue>("ServMgrQueue");
#endif
    InitParameterHandlerMap();
}

template<typename T, typename E>
IntellVoiceServiceManager<T, E>::~IntellVoiceServiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
#ifdef USE_FFRT
    if (taskQueue_ != nullptr) {
        taskQueue_.reset();
    }
#endif
    TaskExecutor::StopThread();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::CreateSwitchProvider()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(switchMutex_);
    switchProvider_ = UniquePtrFactory<SwitchProvider>::CreateInstance();
    if (switchProvider_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("switchProvider_ is nullptr");
        return;
    }
    RegisterObserver(WAKEUP_KEY);
    RegisterObserver(WHISPER_KEY);
    if (IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_SHORT_WORD))
        RegisterObserver(SHORTWORD_KEY);
    if (IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_DUAL_MIC))
        RegisterObserver(DUALMIC_KEY);
    if (switchProvider_->QuerySwitchStatus(WAKEUP_KEY) || switchProvider_->QuerySwitchStatus(WHISPER_KEY))
        WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_OPEN,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_INIT);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::RegisterObserver(const std::string &switchKey)
{
    if (switchObserver_[switchKey] == nullptr) {
        switchObserver_[switchKey] = sptr<SwitchObserver>(new (std::nothrow) SwitchObserver());
        if (switchObserver_[switchKey] == nullptr) {
            INTELL_VOICE_LOG_ERROR("switchObserver_ is nullptr");
            return;
        }
        switchObserver_[switchKey]->SetUpdateFunc([this, switchKey]() {
            OnSwitchChange(switchKey);
        });
    }

    switchProvider_->RegisterObserver(switchObserver_[switchKey], switchKey);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ReleaseSwitchProvider()
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    if (switchProvider_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("switchProvider_ is nullptr");
        return;
    }

    for (auto it : switchObserver_) {
        if (it.second != nullptr) {
            switchProvider_->UnregisterObserver(it.second, it.first);
        }
    }
    switchProvider_ = nullptr;
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::StartDetection(int32_t uuid)
{
    if (E::AnyEngineExist({INTELL_VOICE_ENROLL, INTELL_VOICE_UPDATE})) {
        INTELL_VOICE_LOG_INFO("enroll engine or update engine exist, do nothing");
        return false;
    }
    if (!QuerySwitchByUuid(uuid)) {
        INTELL_VOICE_LOG_INFO("switch is off, uuid is %{public}d", uuid);
        return false;
    }

    int32_t ret = T::StartDetection(uuid);
    if (ret == 1) {
        isStarted_[uuid] = true;
        return true;
    }

    if (ret == 0) {
        return AddStartDetectionTask(uuid);
    }

    return false;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::StopDetection(int32_t uuid)
{
    T::StopDetection(uuid);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::QuerySwitchStatus(const std::string &key)
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    if (!IsSwitchKeyValid(key)) {
        INTELL_VOICE_LOG_ERROR("invalid key :%{public}s", key.c_str());
        return false;
    }

    if (switchProvider_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("switchProvider_ is nullptr");
        return false;
    }
    return switchProvider_->QuerySwitchStatus(key);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::IsSwitchError(const std::string &key)
{
    std::lock_guard<std::mutex> lock(switchMutex_);
    if (switchProvider_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("switchProvider_ is nullptr");
        return true;
    }

    return switchProvider_->IsSwitchError(key);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnSwitchChange(const std::string &switchKey)
{
    if (switchKey == WAKEUP_KEY) {
        if (QuerySwitchStatus(switchKey)) {
            if (!QuerySwitchStatus(WHISPER_KEY)) {
                WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_OPEN,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_USER_SETTING);
            }
            NoiftySwitchOnToPowerChange();
            HandleSwitchOn(false, VOICE_WAKEUP_MODEL_UUID, false);
        } else {
            if (!QuerySwitchStatus(WHISPER_KEY)) {
                WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_CLOSE,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_USER_SETTING);
            }
            HandleSwitchOff(false, VOICE_WAKEUP_MODEL_UUID);
            INTELL_VOICE_LOG_INFO("switch off process finish");
            HistoryInfoMgr::GetInstance().DeleteKey(TRIGGER_PARAMS);
            HandleUnloadIntellVoiceService(false);
        }
        switchProvider_->QueryAndLogReason();
    } else if (switchKey == WHISPER_KEY) {
        if (QuerySwitchStatus(switchKey)) {
            if (!QuerySwitchStatus(WAKEUP_KEY)) {
                WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_OPEN,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_USER_SETTING);
            }
            NoiftySwitchOnToPowerChange();
            HandleSwitchOn(false, PROXIMAL_WAKEUP_MODEL_UUID, false);
        } else {
            if (!QuerySwitchStatus(WAKEUP_KEY)) {
                WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_CLOSE,
                    OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_USER_SETTING);
            }
            HandleSwitchOff(false, PROXIMAL_WAKEUP_MODEL_UUID);
            HandleUnloadIntellVoiceService(false);
        }
        switchProvider_->QueryAndLogReason();
    } else if (switchKey == SHORTWORD_KEY) {
        TaskExecutor::AddSyncTask([this]() -> int32_t {
            INTELL_VOICE_LOG_INFO("short word switch change");
            if (E::AnyEngineExist({INTELL_VOICE_ENROLL, INTELL_VOICE_UPDATE})) {
                INTELL_VOICE_LOG_INFO("enroll engine or update engine exist, do nothing");
                return 0;
            }
            StopDetection(VOICE_WAKEUP_MODEL_UUID);
            StopDetection(PROXIMAL_WAKEUP_MODEL_UUID);
            E::CreateOrResetWakeupEngine();
            SetShortWordStatus();
            StartDetection(VOICE_WAKEUP_MODEL_UUID);
            StartDetection(PROXIMAL_WAKEUP_MODEL_UUID);
            return 0;
        });
    } else if (switchKey == DUALMIC_KEY) {
        TaskExecutor::AddSyncTask([this]() -> int32_t {
            SetDualMicStatus();
            return 0;
        });
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::SetShortWordStatus()
{
    if (!IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_SHORT_WORD)) {
        return;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (QuerySwitchStatus(SHORTWORD_KEY)) {
        INTELL_VOICE_LOG_INFO("short_word_switch true");
        T::SetParameter(SHORT_WORD_SWITCH, "1");
    } else {
        INTELL_VOICE_LOG_INFO("short_word_switch false");
        T::SetParameter(SHORT_WORD_SWITCH, "0");
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::SetDualMicStatus()
{
    if (!IntellVoiceUtils::IntellVoiceUtil::GetSupportAbility(ABILITY_DUAL_MIC)) {
        return;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (QuerySwitchStatus(DUALMIC_KEY)) {
        INTELL_VOICE_LOG_INFO("dual_mic_switch true");
        E::SubscribeSwingEvent("dualMic");
        T::SetParameter(KEY_DUALMIC_ENABLE, "true");
    } else {
        INTELL_VOICE_LOG_INFO("dual_mic_switch false");
        E::UnSubscribeSwingEvent("dualMic");
        T::SetParameter(KEY_DUALMIC_ENABLE, "false");
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ProcBreathModel()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint32_t size = 0;
    if (!IntellVoiceUtil::ReadFile(WHISPER_MODEL_PATH, buffer, size)) {
        return;
    }
    std::vector<uint8_t> data(buffer.get(), buffer.get() + size);
    T::UpdateModel(data, PROXIMAL_WAKEUP_MODEL_UUID, TriggerModelType::PROXIMAL_WAKEUP_TYPE);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::ClearUserData()
{
    INTELL_VOICE_LOG_INFO("enter");
    T::DeleteModel(VOICE_WAKEUP_MODEL_UUID);
    T::DeleteModel(PROXIMAL_WAKEUP_MODEL_UUID);
    return TaskExecutor::AddSyncTask([this]() -> int32_t {
        E::ClearUserDataInner();
        HistoryInfoMgr::GetInstance().DeleteKey({KEY_WAKEUP_ENGINE_BUNDLE_NAME, KEY_WAKEUP_ENGINE_ABILITY_NAME,
            KEY_WAKEUP_VESRION, KEY_LANGUAGE, KEY_AREA, KEY_WAKEUP_PHRASE, KEY_WHISPER_VPR});
        HistoryInfoMgr::GetInstance().DeleteKey(TRIGGER_PARAMS);
        return UnloadIntellVoiceService();
    });
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::RegisterWakeupDevice(const std::string &macAddress,
    const WakeupDeviceInfo &info)
{
    INTELL_VOICE_LOG_INFO("enter");
    return E::RegisterWakeupDevice(macAddress, info);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::DeregisterWakeupDevice(const std::string &macAddress)
{
    INTELL_VOICE_LOG_INFO("enter");
    return E::DeregisterWakeupDevice(macAddress);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::IsNeedToUnloadService()
{
    if (E::AnyEngineExist({INTELL_VOICE_ENROLL, INTELL_VOICE_UPDATE})) {
        INTELL_VOICE_LOG_DEBUG("enroll engine or update engine exist, no need to unload service");
        return false;
    }
    if ((QuerySwitchStatus(WAKEUP_KEY)) || (QuerySwitchStatus(WHISPER_KEY))) {
        INTELL_VOICE_LOG_DEBUG("switch is on, no need to unload service");
        return false;
    }

    return true;
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::UnloadIntellVoiceService()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (!IsNeedToUnloadService()) {
        return 0;
    }
    serviceUnloading_ = true;
    std::thread([]() {
        auto systemAbilityMgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        if (systemAbilityMgr == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to get systemabilitymanager");
            return;
        }

        int32_t ret = systemAbilityMgr->UnloadSystemAbility(INTELL_VOICE_SERVICE_ID);
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("failed to unload intellvoice service, ret: %{public}d", ret);
            return;
        }
        INTELL_VOICE_LOG_INFO("success to notify samgr to unload intell voice service");
    }).detach();

    return 0;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleUnloadIntellVoiceService(bool isAsync)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (isAsync) {
        TaskExecutor::AddAsyncTask([this]() { UnloadIntellVoiceService(); });
    } else {
        TaskExecutor::AddSyncTask([this]() -> int32_t { return UnloadIntellVoiceService(); });
    }
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::HandleOnIdle()
{
    return TaskExecutor::AddSyncTask([this]() -> bool {
            bool needUnload = IsNeedToUnloadService();
            serviceUnloading_ = needUnload;
            return needUnload;
        });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::NoiftySwitchOnToPowerChange()
{
    {
        std::unique_lock<std::mutex> lock(powerModeChangeMutex_);
        if (!notifyPowerModeChange_) {
            INTELL_VOICE_LOG_INFO("no need to notify");
            return;
        }
    }

    powerModeChangeCv_.notify_all();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandlePowerSaveModeChange()
{
    if ((QuerySwitchStatus(WAKEUP_KEY)) || (QuerySwitchStatus(WHISPER_KEY))) {
        INTELL_VOICE_LOG_INFO("switch is on, no need to process");
        return;
    }

    std::thread([&]() {
        std::unique_lock<std::mutex> lock(powerModeChangeMutex_);
        if ((QuerySwitchStatus(WAKEUP_KEY)) || (QuerySwitchStatus(WHISPER_KEY))) {
            INTELL_VOICE_LOG_INFO("switch is on, no need to process");
            return;
        }
        notifyPowerModeChange_ = true;
        if (powerModeChangeCv_.wait_for(lock, std::chrono::milliseconds(WAIT_SWITCH_TIME),
            [this] { return ((this->QuerySwitchStatus(WAKEUP_KEY)) || (this->QuerySwitchStatus(WHISPER_KEY))); })) {
            INTELL_VOICE_LOG_INFO("switch is on, do nothing");
            notifyPowerModeChange_ = false;
            return;
        }
        INTELL_VOICE_LOG_WARN("wait time out, switch is off, need to unload service");
        WakeupTrack::reportWakeupStatus(OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STAGE_SA,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_STATE_CLOSE,
            OHOS::IntellVoiceUtils::WAKEUP_SWITCH_REASON_POWER_SAVE_MODE);
        notifyPowerModeChange_ = false;
        HandleUnloadIntellVoiceService(true);
    }).detach();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnTriggerConnectServiceStart()
{
    INTELL_VOICE_LOG_INFO("enter");
    TaskExecutor::AddAsyncTask([this]() { SwitchOnProc(VOICE_WAKEUP_MODEL_UUID, false); },
        "IntellVoiceServiceManager::OnTriggerConnectServiceStart::start wakeup", false);
    TaskExecutor::AddAsyncTask([this]() { SwitchOnProc(PROXIMAL_WAKEUP_MODEL_UUID, false); },
        "IntellVoiceServiceManager::OnTriggerConnectServiceStart::start proximal", false);
}

template<typename T, typename E>
sptr<IIntellVoiceEngine> IntellVoiceServiceManager<T, E>::HandleCreateEngine(IntellVoiceEngineType type)
{
    return TaskExecutor::AddSyncTask([this, type]() -> sptr<IIntellVoiceEngine> {
            if (type == INTELL_VOICE_ENROLL) {
                StopDetection(VOICE_WAKEUP_MODEL_UUID);
                StopDetection(PROXIMAL_WAKEUP_MODEL_UUID);
            }
            return E::CreateEngine(type);
    });
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::HandleReleaseEngine(IntellVoiceEngineType type)
{
    return TaskExecutor::AddSyncTask([this, type]() -> int32_t {return ReleaseEngine(type); });
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::ReleaseEngine(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter, type:%{public}d", type);
    auto ret = E::ReleaseEngineInner(type);
    if (ret != 0) {
        return ret;
    }

    if (type != INTELL_VOICE_ENROLL && type != INTELL_VOICE_IMPLICIT_ENROLL) {
        return 0;
    }

    if (E::GetEnrollResult(type)) {
        if (IntellVoiceUtil::IsFileExist(WHISPER_VPR_CONFIG_PATH)) {
            HandleWhisperVprUpdate();
        } else {
            SwitchOnProc(VOICE_WAKEUP_MODEL_UUID, true);
            SwitchOnProc(PROXIMAL_WAKEUP_MODEL_UUID, true);
            SwitchOffProc(PROXIMAL_WAKEUP_MODEL_UUID);
        }
    } else {
        SwitchOnProc(VOICE_WAKEUP_MODEL_UUID, true);
        SwitchOffProc(VOICE_WAKEUP_MODEL_UUID);
        SwitchOnProc(PROXIMAL_WAKEUP_MODEL_UUID, true);
        SwitchOffProc(PROXIMAL_WAKEUP_MODEL_UUID);
        UnloadIntellVoiceService();
    }

    return 0;
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::ReleaseImplicitEnrollEngine()
{
    INTELL_VOICE_LOG_INFO("enter, release implicit enroll");
    TaskExecutor::AddAsyncTask([this]() { ReleaseEngine(INTELL_VOICE_IMPLICIT_ENROLL); });
    return 0;
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::SwitchOnProc(int32_t uuid, bool needUpdateAdapter)
{
    INTELL_VOICE_LOG_INFO("enter, uuid:%{public}d", uuid);
    if (E::AnyEngineExist({INTELL_VOICE_ENROLL, INTELL_VOICE_UPDATE})) {
        INTELL_VOICE_LOG_INFO("enroll engine or update engine exist, do nothing");
        return 0;
    }
    if (!QuerySwitchByUuid(uuid)) {
        INTELL_VOICE_LOG_INFO("switch is off, do nothing, uuid is %{public}d", uuid);
        return 0;
    }

    CreateAndStartServiceObject(uuid, needUpdateAdapter);
    INTELL_VOICE_LOG_INFO("exit");
    return 0;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::CreateAndStartServiceObject(int32_t uuid, bool needResetAdapter)
{
    if (uuid == PROXIMAL_WAKEUP_MODEL_UUID && !IntellVoiceUtil::IsFileExist(WHISPER_MODEL_PATH)) {
        INTELL_VOICE_LOG_INFO("no whisper model");
        IntellVoiceUtil::StartAbility("no_whisper");
        return;
    }

#ifdef TRIGGER_ENABLE
    if (!T::IsModelExist(uuid)) {
        INTELL_VOICE_LOG_INFO("no model");
        return;
    }
#endif

    if (!QuerySwitchByUuid(uuid)) {
        INTELL_VOICE_LOG_INFO("switch is off, uuid is %{public}d", uuid);
        return;
    }

#if defined(ENGINE_ENABLE) || defined(FIRST_STAGE_ONESHOT_ENABLE)
        INTELL_VOICE_LOG_INFO("is not single wakeup level");
        if (!needResetAdapter) {
            E::CreateEngine(INTELL_VOICE_WAKEUP);
        } else {
            E::CreateOrResetWakeupEngine();
        }
        T::CreateDetector(uuid, [this, uuid]() { OnDetected(uuid); });
#else
        T::CreateDetector(uuid, [this, uuid]() { OnSingleLevelDetected(); });
#endif

    if (uuid == VOICE_WAKEUP_MODEL_UUID) {
        SetShortWordStatus();
        SetDualMicStatus();
    }

    if (!StartDetection(uuid)) {
        INTELL_VOICE_LOG_ERROR("failed to start detection");
    }
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::SwitchOffProc(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, uuid:%{public}d", uuid);
    if (E::AnyEngineExist({INTELL_VOICE_ENROLL, INTELL_VOICE_UPDATE})) {
        INTELL_VOICE_LOG_INFO("enroll engine or update engine exist, do nothing");
        return 0;
    }
    if (QuerySwitchByUuid(uuid)) {
        INTELL_VOICE_LOG_INFO("switch is on, do nothing, uuid is %{public}d", uuid);
        return 0;
    }

    DelStartDetectionTask(uuid);
    ReleaseServiceObject(uuid);
    INTELL_VOICE_LOG_INFO("exit, uuid:%{public}d", uuid);
    return 0;
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::AddStartDetectionTask(int32_t uuid)
{
#ifdef USE_FFRT
    INTELL_VOICE_LOG_INFO("enter");
    if (taskQueue_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("task queue is nullptr");
        return false;
    }

    if ((taskHandle_.count(uuid) != 0) && (taskHandle_[uuid] != nullptr)) {
        INTELL_VOICE_LOG_INFO("task handle is exist, uuid is %{public}d", uuid);
        return true;
    }

    taskHandle_[uuid] = taskQueue_->submit_h(std::bind([uuid, this]() {
        if (!QuerySwitchByUuid(uuid)) {
            INTELL_VOICE_LOG_INFO("switch is off, uuid is %{public}d", uuid);
            return;
        }

        INTELL_VOICE_LOG_INFO("begin to wait");
        while (T::GetParameter("audio_hal_status") != "true") {
            ffrt::this_task::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL));
            if (!QuerySwitchByUuid(uuid)) {
                INTELL_VOICE_LOG_INFO("switch is off, uuid is %{public}d", uuid);
                return;
            }
        }
        INTELL_VOICE_LOG_INFO("end to wait");
        TaskExecutor::AddAsyncTask([uuid, this]() {
            if (isStarted_.count(uuid) != 0 && isStarted_[uuid]) {
                INTELL_VOICE_LOG_INFO("already start, uuid is %{public}d", uuid);
                return;
            }
            StartDetection(uuid);
        });
    }));
#endif
    return true;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::DelStartDetectionTask(int32_t uuid)
{
#ifdef USE_FFRT
    INTELL_VOICE_LOG_INFO("enter");
    if (taskQueue_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("task queue is nullptr");
        return;
    }

    if ((taskHandle_.count(uuid) != 0) && (taskHandle_[uuid] != nullptr)) {
        taskQueue_->cancel(taskHandle_[uuid]);
        taskHandle_.erase(uuid);
        INTELL_VOICE_LOG_INFO("task is canceled");
    }
#endif

    if (isStarted_.count(uuid)) {
        isStarted_.erase(uuid);
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ReleaseServiceObject(int32_t uuid)
{
    T::ReleaseTriggerDetector(uuid);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleSilenceUpdate()
{
    TaskExecutor::AddAsyncTask([this]() {
        int32_t ret = E::SilenceUpdate();
        if (ret != 0 && IntellVoiceUtil::IsFileExist(WHISPER_VPR_CONFIG_PATH)
            && HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WHISPER_VPR) == "false") {
            E::WhisperVprUpdate();
        }
    });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleWhisperVprUpdate()
{
    INTELL_VOICE_LOG_INFO("enter");
    TaskExecutor::AddAsyncTask([this]() {
        E::WhisperVprUpdate();
    });
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::HandleCloneUpdate(const std::string &wakeupInfo,
    const sptr<IRemoteObject> &object)
{
    return TaskExecutor::AddSyncTask([this, wakeupInfo, object = std::move(object)]() -> int32_t {
        if (serviceUnloading_) {
            return -1;
        }
        if (IntellVoiceUtil::IsImplicitEnroll(wakeupInfo)) {
            return E::ImplicitEnroll(wakeupInfo, object);
        }
        return E::CloneUpdate(wakeupInfo, object);
    });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::StopWakeupSource()
{
    INTELL_VOICE_LOG_INFO("enter, stop all recognition");
    TaskExecutor::AddAsyncTask([this]() {
        T::SetParameter(STOP_ALL_RECOGNITION, "true");
        }, "IntellVoiceServiceManager::StopWakeupSource", false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleCloseWakeupSource(bool isNeedStop)
{
    INTELL_VOICE_LOG_INFO("enter, isNeedStop:%{public}d", isNeedStop);
    TaskExecutor::AddAsyncTask([this, isNeedStop]() {
        if (isNeedStop) {
            T::SetParameter(STOP_ALL_RECOGNITION, "true");
        }
        StartDetection(VOICE_WAKEUP_MODEL_UUID);
        StartDetection(PROXIMAL_WAKEUP_MODEL_UUID);
        }, "IntellVoiceServiceManager::HandleCloseWakeupSource", false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleServiceStop()
{
    TaskExecutor::AddSyncTask([this]() -> int32_t {
        return E::ServiceStopProc();
    });
    if (IsSwitchError(WAKEUP_KEY)) {
        INTELL_VOICE_LOG_WARN("db is abnormal, can not find wakeup switch, notify db error");
        IntellVoiceUtil::StartAbility("db_error");
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleClearWakeupEngineCb()
{
    TaskExecutor::AddSyncTask([this]() -> int32_t {
        E::ClearWakeupEngineCb();
        return 0;
    });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleDeviceServiceDie(const std::string &serviceName)
{
    TaskExecutor::AddSyncTask([this, serviceName]() -> int32_t {
        E::DeviceServiceDie(serviceName);
        return 0;
    });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleSwitchOff(bool isAsync, int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, isAsync:%{public}d, uuid:%{public}d", isAsync, uuid);
    if (!isAsync) {
        TaskExecutor::AddSyncTask([this, uuid]() -> int32_t { return SwitchOffProc(uuid); });
    } else {
        TaskExecutor::AddAsyncTask([this, uuid]() { SwitchOffProc(uuid); });
    }
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleSwitchOn(bool isAsync, int32_t uuid, bool needUpdateAdapter)
{
    INTELL_VOICE_LOG_INFO("enter, isAsync:%{public}d, uuid:%{public}d, needUpdateAdapter:%{public}d",
        isAsync, uuid, needUpdateAdapter);
    if (isAsync) {
        TaskExecutor::AddAsyncTask([this, uuid, needUpdateAdapter]() { SwitchOnProc(uuid, needUpdateAdapter); });
    } else {
        TaskExecutor::AddSyncTask([this, uuid, needUpdateAdapter]() -> int32_t {
            return SwitchOnProc(uuid, needUpdateAdapter);
        });
    }
    INTELL_VOICE_LOG_INFO("exit");
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleUpdateComplete(int32_t result, const std::string &param)
{
    TaskExecutor::AddAsyncTask([this, result, param]() {
            bool isNeedUpdateComplete = E::IsNeedUpdateComplete(result, param);
            INTELL_VOICE_LOG_INFO("isNeedUpdateComplete:%{public}d", isNeedUpdateComplete);
            if (isNeedUpdateComplete && IntellVoiceUtil::IsFileExist(WHISPER_VPR_CONFIG_PATH)
                && param != "WhisperVprUpdate") {
                E::WhisperVprUpdate(true);
                return;
            }

            if (isNeedUpdateComplete) {
                SwitchOnProc(VOICE_WAKEUP_MODEL_UUID, true);
                SwitchOnProc(PROXIMAL_WAKEUP_MODEL_UUID, true);
                UnloadIntellVoiceService();
            }
        }, "IntellVoiceServiceManager::HandleUpdateComplete", false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleUpdateRetry()
{
    TaskExecutor::AddAsyncTask([this]() {
        if (E::IsNeedUpdateRetry()) {
            SwitchOnProc(VOICE_WAKEUP_MODEL_UUID, true);
            SwitchOnProc(PROXIMAL_WAKEUP_MODEL_UUID, true);
            UnloadIntellVoiceService();
        }
        }, "IntellVoiceServiceManager::HandleUpdateRetry", false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnDetected(int32_t uuid)
{
    TaskExecutor::AddAsyncTask([uuid, this]() {
        if (!E::IsEngineExist(INTELL_VOICE_WAKEUP)) {
            INTELL_VOICE_LOG_WARN("wakeup engine is nullptr");
            HandleCloseWakeupSource(true);
            return;
        }
        E::EngineOnDetected(uuid);
        }, "IntellVoiceServiceManager::OnDetected", false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnServiceStart(std::map<int32_t, std::function<void(bool)>> &saChangeFuncMap)
{
    saChangeFuncMap[TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID] = [this](bool isAdded) {
        T::OnTelephonyStateRegistryServiceChange(isAdded);
    };
#ifndef SUPPORT_RECORD_WAKEUP_CONCURRENT
    saChangeFuncMap[AUDIO_DISTRIBUTED_SERVICE_ID] = [this](bool isAdded) {
        T::OnAudioDistributedServiceChange(isAdded);
    };
#endif
    saChangeFuncMap[AUDIO_POLICY_SERVICE_ID] = [this](bool isAdded) {
        T::OnAudioPolicyServiceChange(isAdded);
        E::OnAudioPolicyServiceChange(isAdded);
    };
#ifdef POWER_MANAGER_ENABLE
    saChangeFuncMap[POWER_MANAGER_SERVICE_ID] = [this](bool isAdded) {
        T::OnPowerManagerServiceChange(isAdded);
    };
#endif
    saChangeFuncMap[DISPLAY_MANAGER_SERVICE_ID] = [this](bool isAdded) {
        T::OnDisplayManagerServiceChange(isAdded);
    };
    T::OnServiceStart();
    E::OnServiceStart();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnServiceStop()
{
    T::OnServiceStop();
    E::OnServiceStop();
}

/**
 * @brief 处理服务激活回调
 * @description 当服务从idle状态重新激活时调用，重置卸载标志
 */
template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnServiceActive()
{
    serviceUnloading_ = false;
}

template<typename T, typename E>
std::string IntellVoiceServiceManager<T, E>::TriggerGetParameter(const std::string &key)
{
    return T::GetParameter(key);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::TriggerSetParameter(const std::string &key, const std::string &value)
{
    return T::SetParameter(key, value);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::TriggerMgrUpdateModel(std::vector<uint8_t> buffer, int32_t uuid,
    TriggerModelType type)
{
    T::UpdateModel(buffer, uuid, type);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::RegisterProxyDeathRecipient(IntellVoiceEngineType type,
    const sptr<IRemoteObject> &object)
{
    return E::RegisterProxyDeathRecipient(type, object);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::DeregisterProxyDeathRecipient(IntellVoiceEngineType type)
{
    return E::DeregisterProxyDeathRecipient(type);
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::RegisterDeviceDeathRecipient(const std::string &serviceName,
    const sptr<IRemoteObject> &object)
{
    return E::RegisterDeviceDeathRecipient(serviceName, object);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files)
{
    int32_t ret = E::GetUploadFiles(numMax, files);
    ScheduleDelayedUnload();
    return ret;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ScheduleDelayedUnload()
{
    if (!IsNeedToUnloadService()) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(delayUnloadMutex_);
        lastCallTime_ = std::chrono::steady_clock::now();
        if (!delayUnloadWaiting_) {
            delayUnloadWaiting_ = true;
            std::thread([this]() { this->DelayedUnloadService(); }).detach();
        }
    }
    return;
}
template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::DelayedUnloadService()
{
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INTERVAL));

        std::unique_lock<std::mutex> lock(delayUnloadMutex_);

        if (!IsNeedToUnloadService()) {
            delayUnloadWaiting_ = false;
            return;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - lastCallTime_;

        if (elapsed < std::chrono::milliseconds(WAIT_SWITCH_TIME)) {
            continue;
        }

        delayUnloadWaiting_ = false;
        lock.unlock();

        INTELL_VOICE_LOG_WARN("timeout and switch is off, delay unloading service.");
        TaskExecutor::AddAsyncTask([this]() { UnloadIntellVoiceService(); });
        return;
    }
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::EngineSetParameter(const std::string &keyValueList)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::map<std::string, std::string> kvpairs;
    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);
    for (auto it : kvpairs) {
        auto handler = handlerMap_.find(it.first);
        if (handler != handlerMap_.end()) {
            return handler->second(it.first, it.second);
        }
        auto voidHandler = voidHandlerMap_.find(it.first);
        if (voidHandler != voidHandlerMap_.end()) {
            voidHandler->second(it.first, it.second);
        } else {
            INTELL_VOICE_LOG_INFO("no need to process, key:%{public}s", it.first.c_str());
        }
    }
    return 0;
}

template<typename T, typename E>
std::string IntellVoiceServiceManager<T, E>::EngineGetParameter(const std::string &key)
{
    return E::GetParameter(key);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles)
{
    return E::GetWakeupSourceFilesList(cloneFiles);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::GetWakeupSourceFile(const std::string &filePath,
    std::vector<uint8_t> &buffer)
{
    return E::GetWakeupSourceFile(filePath, buffer);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::SendWakeupFile(const std::string &filePath,
    const std::vector<uint8_t> &buffer)
{
    return E::SendWakeupFile(filePath, buffer);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::SetScreenOff(bool value)
{
    return E::SetScreenOff(value);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::OnSingleLevelDetected()
{
    INTELL_VOICE_LOG_INFO("single level detected");
    recordStart_ = -1;
    StopWakeupSource();
    IntellVoiceUtil::StartAbility("single_level_event");
    HandleRecordStartInfoChange();
    return;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ProcSingleLevelModel()
{
#ifdef ONLY_FIRST_STAGE
    INTELL_VOICE_LOG_INFO("enter");
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint32_t size = 0;
    std::string country = "";
    std::string model_path = "";
    country = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_COUNTRY_INFO_NAME);
    INTELL_VOICE_LOG_INFO("current country is %{public}s", country.c_str());

    if (country == "CN") {
        model_path = SINGLE_LEVEL_MODEL_CN_PATH;
    } else if (country == "AE") {
        model_path = SINGLE_LEVEL_MODEL_AE_PATH;
    } else if (country == "MY") {
        model_path = SINGLE_LEVEL_MODEL_MY_PATH;
    } else if (country == "MX") {
        model_path = SINGLE_LEVEL_MODEL_MX_PATH;
    } else if (country == "SA") {
        model_path = SINGLE_LEVEL_MODEL_SA_PATH;
    } else if (country == "GB") {
        model_path = SINGLE_LEVEL_MODEL_GB_PATH;
    } else if (country == "CL") {
        model_path = SINGLE_LEVEL_MODEL_CL_PATH;
    } else {
        /* According to the model system archive path, select the default model path */
        if (access(SINGLE_LEVEL_MODEL_CN_PATH.c_str(), F_OK) == 0) {
            model_path = SINGLE_LEVEL_MODEL_CN_PATH;
        } else {
            model_path = SINGLE_LEVEL_MODEL_GB_PATH;
        }
    }

    if (!IntellVoiceUtil::ReadFile(model_path, buffer, size)) {
        INTELL_VOICE_LOG_ERROR("read model failed!");
        return;
    }
    std::vector<uint8_t> data(buffer.get(), buffer.get() + size);
    T::UpdateModel(data, VOICE_WAKEUP_MODEL_UUID, TriggerModelType::VOICE_WAKEUP_TYPE);
#endif
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::ResetSingleLevelWakeup(const std::string &value)
{
    INTELL_VOICE_LOG_INFO("record_start:%{public}s", value.c_str());
    if (value != std::string("true") && value != std::string("false")) {
        return;
    }

    recordStart_ = (value == std::string("true")) ? 1 : 0;
    NoiftyRecordStartInfoChange();
    if (recordStart_ == 0) {
        HandleCloseWakeupSource(true);
    }
}

template<typename T, typename E>
bool IntellVoiceServiceManager<T, E>::HasReceviedRecordStartMsg()
{
    return (recordStart_ == 1 || recordStart_ == 0);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::NoiftyRecordStartInfoChange()
{
    {
        std::unique_lock<std::mutex> lock(recordStartInfoChangeMutex_);
        if (!notifyRecordStartInfoChange_) {
            INTELL_VOICE_LOG_INFO("no need to notify");
            return;
        }
    }

    recordStartInfoChangeCv_.notify_all();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleRecordStartInfoChange()
{
    if (HasReceviedRecordStartMsg()) {
        INTELL_VOICE_LOG_INFO("record start info has received, no need to process");
        return;
    }

    std::thread([&]() {
        std::unique_lock<std::mutex> lock(recordStartInfoChangeMutex_);
        if (HasReceviedRecordStartMsg()) {
            INTELL_VOICE_LOG_INFO("record start info has received, no need to process");
            return;
        }
        notifyRecordStartInfoChange_ = true;
        if (recordStartInfoChangeCv_.wait_for(lock, std::chrono::milliseconds(WAIT_RECORD_START_TIME),
            [this] { return HasReceviedRecordStartMsg(); })) {
            INTELL_VOICE_LOG_INFO("record start info has received, do nothing");
            notifyRecordStartInfoChange_ = false;
            return;
        }
        INTELL_VOICE_LOG_WARN("wait time out, need to reset wakeup");
        notifyRecordStartInfoChange_ = false;
        HandleCloseWakeupSource(true);
    }).detach();
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleSensibility(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set Sensibility:%{public}s", value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_SENSIBILITY, value);
    TaskExecutor::AddSyncTask([this, key, value]() -> int32_t {
        E::SetDspSensibility(value);
        return E::SetParameter(key, value);
    });
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleWakeupBundleName(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set wakeup bundle name:%{public}s", value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_ENGINE_BUNDLE_NAME, value);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleCountryInfo(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set country name:%{public}s", value.c_str());
    std::string oldCountry = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_COUNTRY_INFO_NAME);
    if (oldCountry == value) {
        INTELL_VOICE_LOG_INFO("country is same, no need to update");
        return;
    }
    HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_COUNTRY_INFO_NAME, value);
    /* When the country is set, the model is updated synchronously */
    ProcSingleLevelModel();
    HandleSwitchOn(true, VOICE_WAKEUP_MODEL_UUID, false);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleWakeupAbilityName(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set wakeup ability name:%{public}s", value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_ENGINE_ABILITY_NAME, value);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleRecordStart(const std::string &key, const std::string &value)
{
#ifdef ONLY_FIRST_STAGE
#ifndef FIRST_STAGE_ONESHOT_ENABLE
    ResetSingleLevelWakeup(value);
#else
    E::SetParameter(key, value);
#endif
#else
    (void)key;
    (void)value;
#endif
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleWakeupScene(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set wakeup scene %{public}s:%{public}s", key.c_str(), value.c_str());
    E::SetParameter(key, value);
    T::SetParameter(key, value);
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::HandleDataClear(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("clear data %{public}s:%{public}s", key.c_str(), value.c_str());
    return TaskExecutor::AddSyncTask([this, key, value]() -> int32_t {
        if (serviceUnloading_) {
            return -1;
        }
        return E::SetParameter(key, value);
    });
}

template<typename T, typename E>
int32_t IntellVoiceServiceManager<T, E>::HandleEngineRelease(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("release cmd %{public}s:%{public}s", key.c_str(), value.c_str());
    if (value == "implicit_enroll") {
        TaskExecutor::AddAsyncTask([this]() { ReleaseEngine(INTELL_VOICE_IMPLICIT_ENROLL); });
        INTELL_VOICE_LOG_INFO("implicit enroll release started");
        return 0;
    }
    return -1;
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleTriggerParam(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("set trigger param to db %{public}s:%{public}s", key.c_str(), value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair(key, value);
    TaskExecutor::AddAsyncTask([this, key, value]() {
        T::SetParameter(key, value);
    }, "IntellVoiceServiceManager::SetTriggerParams", false);
}

#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleConcurrentWhiteList(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("v:%{public}s", value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair("concurrentWhiteList", value);
    bool isWhiteList = true;
    bool isBlackList = false;
    T::ClearConcurrentControlList(isWhiteList, isBlackList);
}

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::HandleConcurrentBlackList(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("v:%{public}s", value.c_str());
    HistoryInfoMgr::GetInstance().SetStringKVPair("concurrentBlackList", value);
    bool isWhiteList = false;
    bool isBlackList = true;
    T::ClearConcurrentControlList(isWhiteList, isBlackList);
}
#endif

template<typename T, typename E>
void IntellVoiceServiceManager<T, E>::InitParameterHandlerMap()
{
    voidHandlerMap_ = {
        {"Sensibility", [this](const std::string &k, const std::string &v) { HandleSensibility(k, v); }},
        {"wakeup_bundle_name", [this](const std::string &k, const std::string &v) { HandleWakeupBundleName(k, v); }},
        {"wakeup_ability_name", [this](const std::string &k, const std::string &v) { HandleWakeupAbilityName(k, v); }},
        {"record_start", [this](const std::string &k, const std::string &v) { HandleRecordStart(k, v); }},
        {"country", [this](const std::string &k, const std::string &v) { HandleCountryInfo(k, v); }},
        {"wakeup_scene", [this](const std::string &k, const std::string &v) { HandleWakeupScene(k, v); }},
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
        {"concurrentWhiteList",
            [this](const std::string &k, const std::string &v) { HandleConcurrentWhiteList(k, v); }},
        {"concurrentBlackList",
            [this](const std::string &k, const std::string &v) { HandleConcurrentBlackList(k, v); }},
#endif
    };
    // add TRIGGER_PARAMS
    for (const auto &param : TRIGGER_PARAMS) {
        voidHandlerMap_[param] = [this](const std::string &k, const std::string &v) { HandleTriggerParam(k, v); };
    }

    handlerMap_ = {
        {"clear_data", [this](const std::string &k, const std::string &v) { return HandleDataClear(k, v); }},
        {"engine_release", [this](const std::string &k, const std::string &v) { return HandleEngineRelease(k, v); }},
    };
}

template class IntellVoiceServiceManager<TriggerManagerType, EngineManagerType>;
}  // namespace IntellVoiceEngine
} // namespace OHOS