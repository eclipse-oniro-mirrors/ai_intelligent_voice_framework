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
#include "wakeup_engine.h"
#include "idevmgr_hdi.h"
#include "intell_voice_engine_manager.h"
#include "intell_voice_log.h"
#include "intell_voice_definitions.h"
#include "headset_host_manager.h"
#include "headset_wakeup_wrapper.h"
#include "engine_callback_message.h"
#include "intell_voice_util.h"
#include "ability_manager_client.h"
#include "history_info_mgr.h"
#include "wakeup_track.h"

#define LOG_TAG "WakeupEngine"

using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;
using OHOS::HDI::DeviceManager::V1_0::IDeviceManager;

namespace OHOS {
namespace IntellVoiceEngine {
WakeupEngine::WakeupEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

WakeupEngine::~WakeupEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

void WakeupEngine::OnDetected(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, uuid is %{public}d", uuid);
    if (uuid == VOICE_WAKEUP_MODEL_UUID) {
        WakeupTrack::reportWakeupStart(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_WAKEUP_DETECTED,
            OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    } else if (uuid == PROXIMAL_WAKEUP_MODEL_UUID) {
        WakeupTrack::reportWakeupStart(OHOS::IntellVoiceUtils::WAKEUP_STAGE_SA_WHISPER_DETECTED,
            OHOS::IntellVoiceUtils::WAKEUP_RESULT_DEFAULT, OHOS::IntellVoiceUtils::WAKEUP_ERROR_CODE_END);
    }

    ROLE(WakeupEngineImplFacade).OnTriggerDetected(uuid, DEFAULT_MAC_ADDRESS, { VOICE_RECOGNIZE_EVENT, 0 });
}

bool WakeupEngine::Init(const std::string & /* param */, bool reEnroll)
{
    return ROLE(WakeupEngineImplFacade).Init();
}

void WakeupEngine::SetCallback(sptr<IRemoteObject> object)
{
    sptr<IIntelligentVoiceEngineCallback> callback = iface_cast<IIntelligentVoiceEngineCallback>(object);
    if (callback == nullptr) {
        INTELL_VOICE_LOG_WARN("clear callback");
    }
    ROLE(WakeupEngineImplFacade).SaveCallback(callback);
}

int32_t WakeupEngine::Attach(const IntellVoiceEngineInfo & /* info */)
{
    return 0;
}

int32_t WakeupEngine::StartCapturer(int32_t channels)
{
    StateMsg msg(START_CAPTURER, &channels, sizeof(int32_t));
    return ROLE(WakeupEngineImplFacade).Handle(msg);
}

int32_t WakeupEngine::Read(std::vector<uint8_t> &data)
{
    CapturerData capturerData;
    StateMsg msg(READ, nullptr, 0, reinterpret_cast<void *>(&capturerData));
    int32_t ret = ROLE(WakeupEngineImplFacade).Handle(msg);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("read failed, ret:%{public}d", ret);
        return -1;
    }

    data.swap(capturerData.data);
    return 0;
}

int32_t WakeupEngine::StopCapturer()
{
    StateMsg msg(STOP_CAPTURER);
    return ROLE(WakeupEngineImplFacade).Handle(msg);
}

int32_t WakeupEngine::NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event)
{
    INTELL_VOICE_LOG_INFO("enter, event:%{public}d", event.wakeupEvent);
    return ROLE(WakeupEngineImplFacade).OnTriggerDetected(VOICE_WAKEUP_MODEL_UUID, macAddress, event);
}

int32_t WakeupEngine::Detach(void)
{
    StateMsg msg(RELEASE);
    return ROLE(WakeupEngineImplFacade).Handle(msg);
}

int32_t WakeupEngine::Start(bool /* isLast */)
{
    return 0;
}

int32_t WakeupEngine::SetParameter(const std::string &keyValueList)
{
    StringParam param(keyValueList);
    StateMsg msg(SET_PARAM, &param, sizeof(param));
    return ROLE(WakeupEngineImplFacade).Handle(msg);
}

std::string WakeupEngine::GetParameter(const std::string &key)
{
    StringParam keyParam(key);
    StringParam valueParam;
    StateMsg msg(GET_PARAM, &keyParam, sizeof(keyParam), &valueParam);
    if (ROLE(WakeupEngineImplFacade).Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get parameter, key:%{public}s", key.c_str());
        return "";
    }

    return valueParam.strParam;
}

int32_t WakeupEngine::Stop()
{
    StateMsg msg(STOP_RECOGNIZE);
    return ROLE(WakeupEngineImplFacade).Handle(msg);
}

int32_t WakeupEngine::GetWakeupPcm(std::vector<uint8_t> &data)
{
    CapturerData capturerData;
    StateMsg msg(GET_WAKEUP_PCM, nullptr, 0, reinterpret_cast<void *>(&capturerData));
    int32_t ret = ROLE(WakeupEngineImplFacade).Handle(msg);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get wakeup pcm failed, ret:%{public}d", ret);
        return -1;
    }

    data.swap(capturerData.data);
    return 0;
}

bool WakeupEngine::ResetAdapter()
{
    StateMsg msg(RESET_ADAPTER);
    return (ROLE(WakeupEngineImplFacade).Handle(msg) == 0 ? true : false);
}

void WakeupEngine::ReleaseAdapter()
{
    StateMsg msg(RELEASE_ADAPTER);
    ROLE(WakeupEngineImplFacade).Handle(msg);
}
}  // namespace IntellVoice
}  // namespace OHOS
