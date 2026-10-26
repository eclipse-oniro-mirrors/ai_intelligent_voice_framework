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
#ifndef INTELL_VOICE_TRIGGER_HELPER_H
#define INTELL_VOICE_TRIGGER_HELPER_H

#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include "msg_handle_thread.h"
#include "trigger_base_type.h"
#include "i_intell_voice_trigger_recognition_callback.h"
#include "i_intell_voice_trigger_connector_module.h"
#include "i_intell_voice_trigger_connector_callback.h"
#include "trigger_connector_common_type.h"
#ifdef SUPPORT_TELEPHONY_SERVICE
#include "telephony_observer.h"
#endif
#include "audio_system_manager.h"
#include "audio_stream_manager.h"
#include "audio_info.h"
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"
#include "app_mgr_client.h"
#include "json/json.h"
#include "audio_engine_client_manager.h"
#include "audio_engine_callback_types.h"
#endif
#ifdef POWER_MANAGER_ENABLE
#include "sync_hibernate_callback_stub.h"
#include "sync_sleep_callback_stub.h"
#include "power_mgr_client.h"
#endif
#ifdef SUPPORT_WINDOW_MANAGER
#include "display_manager_lite.h"
#endif

namespace OHOS {
namespace IntellVoiceTrigger {
using OHOS::AudioStandard::AudioRendererStateChangeCallback;
using OHOS::AudioStandard::AudioRendererChangeInfo;
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
using OHOS::AudioStandard::AudioCapturerStateChangeCallback;
using OHOS::AudioStandard::AudioCapturerChangeInfo;
using OHOS::AudioStandard::SourceType;
using OHOS::AudioStandard::AudioInputPipeCallback;
using OHOS::AudioStandard::AudioPipeChangeType;
using OHOS::AudioStandard::CapturerStreamInfo;
using OHOS::AudioStandard::HdiAdapterType;
enum class concurrencyControlList {
    WHITELIST = 0,
    BLACKLIST,
};
#else 
using OHOS::AudioStandard::AudioCapturerSourceCallback;
#endif
using OHOS::AudioStandard::AudioManagerAudioSceneChangedCallback;
using OHOS::AudioStandard::AudioScene;
#ifdef SUPPORT_WINDOW_MANAGER
using FoldStatus = OHOS::Rosen::FoldStatus;
#endif

#ifdef SUPPORT_FAN_STATUS_LISTEN
// 风扇状态上报接口文档 https://wiki.huawei.com/domains/134531/wiki/378139/WIKI2026071111817819
constexpr uint32_t FAN_RES_ID = 4214;
constexpr uint64_t PERF_REPORT_LEN_MAX = 512;
constexpr uint64_t DATA_TYPE_POWER = 19;
constexpr uint64_t DATA_TYPE_POWER_VAL_ON = 1;
constexpr uint64_t DATA_TYPE_POWER_VAL_OFF = 0;

#pragma pack(push, 1)
struct FanStatus {
    uint32_t type;
    int32_t data;
};
#pragma pack(pop)
#endif

enum ModelState { MODEL_NOTLOADED, MODEL_LOADED, MODEL_STARTED, MODEL_STATE_BUT };

class TriggerModelData {
public:
    explicit TriggerModelData(int32_t uuid);
    ~TriggerModelData();
    void SetCallback(std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> GetCallback();
    void SetModel(std::shared_ptr<GenericTriggerModel> model);
    std::shared_ptr<GenericTriggerModel> GetModel();
    void SetState(ModelState state);
    ModelState GetState() const;
    void SetModelHandle(int32_t handle);
    int32_t GetModelHandle() const;
    void SetRequested(bool requested);
    bool GetRequested() const;

    bool SameModel(std::shared_ptr<GenericTriggerModel> model);
    void Clear();
    void ClearCallback();

public:
    int32_t uuid_ = -1;

private:
    ModelState state_ = MODEL_NOTLOADED;
    std::shared_ptr<GenericTriggerModel> model_ = nullptr;
    std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback_ = nullptr;
    int32_t modelHandle_ = 0;
    bool requested_ = false;
};

class TriggerHelper : public IIntellVoiceTriggerConnectorCallback, public std::enable_shared_from_this<TriggerHelper> {
public:
    ~TriggerHelper();

    static std::shared_ptr<TriggerHelper> Create();

    int32_t StartGenericRecognition(int32_t uuid, std::shared_ptr<GenericTriggerModel> model,
        std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    int32_t StopGenericRecognition(int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    void UnloadGenericTriggerModel(int32_t uuid);
    int32_t SetParameter(const std::string &key, const std::string &value);
    int32_t SetParameterForInputPipe(const std::string &key, const std::string &value);
    
    std::string GetParameter(const std::string &key);
    void AttachAudioRendererEventListener();
    void DetachAudioRendererEventListener();
    void AttachAudioSceneEventListener();
    void DetachAudioSceneEventListener();
#ifdef SUPPORT_FAN_STATUS_LISTEN
    void AttachFanStateListener();
    void DetachFanStateListener();
    void SyncFanState();
#endif
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    void ClearConcurrentControlList(bool isWhiteList, bool isBlackList);
    void ParseSourceTypes(const Json::Value& sources,
                            std::set<SourceType>& outSourceTypes,
                            std::set<SourceType>& globalTypeSet);
    void SetConcurrencyControlList(concurrencyControlList listType);
    std::string GetBundleName(int uid);
    void AttachAudioEngineListener();
    void DetachAudioEngineListener();
#else
    void AttachAudioCaptureListener();
    void DetachAudioCaptureListener();
#endif
#ifdef SUPPORT_TELEPHONY_SERVICE
    void AttachTelephonyObserver();
    void DetachTelephonyObserver();
#endif
#ifdef POWER_MANAGER_ENABLE
    void AttachHibernateObserver();
    void DetachHibernateObserver();
#endif

private:
    TriggerHelper();
    bool GetModule();
    std::shared_ptr<TriggerModelData> GetTriggerModelData(int32_t uuid);
    std::shared_ptr<TriggerModelData> CreateTriggerModelData(int32_t uuid);
    int32_t InitRecognition(std::shared_ptr<TriggerModelData> modelData, bool unload);
    int32_t PrepareForRecognition(std::shared_ptr<TriggerModelData> modelData);
    int32_t StartRecognition(std::shared_ptr<TriggerModelData> modelData);
    int32_t StopRecognition(std::shared_ptr<TriggerModelData> modelData);
    int32_t LoadModel(std::shared_ptr<TriggerModelData> modelData);
    int32_t UnloadModel(std::shared_ptr<TriggerModelData> modelData);
#ifdef ONLY_FIRST_STAGE
    void HandleWakeupInputGain();
#endif
    void OnUpdateAllRecognitionState();
    bool IsConflictSceneActive();

    void OnRecognition(int32_t modelHandle, const IntellVoiceRecognitionEvent &event) override;

    void OnUpdateRendererState(int32_t streamUsage, bool isPlaying);
#ifndef SUPPORT_RECORD_WAKEUP_CONCURRENT
    void OnCapturerStateChange(bool isActive);
#endif
#ifdef POWER_MANAGER_ENABLE
    void OnHibernateStateUpdated(bool isHibernate);
#endif
    void OnAudioSceneChange(const AudioScene audioScene);
#ifdef SUPPORT_FAN_STATUS_LISTEN
    void HandleFanStateChanged(uint32_t resId, const char* buffer, uint64_t len);
#endif
#ifdef SUPPORT_TELEPHONY_SERVICE
    void OnCallStateUpdated(int32_t callState);
    class TelephonyStateObserver : public Telephony::TelephonyObserver {
    public:
        explicit TelephonyStateObserver(const std::shared_ptr<TriggerHelper> helper) : helper_(helper)
        {}
        ~TelephonyStateObserver()
        {
            helper_ = nullptr;
        }
        void OnCallStateUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber) override;

    public:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };

private:
    sptr<TelephonyStateObserver> telephonyObserver0_ = nullptr;
#endif

#ifdef SUPPORT_WINDOW_MANAGER
public:
    void AttachFoldStatusListener();
    void DetachFoldStatusListener();

private:
    void UpdateGenericTriggerModel(std::shared_ptr<GenericTriggerModel> model);
    std::shared_ptr<GenericTriggerModel> ReadWhisperModel();
    void ReLoadWhisperModel(std::shared_ptr<TriggerModelData> modelData);
    void FoldStatusOperation(std::shared_ptr<TriggerModelData> modelData);
    void SetFoldStatus();
    void RegisterFoldStatusListener();
    void StartAllRecognition();
    void StopAllRecognition();
    void RestartAllRecognition();
    void OnFoldStatusChanged(FoldStatus foldStatus);
    std::string GetFoldStatusInfo();
    bool GetParameterInner(const std::string &key, std::string &value);
class FoldStatusListener : public OHOS::Rosen::DisplayManagerLite::IFoldStatusListener {
public:
    explicit FoldStatusListener(const std::shared_ptr<TriggerHelper> helper) : helper_(helper)
    {}

    ~FoldStatusListener()
    {
        helper_ = nullptr;
    }
    void OnFoldStatusChanged(FoldStatus foldStatus) override;

    public:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
};
private:
    sptr<OHOS::Rosen::DisplayManagerLite::IFoldStatusListener> foldStatusListener_;
    FoldStatus curFoldStatus_ = FoldStatus::UNKNOWN;
    bool isFoldStatusDetached_ = false;
    bool isFoldable_ = false;
    std::mutex foldStatusMutex_;
#endif

    class AudioSceneChangeCallback : public AudioManagerAudioSceneChangedCallback {
    public:
        explicit AudioSceneChangeCallback(const std::shared_ptr<TriggerHelper> helper) : helper_(helper)
        {}
        ~AudioSceneChangeCallback()
        {
            helper_ = nullptr;
        }
        void OnAudioSceneChange(const AudioScene audioScene) override;

    public:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };

#ifdef SUPPORT_FAN_STATUS_LISTEN
    static int32_t OnFanStateChanged(uint32_t resId, const char* buffer, uint64_t len);
#endif

#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    class AudioInputPipeListener : public AudioInputPipeCallback {
    public:
        explicit AudioInputPipeListener(const std::shared_ptr<TriggerHelper> helper)
            : helper_(helper)
        {}
        ~AudioInputPipeListener()
        {
            helper_ = nullptr;
        }
        void OnInputPipeChange(AudioPipeChangeType changeType,
            const std::shared_ptr<AudioInputPipeInfo> &changedPipeInfo) override;

    private:
        void GetConcurrentState(bool &isConcurrent,
                                std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>>& inputPipeStateMap);
        bool GetWhisperEnable(std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>>& inputPipeStateMap);
        void GetPipeInfo(std::map<uint32_t, std::map<uint32_t, CapturerStreamInfo>>& inputPipeinfo);
        bool CheckCallConflict();
        bool CheckCallOnly();
        bool CheckBlacklistConflict(const std::string &bundleName, 
            const CapturerStreamInfo &streamInfo);
        bool CheckInWhitelist(const std::string &bundleName, const CapturerStreamInfo &streamInfo);
        bool CheckSourceConcurrency(const CapturerStreamInfo &streamInfo);
        bool CheckVoipOnly(const CapturerStreamInfo &streamInfo);
        bool CheckSourceWhisperEnable(const CapturerStreamInfo &streamInfo);
        // two bool to one int
        int stateToInt(bool concurrentState, bool sourceState) {
            return (concurrentState ? 2 : 0) + (sourceState ? 1 : 0);
        }
        void ExecuteStateTransition(int oldState, int newState, const std::shared_ptr<TriggerModelData> &modelData);
        enum class captureMethod {
            SAME_STATE = 0,
            DO_CONCURRENT_CLOSE_SOURCE,
            DO_CONCURRENT_OPEN_SOURCE,
            DO_NO_CONCURRENT_STOP_SOURCE,
            DO_NO_CONCURRENT_OPEN_SOURCE,
            // clear concurrent flag and do old capture process
            DO_CONCURRENT_STOP_AND_NO_CONCURRENT_OPEN_SOURCE,
            // start wakeup and set concurrent flag
            DO_NO_CONCURRENT_STOP_AND_CONCURRENT_OPEN_SOURCE
        };

        static constexpr captureMethod methodTable[4][4] {
            {
                captureMethod::SAME_STATE,
                captureMethod::DO_NO_CONCURRENT_OPEN_SOURCE,
                captureMethod::DO_NO_CONCURRENT_STOP_SOURCE,
                captureMethod::DO_NO_CONCURRENT_STOP_AND_CONCURRENT_OPEN_SOURCE
            },
            {
                captureMethod::DO_NO_CONCURRENT_STOP_SOURCE,
                captureMethod::SAME_STATE,
                captureMethod::DO_NO_CONCURRENT_STOP_SOURCE,
                captureMethod::DO_NO_CONCURRENT_STOP_AND_CONCURRENT_OPEN_SOURCE
            },
            {
                captureMethod::DO_NO_CONCURRENT_OPEN_SOURCE,
                captureMethod::DO_NO_CONCURRENT_OPEN_SOURCE,
                captureMethod::SAME_STATE,
                captureMethod::DO_CONCURRENT_OPEN_SOURCE
            },
            {
                captureMethod::DO_CONCURRENT_STOP_AND_NO_CONCURRENT_OPEN_SOURCE,
                captureMethod::DO_CONCURRENT_STOP_AND_NO_CONCURRENT_OPEN_SOURCE,
                captureMethod::DO_CONCURRENT_CLOSE_SOURCE,
                captureMethod::SAME_STATE
            }
        };
        std::mutex mutex_;
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };
#else 
    class AudioCapturerSourceChangeCallback : public AudioCapturerSourceCallback {
    public:
        explicit AudioCapturerSourceChangeCallback(const std::shared_ptr<TriggerHelper> helper) : helper_(helper)
        {}
        ~AudioCapturerSourceChangeCallback()
        {
            helper_ = nullptr;
        }
        void OnCapturerState(bool isActive) override;

    public:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };
#endif

    class AudioRendererStateChangeCallbackImpl : public AudioRendererStateChangeCallback {
    public:
        explicit AudioRendererStateChangeCallbackImpl(const std::shared_ptr<TriggerHelper> helper)
            : helper_(helper)
        {}
        ~AudioRendererStateChangeCallbackImpl()
        {
            helper_ = nullptr;
            rendererStateMap_.clear();
        }

        void OnRendererStateChange(
           const std::vector<std::shared_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;
    private:
        std::mutex mutex_;
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
        std::map<int32_t, bool> rendererStateMap_;
    };

#ifdef POWER_MANAGER_ENABLE
    class HibernateCallback : public PowerMgr::SyncHibernateCallbackStub {
    public:
        explicit HibernateCallback(const std::shared_ptr<TriggerHelper> helper)
            : helper_(helper)
        {}
        ~HibernateCallback()
        {
            helper_ = nullptr;
        }

        void OnSyncHibernate() override;
        void OnSyncWakeup(bool hibernateResult = false) override;

    private:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };

    class SleepCallback : public PowerMgr::SyncSleepCallbackStub {
    public:
        explicit SleepCallback(const std::shared_ptr<TriggerHelper> helper)
            : helper_(helper)
        {}
        ~SleepCallback()
        {
            helper_ = nullptr;
        }

