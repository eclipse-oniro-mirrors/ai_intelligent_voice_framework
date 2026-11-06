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
#ifndef SWING_SERVICE_WRAPPER_H
#define SWING_SERVICE_WRAPPER_H

#include <string>
#include <mutex>

#include "intell_voice_log.h"

#define LOG_TAG "SwingServiceWrapper"

namespace OHOS {
namespace IntellVoiceEngine {
#define LOAD_FUNCTION(funcPoint, funcPrototype, funcName, impl, errCount)          \
    do {                                                                 \
        (funcPoint) = (funcPrototype)dlsym((impl).handle, (funcName)); \
        const char *strErr = dlerror();                                  \
        if (strErr != nullptr) {                                         \
            INTELL_VOICE_LOG_ERROR("load function failed: %{public}s", strErr);               \
            (errCount)++;                                        \
        }                                                                \
    } while (0)

using SubscribeSwingEventPtr = int (*)(const char *swingEventType, const char *eventParams);
using UnSubscribeSwingEventPtr = int (*)(const char *swingEventType, const char *eventParams);

const std::string SHORT_WORD_EVENT_TYPE = "AUDIO_WAKEUP_1_5";
const std::string SHORT_WORD_DOMAIN_ID = "7";
const std::string DUAL_MIC_EVENT_TYPE = "AUDIO_WAKEUP_1_5";
const std::string DUAL_MIC_DOMAIN_ID = "7";

struct SwingServiceManagerPriv {
    void *handle { nullptr };
    SubscribeSwingEventPtr subscribeSwingEvent { nullptr };
    UnSubscribeSwingEventPtr unSubscribeSwingEvent { nullptr };
};

class SwingServiceWrapper {
public:
    SwingServiceWrapper();
    ~SwingServiceWrapper();

    static SwingServiceWrapper &GetInstance()
    {
        static SwingServiceWrapper swingServiceWrapper;
        return swingServiceWrapper;
    }

    int32_t SubscribeSwingEvent(std::string swingEventType, std::string eventParams);
    int32_t UnSubscribeSwingEvent(std::string swingEventType, std::string eventParams);

private:
    int32_t LoadSwingServiceLib();
    void UnloadSwingServiceLib();
    int32_t LoadLibFunction();

private:
    static std::mutex mutex_;
    static bool isModelLoaded_;
    SwingServiceManagerPriv swingServicePriv_;
};
}
}
#undef LOG_TAG
#endif