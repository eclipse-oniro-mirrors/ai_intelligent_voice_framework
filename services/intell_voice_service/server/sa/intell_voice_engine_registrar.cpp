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
#include <any>

#include "engine_callback_message.h"
#include "intell_voice_log.h"
#include "intell_voice_engine_registrar.h"

#define LOG_TAG "IntellVoiceEngineRegistrar"

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t ARGS_ONE = 1;
static constexpr uint32_t ARGS_TWO = 2;
static constexpr uint32_t ARGS_THREE = 3;
static constexpr uint32_t ARGS_INDEX_0 = 0;
static constexpr uint32_t ARGS_INDEX_1 = 1;
static constexpr uint32_t ARGS_INDEX_2 = 2;

void IntellVoiceEngineRegistrar::RegisterEngineCallbacks()
{
    RegisterHandleCloseWakeupSource();
    RegisterHandleClearWakeupEngineCb();
    RegisterHandleReleaseEngine();
    RegisterHandleDeviceServiceDie();
    RegisterHandleUpdateComplete();
    RegisterHandleUpdateRetry();
    RegisterReleaseEngine();
    RegisterReleaseImplicitEnrollEngine();
    RegisterQuerySwitchStatus();
    RegisterTriggerGetParameter();
    RegisterTriggerSetParameter();
    RegisterTriggerMgrUpdateModel();
}

void IntellVoiceEngineRegistrar::RegisterHandleCloseWakeupSource()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_CLOSE_WAKEUP_SOURCE,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("HANDLE_CLOSE_WAKEUP_SOURCE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                bool isNeedStop = std::any_cast<bool>(args[ARGS_INDEX_0]);
                HandleCloseWakeupSource(isNeedStop);
                return std::nullopt;
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("HANDLE_CLOSE_WAKEUP_SOURCE bad any cast");
                return std::nullopt;
            }
        });
}

void IntellVoiceEngineRegistrar::RegisterHandleClearWakeupEngineCb()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_CLEAR_WAKEUP_ENGINE_CB,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != 0) {
                INTELL_VOICE_LOG_ERROR("HANDLE_CLEAR_WAKEUP_ENGINE_CB receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            HandleClearWakeupEngineCb();
            return std::nullopt;
        });
}

void IntellVoiceEngineRegistrar::RegisterHandleReleaseEngine()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_RELEASE_ENGINE,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("HANDLE_RELEASE_ENGINE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                IntellVoiceEngineType type = std::any_cast<IntellVoiceEngineType>(args[ARGS_INDEX_0]);
                int32_t result = HandleReleaseEngine(type);
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("HANDLE_RELEASE_ENGINE bad any cast");
                return std::nullopt;
            }
        });
}

void IntellVoiceEngineRegistrar::RegisterHandleDeviceServiceDie()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_DEVICE_SERVICE_DIE,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("HANDLE_DEVICE_SERVICE_DIE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                std::string serviceName = std::any_cast<std::string>(args[ARGS_INDEX_0]);
                HandleDeviceServiceDie(serviceName);
                return std::nullopt;
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("HANDLE_DEVICE_SERVICE_DIE bad any cast");
                return std::nullopt;
            }
        });
}

void IntellVoiceEngineRegistrar::RegisterHandleUpdateComplete()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_UPDATE_COMPLETE,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_TWO || !args[ARGS_INDEX_0].has_value() || !args[ARGS_INDEX_1].has_value()) {
                INTELL_VOICE_LOG_ERROR("HANDLE_UPDATE_COMPLETE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                int32_t result = std::any_cast<int32_t>(args[ARGS_INDEX_0]);
                const std::string &param = std::any_cast<std::string>(args[ARGS_INDEX_1]);
                HandleUpdateComplete(result, param);
                return std::nullopt;
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("HANDLE_UPDATE_COMPLETE bad any cast");
                return std::nullopt;
            }
        });
}

void IntellVoiceEngineRegistrar::RegisterHandleUpdateRetry()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::HANDLE_UPDATE_RETRY,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != 0) {
                INTELL_VOICE_LOG_ERROR("HANDLE_UPDATE_RETRY receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            HandleUpdateRetry();
            return std::nullopt;
        });
}

void IntellVoiceEngineRegistrar::RegisterReleaseEngine()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::RELEASE_ENGINE, [this](const std::vector<std::any> &args)
        -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("RELEASE_ENGINE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                IntellVoiceEngineType type = std::any_cast<IntellVoiceEngineType>(args[ARGS_INDEX_0]);
                int32_t result = ReleaseEngine(type);
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("RELEASE_ENGINE bad any cast");
                return std::nullopt;
            }
    });
}

void IntellVoiceEngineRegistrar::RegisterReleaseImplicitEnrollEngine()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::RELEASE_IMPLICIT_ENROLL_ENGINE,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() >= ARGS_ONE) {
                INTELL_VOICE_LOG_ERROR("RELEASE_IMPLICIT_ENROLL_ENGINE receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                int32_t result = ReleaseImplicitEnrollEngine();
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("RELEASE_IMPLICIT_ENROLL_ENGINE bad any cast");
                return std::nullopt;
            }
        });
}

void IntellVoiceEngineRegistrar::RegisterQuerySwitchStatus()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::QUERY_SWITCH_STATUS,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("QUERY_SWITCH_STATUS receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                const std::string &key = std::any_cast<std::string>(args[ARGS_INDEX_0]);
                bool result = QuerySwitchStatus(key);
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("QUERY_SWITCH_STATUS bad any cast");
                return std::nullopt;
            }
    });
}

void IntellVoiceEngineRegistrar::RegisterTriggerGetParameter()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::TRIGGERMGR_GET_PARAMETER,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_ONE || !args[ARGS_INDEX_0].has_value()) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_GET_PARAMETER receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                const std::string &key = std::any_cast<std::string>(args[ARGS_INDEX_0]);
                std::string result = TriggerGetParameter(key);
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_GET_PARAMETER bad any cast");
                return std::nullopt;
            }
    });
}

void IntellVoiceEngineRegistrar::RegisterTriggerSetParameter()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::TRIGGERMGR_SET_PARAMETER,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_TWO || !args[ARGS_INDEX_0].has_value() || !args[ARGS_INDEX_1].has_value()) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_SET_PARAMETER receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                const std::string &key = std::any_cast<std::string>(args[ARGS_INDEX_0]);
                const std::string &value = std::any_cast<std::string>(args[ARGS_INDEX_1]);
                int32_t result = TriggerSetParameter(key, value);
                return std::optional<std::any>(result);
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_SET_PARAMETER bad any cast");
                return std::nullopt;
            }
    });
}

void IntellVoiceEngineRegistrar::RegisterTriggerMgrUpdateModel()
{
    EngineCallbackMessage::RegisterFunc(EngineCbMessageId::TRIGGERMGR_UPDATE_MODEL,
        [this](const std::vector<std::any> &args) -> std::optional<std::any> {
            if (args.size() != ARGS_THREE || !args[ARGS_INDEX_0].has_value() ||
                !args[ARGS_INDEX_1].has_value() || !args[ARGS_INDEX_2].has_value()) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_UPDATE_MODEL receiving wrong param size: %{public}u",
                    static_cast<uint32_t>(args.size()));
                return std::nullopt;
            }
            try {
                const std::vector<uint8_t> &buffer = std::any_cast<std::vector<uint8_t>>(args[ARGS_INDEX_0]);
                int32_t uuid = std::any_cast<int32_t>(args[ARGS_INDEX_1]);
                IntellVoiceTrigger::TriggerModelType type =
                    std::any_cast<IntellVoiceTrigger::TriggerModelType>(args[ARGS_INDEX_2]);
                TriggerMgrUpdateModel(buffer, uuid, type);
                return std::nullopt;
            } catch (const std::bad_any_cast&) {
                INTELL_VOICE_LOG_ERROR("TRIGGERMGR_UPDATE_MODEL bad any cast");
                return std::nullopt;
            }
    });
}
}  // namespace IntellVoiceEngine
} // namespace OHOS