        void OnSyncSleep(bool onForceSleep);
        void OnSyncWakeup(bool onForceSleep);

    private:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };
#endif

private:
    std::mutex mutex_;
    std::mutex telephonyMutex_;
    std::mutex rendererMutex_;
    std::mutex playMutex_;
    bool hasStreamBesidesVoip_ = false;
    bool hasStreamBesidesVoice_ = false;
#ifdef SUPPORT_RECORD_WAKEUP_CONCURRENT
    bool mFirstProc = false;
    bool mExtensionConcurrency_ = false;
    bool mVoipConcurrency_ = false;
    bool mCallConcurrency_ = false;
    std::mutex capturerMutex_;
    std::mutex whitelistMutex_;
    std::mutex blacklistMutex_;
    std::shared_ptr<AudioInputPipeCallback> audioInputPipeCallback_ = nullptr;
    bool audioConcurrentCapture_ = true;
    bool whitelistWhisperEnbale_ = true;
    std::map<std::string, std::set<SourceType>> concurrentWhitelist_{};
    std::map<std::string, std::set<SourceType>> concurrentBlacklist_{};
    bool isCapturerDetached_ = false;
    bool mCameraConcurrency_ = false;
    std::set<SourceType> concurrentTypeSet_{};
    std::set<SourceType> ExtensionConcurrentTypeSet_{
        SourceType::SOURCE_TYPE_MIC, 
        SourceType::SOURCE_TYPE_VOICE_RECOGNITION,
        SourceType::SOURCE_TYPE_ULTRASONIC, 
        SourceType::SOURCE_TYPE_VOICE_TRANSCRIPTION,
        SourceType::SOURCE_TYPE_CAMCORDER,
        SourceType::SOURCE_TYPE_UNPROCESSED};
#else
    std::shared_ptr<AudioCapturerSourceChangeCallback> audioCapturerSourceChangeCallback_ = nullptr;
#endif

#ifdef POWER_MANAGER_ENABLE
    std::mutex hiberateMutex_;
#endif
    std::mutex sceneMutex_;

    std::map<int32_t, std::shared_ptr<TriggerModelData>> modelDataMap_;
    std::shared_ptr<IIntellVoiceTriggerConnectorModule> module_ = nullptr;
    std::vector<TriggerConnectorModuleDesc> moduleDesc_;
    std::shared_ptr<AudioRendererStateChangeCallbackImpl> audioRendererStateChangeCallback_ = nullptr;
    std::shared_ptr<AudioSceneChangeCallback> audioSceneChangeCallback_ = nullptr;
    bool callActive_ = false;
    bool systemHibernate_ = false;
    bool audioCaptureActive_ = false;
    AudioScene audioScene_ = AudioScene::AUDIO_SCENE_DEFAULT;

#ifdef SUPPORT_TELEPHONY_SERVICE
    bool isTelephonyDetached_ = false;
#endif
    bool isRendererDetached_ = false;
#ifdef POWER_MANAGER_ENABLE
    sptr<HibernateCallback> hibernateCallback_ = nullptr;
    sptr<SleepCallback> sleepCallback_ = nullptr;
    bool isHibernateDetached_ = false;
#endif
    bool isSceneDetached_ = false;
#ifdef SUPPORT_FAN_STATUS_LISTEN
    std::mutex fanStateMutex_;
    void *perfGeniusHandle_ = nullptr;
    bool fanRunning_ = false;
    bool isFanListenerDetached_ = false;
#endif
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif