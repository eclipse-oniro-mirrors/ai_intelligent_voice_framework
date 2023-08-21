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
#include "msg_handle_thread.h"
#include "trigger_base_type.h"
#include "i_intell_voice_trigger_recognition_callback.h"

#include "i_intell_voice_trigger_connector_module.h"
#include "i_intell_voice_trigger_connector_callback.h"
#include "trigger_connector_common_type.h"

#include "telephony_observer.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace IntellVoiceTrigger {
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

    bool SameModel(std::shared_ptr<GenericTriggerModel> model);
    void Clear();

public:
    int32_t uuid_ = -1;

private:
    ModelState state_ = MODEL_NOTLOADED;
    std::shared_ptr<GenericTriggerModel> model_ = nullptr;
    std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback_ = nullptr;
    int32_t modelHandle_ = 0;
};

class TriggerHelper : public IIntellVoiceTriggerConnectorCallback,
    public std::enable_shared_from_this<TriggerHelper> {
public:
    ~TriggerHelper();

    static std::shared_ptr<TriggerHelper> Create();

    int32_t StartGenericRecognition(int32_t uuid, std::shared_ptr<GenericTriggerModel> model,
        std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    int32_t StopGenericRecognition(int32_t uuid, std::shared_ptr<IIntellVoiceTriggerRecognitionCallback> callback);
    void UnloadGenericTriggerModel(int32_t uuid);
    std::shared_ptr<TriggerModelData> GetTriggerModelData(int32_t uuid);
    void AttachTelephonyObserver();
    void DettachTelephonyObserver();
    void AttachAudioCaptureListener();
    void DettachAudioCaptureListener();

private:
    TriggerHelper();
    void GetModule();
    int32_t InitRecognition(std::shared_ptr<TriggerModelData> modelData, bool unload);
    int32_t StartRecognition(std::shared_ptr<TriggerModelData> modelData);
    int32_t StopRecognition(std::shared_ptr<TriggerModelData> modelData);
    int32_t LoadModel(std::shared_ptr<TriggerModelData> modelData);
    int32_t UnloadModel(std::shared_ptr<TriggerModelData> modelData);
    void OnUpdateAllRecognitionState();
    bool IsConflictSceneActive();

    void OnRecognition(int32_t modelHandle, const IntellVoiceRecognitionEvent &event) override;

private:
    class TelephonyStateObserver : public Telephony::TelephonyObserver {
    public:
        explicit TelephonyStateObserver(const std::shared_ptr<TriggerHelper> helper)
        {
            helper_ = helper;
        }
        ~TelephonyStateObserver()
        {
            helper_ = nullptr;
        }
        void OnCallStateUpdated(int32_t slotId, int32_t callState, const std::u16string &phoneNumber) override;

    public:
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
        bool callActive_ = false;
    };

private:
    class AudioCapturerSourceChangeCallback : public OHOS::AudioStandard::AudioCapturerSourceCallback {
    public:
        explicit AudioCapturerSourceChangeCallback(const std::shared_ptr<TriggerHelper> helper)
        {
            helper_ = helper;
        }
        ~AudioCapturerSourceChangeCallback()
        {
            helper_ = nullptr;
        }
        void OnCapturerState(bool isActive) override;

    public:
        bool audioCaptureActive_ = false;
        std::shared_ptr<TriggerHelper> helper_ = nullptr;
    };

private:
    std::mutex mutex_;

    std::map<int32_t, std::shared_ptr<TriggerModelData>> modelDataMap_;
    std::shared_ptr<IIntellVoiceTriggerConnectorModule> module_ = nullptr;
    std::vector<TriggerConnectorModuleDesc> moduleDesc_;
    sptr<TelephonyStateObserver> telephonyObserver0_ = nullptr;
    sptr<TelephonyStateObserver> telephonyObserver1_ = nullptr;
    std::shared_ptr<AudioCapturerSourceChangeCallback> audioCapturerSourceChangeCallback_ = nullptr;
};
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
#endif