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
#include "swing_service_wrapper.h"

#include <dlfcn.h>
#include "intell_voice_log.h"

#define LOG_TAG "SwingServiceWrapper"

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string SWINGSERVICE_SO_PATH = "libswing_client.z.so";
bool SwingServiceWrapper::isModelLoaded_ = false;
std::mutex SwingServiceWrapper::mutex_;

int32_t SwingServiceWrapper::LoadSwingServiceLib()
{
    if (swingServicePriv_.handle != nullptr) {
        INTELL_VOICE_LOG_INFO("Npu lib has loaded");
        return 0;
    }

    const char *strError = nullptr;
    swingServicePriv_.handle = dlopen(SWINGSERVICE_SO_PATH.c_str(), RTLD_LAZY);
    if (swingServicePriv_.handle == nullptr) {
        strError = dlerror();
        INTELL_VOICE_LOG_ERROR("dlopen err:%{public}s", strError);
        return -1;
    }

    if (LoadLibFunction() > 0) {
        UnloadSwingServiceLib();
        return -1;
    }

    INTELL_VOICE_LOG_INFO("load swingService lib success");
    return 0;
}

int32_t SwingServiceWrapper::LoadLibFunction()
{
    int32_t errCount = 0;
    dlerror(); // clear existing error
    LOAD_FUNCTION(swingServicePriv_.subscribeSwingEvent, SubscribeSwingEventPtr,
        "SubscribeSwingEvent", swingServicePriv_, errCount);
    LOAD_FUNCTION(swingServicePriv_.unSubscribeSwingEvent, UnSubscribeSwingEventPtr,
        "UnSubscribeSwingEvent", swingServicePriv_, errCount);
    return errCount;
}

void SwingServiceWrapper::UnloadSwingServiceLib()
{
    if (swingServicePriv_.handle != nullptr) {
        dlclose(swingServicePriv_.handle);
        swingServicePriv_.handle = nullptr;
    }
    swingServicePriv_.subscribeSwingEvent = nullptr;
    swingServicePriv_.unSubscribeSwingEvent = nullptr;
}

SwingServiceWrapper::SwingServiceWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (LoadSwingServiceLib() != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get swingServicePriv_");
    }
}

SwingServiceWrapper::~SwingServiceWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    UnloadSwingServiceLib();
}

int32_t SwingServiceWrapper::SubscribeSwingEvent(std::string swingEventType, std::string eventParams)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    isModelLoaded_ = true;
    if (swingServicePriv_.subscribeSwingEvent == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get subscribeSwingEvent");
        return -1;
    }
    int32_t ret = swingServicePriv_.subscribeSwingEvent(swingEventType.c_str(), eventParams.c_str());

    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("load tiny model failed: %{public}d", ret);
    }
    return ret; 
}

int32_t SwingServiceWrapper::UnSubscribeSwingEvent(std::string swingEventType, std::string eventParams)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isModelLoaded_) {
        INTELL_VOICE_LOG_INFO("no need to UnSubscribeSwingEvent.");
        return 0;
    }
    isModelLoaded_ = false;
    if (swingServicePriv_.unSubscribeSwingEvent == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to get unSubscribeSwingEvent");
        return -1;
    }
    int32_t ret = swingServicePriv_.unSubscribeSwingEvent(swingEventType.c_str(), eventParams.c_str());
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("unload tiny model failed: %{public}d", ret);
    }
    return ret;
}
}  // namespace IntellVoice
}  // namespace OHOS
