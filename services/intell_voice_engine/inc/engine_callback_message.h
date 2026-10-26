
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
#ifndef ENGINE_CALLBACK_MESSAGE_H
#define ENGINE_CALLBACK_MESSAGE_H

#include <functional>
#include <map>
#include <string>
#include <any>
#include <optional>
#include "nocopyable.h"
#include "intell_voice_log.h"

#undef LOG_TAG
#define LOG_TAG "EngineCallbackMessage"

namespace OHOS {
namespace IntellVoiceEngine {
enum EngineCbMessageId {
    HANDLE_CLOSE_WAKEUP_SOURCE,
    HANDLE_HEADSET_HOST_DIE,
    HANDLE_CLEAR_WAKEUP_ENGINE_CB,
    HANDLE_RELEASE_ENGINE,
    HANDLE_ENGINE_SET_SENSIBILITY,
    HANDLE_UPDATE_COMPLETE,
    HANDLE_UPDATE_RETRY,
    RELEASE_ENGINE,
    RELEASE_IMPLICIT_ENROLL_ENGINE,
    QUERY_SWITCH_STATUS,
    TRIGGERMGR_GET_PARAMETER,
    TRIGGERMGR_SET_PARAMETER,
    TRIGGERMGR_UPDATE_MODEL,
    HANDLE_DEVICE_SERVICE_DIE,
};

class EngineCallbackMessage {
public:
    EngineCallbackMessage() = default;
    ~EngineCallbackMessage() = default;

    using Func = std::function<std::optional<std::any>(const std::vector<std::any>&)>;

    static void RegisterFunc(EngineCbMessageId id, Func func)
    {
        EngineFuncMap[id] = func;
    }
    template <typename... Args>
    static std::optional<std::any> CallFunc(EngineCbMessageId id, Args &&... args)
    {
        std::vector<std::any> params = { std::forward<Args>(args)... };
        if (EngineFuncMap.find(id) != EngineFuncMap.end()) {
            INTELL_VOICE_LOG_INFO("enter, EngineCbMessageId: %{public}d, Number of arguments: %{public}u",
                id, static_cast<uint32_t>(params.size()));
            return EngineFuncMap[id](params);
        } else {
            INTELL_VOICE_LOG_ERROR("Engine Callback Function not found");
            return std::nullopt;
        }
    }

private:
    static std::map<EngineCbMessageId, Func> EngineFuncMap;
    DISALLOW_COPY_AND_MOVE(EngineCallbackMessage);
};
}  // namespace IntellVoice
}  // namespace OHOS

#undef LOG_TAG

#endif