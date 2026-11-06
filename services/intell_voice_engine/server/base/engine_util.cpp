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
#include "engine_util.h"
#include <ashmem.h>
#include "intell_voice_log.h"
#include "string_util.h"
#include "scope_guard.h"
#include "engine_host_manager.h"
#include "engine_callback_message.h"
#include "intell_voice_engine_manager.h"
#include "history_info_mgr.h"
#include "intell_voice_definitions.h"
#include "intell_voice_sensibility.h"

#define LOG_TAG "EngineUtils"

using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::AudioStandard;

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string LANGUAGE_TEXT = "language=";
static const std::string AREA_TEXT = "area=";
static const int32_t INTELL_VOICE_SERVICE_UID = 1042;

EngineUtil::EngineUtil()
{
    desc_.adapterType = ADAPTER_TYPE_BUT;
}

int32_t EngineUtil::SetParameter(const std::string &keyValueList)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }
    return adapter_->SetParameter(keyValueList);
}

std::string EngineUtil::GetParameter(const std::string &key)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return "";
    }

    std::string value;
    adapter_->GetParameter(key, value);
    return value;
}

int32_t EngineUtil::WriteAudio(const uint8_t *buffer, uint32_t size)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    if (buffer == nullptr || size == 0) {
        INTELL_VOICE_LOG_ERROR("buffer is invalid, size:%{public}u", size);
        return -1;
    }

    std::vector<uint8_t> audioBuff(&buffer[0], &buffer[size]);
    return adapter_->WriteAudio(audioBuff);
}

int32_t EngineUtil::Stop()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }
    return adapter_->Stop();
}

int32_t EngineUtil::GetWakeupPcm(std::vector<uint8_t> &data)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }
    return adapter_->GetWakeupPcm(data);
}

int32_t EngineUtil::Evaluate(const std::string &word, EvaluationResultInfo &info)
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return -1;
    }

    return adapter_->Evaluate(word, info);
}

bool EngineUtil::SetDspFeatures()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return false;
    }

    auto ret = EngineCallbackMessage::CallFunc(TRIGGERMGR_GET_PARAMETER, KEY_GET_WAKEUP_FEATURE);
    std::string features = "";
    if (ret.has_value()) {
        try {
            features = std::any_cast<std::string>(*ret);
        } catch (const std::bad_any_cast&) {
            INTELL_VOICE_LOG_ERROR("msg bus bad any cast");
            return false;
        }
    } else {
        INTELL_VOICE_LOG_ERROR("msg bus return no value");
        return false;
    }

    if (features == "") {
        INTELL_VOICE_LOG_WARN("failed to get wakeup dsp feature");
        features = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WAKEUP_DSP_FEATURE);
        if (features == "") {
            INTELL_VOICE_LOG_WARN("no historical wakeup dsp feature");
            return false;
        }
    } else {
        HistoryInfoMgr::GetInstance().SetStringKVPair(KEY_WAKEUP_DSP_FEATURE, features);
    }

    std::string kvPair = KEY_GET_WAKEUP_FEATURE + "=" + features;
    adapter_->SetParameter(kvPair);
    return true;
}

void EngineUtil::WriteBufferFromAshmem(uint8_t *&buffer, uint32_t size, sptr<OHOS::Ashmem> ashmem)
{
    if (!ashmem->MapReadOnlyAshmem()) {
        INTELL_VOICE_LOG_ERROR("map ashmem failed");
        return;
    }

    const uint8_t *tmpBuffer = static_cast<const uint8_t *>(ashmem->ReadFromAshmem(size, 0));
    if (tmpBuffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("read from ashmem failed");
        return;
    }

    buffer = new (std::nothrow) uint8_t[size];
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("allocate buffer failed");
        return;
    }

    if (memcpy_s(buffer, size, tmpBuffer, size) != 0) {
        INTELL_VOICE_LOG_ERROR("memcpy_s failed");
        return;
    }
}

std::vector<uint8_t> EngineUtil::ReadDspModel(
    OHOS::HDI::IntelligentVoice::Engine::V1_0::ContentType type)
{
    INTELL_VOICE_LOG_INFO("enter");
    uint8_t *buffer = nullptr;
    uint32_t size = 0;
    sptr<Ashmem> ashmem = nullptr;
    std::vector<uint8_t> ret;

    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return ret;
    }
    adapter_->Read(type, ashmem);
    if (ashmem == nullptr) {
        INTELL_VOICE_LOG_ERROR("ashmem is nullptr");
        return ret;
    }

    ON_SCOPE_EXIT_WITH_NAME(ashmemExit)
    {
        INTELL_VOICE_LOG_DEBUG("close ashmem");
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
    };

    size = static_cast<uint32_t>(ashmem->GetAshmemSize());
    if (size == 0) {
        INTELL_VOICE_LOG_ERROR("size is zero");
        return ret;
    }

    WriteBufferFromAshmem(buffer, size, ashmem);
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is nullptr");
        return ret;
    }

    ON_SCOPE_EXIT_WITH_NAME(bufferExit)
    {
        INTELL_VOICE_LOG_DEBUG("now delete buffer");
        delete[] buffer;
        buffer = nullptr;
    };
    ret.insert(ret.end(), buffer, buffer + size);
    return ret;
}

void EngineUtil::ProcDspModel(const std::vector<uint8_t> &buffer)
{
    if (buffer.empty()) {
        INTELL_VOICE_LOG_ERROR("vector is empty");
        return;
    }
    EngineCallbackMessage::CallFunc(TRIGGERMGR_UPDATE_MODEL, buffer, VOICE_WAKEUP_MODEL_UUID,
        IntellVoiceTrigger::TriggerModelType::VOICE_WAKEUP_TYPE);
}

void EngineUtil::SetLanguage()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }

    std::string language = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_LANGUAGE);
    if (language.empty()) {
        INTELL_VOICE_LOG_WARN("language is empty");
        return;
    }
    adapter_->SetParameter(LANGUAGE_TEXT + language);
}

void EngineUtil::SetWhisperVpr()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }
    adapter_->SetParameter("WhisperVpr=" + HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WHISPER_VPR));
}

void EngineUtil::SetArea()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }

    std::string area = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_AREA);
    if (area.empty()) {
        INTELL_VOICE_LOG_WARN("area is empty");
        return;
    }

    adapter_->SetParameter(AREA_TEXT + area);
}

void EngineUtil::SetSensibility()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }

    std::string sensibility = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_SENSIBILITY);
    if (sensibility.empty()) {
        INTELL_VOICE_LOG_WARN("sensibility is empty");
        return;
    }

    auto ret = EngineCallbackMessage::CallFunc(TRIGGERMGR_GET_PARAMETER, KEY_GET_WAKEUP_FEATURE);
    std::string features = "";
    if (ret.has_value()) {
        try {
            features = std::any_cast<std::string>(*ret);
        } catch (const std::bad_any_cast&) {
            INTELL_VOICE_LOG_ERROR("msg bus bad any cast");
            return;
        }
    } else {
        INTELL_VOICE_LOG_ERROR("msg bus return no value");
        return;
    }
    auto value = IntellVoiceSensibility::GetDspSensibility(sensibility, features, WAKEUP_CONFIG_PATH);
    if (value.empty()) {
        INTELL_VOICE_LOG_ERROR("no sensibility value");
        return;
    }
    EngineCallbackMessage::CallFunc(TRIGGERMGR_SET_PARAMETER, WAKEUP_SENSIBILITY, value);
    adapter_->SetParameter(SENSIBILITY_TEXT + sensibility);
}

void EngineUtil::SetTriggerParameter()
{
    for (const auto& param : TRIGGER_PARAMS) {
        std::string value = HistoryInfoMgr::GetInstance().GetStringKVPair(param);
        if (!value.empty()) {
            EngineCallbackMessage::CallFunc(TRIGGERMGR_SET_PARAMETER, param, value);
            INTELL_VOICE_LOG_INFO("set %{public}s:%{public}s", param.c_str(), value.c_str());
        }
    }
}

void EngineUtil::SelectInputDevice(DeviceType type)
{
    INTELL_VOICE_LOG_INFO("enter");
    sptr<AudioCapturerFilter> audioCapturerFilter = new (std::nothrow) AudioCapturerFilter();
    if (audioCapturerFilter == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioCapturerFilter is nullptr");
        return;
    }
    audioCapturerFilter->uid = INTELL_VOICE_SERVICE_UID;
    audioCapturerFilter->capturerInfo.sourceType = SourceType::SOURCE_TYPE_VOICE_RECOGNITION;
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> deviceDescriptorVector;
    auto audioSystemManager = AudioSystemManager::GetInstance();
    if (audioSystemManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("audioSystemManager is nullptr");
        return;
    }
    auto audioDeviceDescriptors = audioSystemManager->GetDevices(DeviceFlag::INPUT_DEVICES_FLAG);
    for (int i = 0; i < audioDeviceDescriptors.size(); i++) {
        if (audioDeviceDescriptors[i]->deviceType_ == type) {
            deviceDescriptorVector.push_back(audioDeviceDescriptors[i]);
        }
    }
    audioSystemManager->SelectInputDevice(audioCapturerFilter, deviceDescriptorVector);
}

void EngineUtil::SetScreenStatus()
{
    if (IntellVoiceEngineManager::GetScreenOff()) {
        SetParameter("screenoff=true");
    } else {
        SetParameter("screenoff=false");
    }
}

void EngineUtil::SetImproveParam()
{
    if (adapter_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("adapter is nullptr");
        return;
    }

    adapter_->SetParameter("userImproveOn=true");
}
}
}
