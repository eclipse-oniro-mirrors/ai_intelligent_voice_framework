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
#include "only_first_wakeup_engine.h"
#include "only_first_engine_manager.h"
#include "only_first_wakeup_engine_impl.h"
#include "intell_voice_log.h"
#include "engine_callback_message.h"
#include "intell_voice_util.h"

#define LOG_TAG "OnlyFirstWakeupEngine"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string SINGLE_WAKEUP_RECORD_START = "record_start";

OnlyFirstWakeupEngine::OnlyFirstWakeupEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

OnlyFirstWakeupEngine::~OnlyFirstWakeupEngine()
{
    INTELL_VOICE_LOG_INFO("enter");
}

int32_t OnlyFirstWakeupEngine::HandleCapturerMsg(StateMsg &msg)
{
    return ROLE(OnlyFirstWakeupEngineImpl).Handle(msg);
}

void OnlyFirstWakeupEngine::OnDetected(int32_t uuid)
{
    INTELL_VOICE_LOG_INFO("enter, uuid is %{public}d", uuid);
    StateMsg msg(START_RECOGNIZE, &uuid, sizeof(int32_t));
    if (HandleCapturerMsg(msg) != 0) {
        INTELL_VOICE_LOG_WARN("start failed");
        EngineCallbackMessage::CallFunc(HANDLE_CLOSE_WAKEUP_SOURCE, true);
    }
}

bool OnlyFirstWakeupEngine::Init(const std::string & /* param */, bool reEnroll)
{
    StateMsg msg(INIT);
    if (HandleCapturerMsg(msg) != 0) {
        return false;
    }
    return true;
}

int32_t OnlyFirstWakeupEngine::StartCapturer(int32_t channels)
{
    StateMsg msg(START_CAPTURER, &channels, sizeof(int32_t));
    return HandleCapturerMsg(msg);
}

int32_t OnlyFirstWakeupEngine::Read(std::vector<uint8_t> &data)
{
    CapturerData capturerData;
    StateMsg msg(READ, nullptr, 0, reinterpret_cast<void *>(&capturerData));
    int32_t ret = HandleCapturerMsg(msg);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("read failed, ret:%{public}d", ret);
        return -1;
    }

    data.swap(capturerData.data);
    return 0;
}

int32_t OnlyFirstWakeupEngine::StopCapturer()
{
    StateMsg msg(STOP_CAPTURER);
    return HandleCapturerMsg(msg);
}

int32_t OnlyFirstWakeupEngine::Detach(void)
{
    StateMsg msg(RELEASE);
    return HandleCapturerMsg(msg);
}

int32_t OnlyFirstWakeupEngine::SetParameter(const std::string &keyValueList)
{
    std::map<std::string, std::string> kvpairs;

    IntellVoiceUtil::SplitStringToKVPair(keyValueList, kvpairs);
    for (auto it : kvpairs) {
        if (it.first == SINGLE_WAKEUP_RECORD_START) {
            ResetSingleLevelWakeup(it.second);
        } else {
            INTELL_VOICE_LOG_INFO("no need to process, key:%{public}s, value:%{public}s",
                it.first.c_str(), it.second.c_str());
        }
    }

    return 0;
}

std::string OnlyFirstWakeupEngine::GetParameter(const std::string &key)
{
    StringParam keyParam(key);
    StringParam valueParam;
    StateMsg msg(GET_PARAM, &keyParam, sizeof(keyParam), &valueParam);
    if (HandleCapturerMsg(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get parameter, key:%{public}s", key.c_str());
        return "";
    }

    return valueParam.strParam;
}

void OnlyFirstWakeupEngine::ReleaseAdapter()
{
    StateMsg msg(RELEASE);
    HandleCapturerMsg(msg);
}

void OnlyFirstWakeupEngine::ResetSingleLevelWakeup(const std::string &value)
{
    int32_t recordStart = -1;
    INTELL_VOICE_LOG_INFO("record_start:%{public}s", value.c_str());
    if (value != std::string("true") && value != std::string("false")) {
        return;
    }

    recordStart = (value == std::string("true")) ? 1 : 0;
    StateMsg msg(RECORD_START, &recordStart, sizeof(int32_t));
    HandleCapturerMsg(msg);
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS
