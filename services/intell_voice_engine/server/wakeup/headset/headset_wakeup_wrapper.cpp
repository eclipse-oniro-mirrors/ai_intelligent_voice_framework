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
#include "headset_wakeup_wrapper.h"

#include <dlfcn.h>
#include "intell_voice_log.h"

#define LOG_TAG "HeadsetWakeupWrapper"

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string HEADSET_SO_PATH = "libaam_connection_inner_client.z.so";

int32_t HeadsetWakeupWrapper::LoadHeadsetLib()
{
    const char *strError = nullptr;
    headsetWakeupPriv_.handle = dlopen(HEADSET_SO_PATH.c_str(), RTLD_LAZY);
    if (headsetWakeupPriv_.handle == nullptr) {
        strError = dlerror();
        INTELL_VOICE_LOG_ERROR("dlopen err:%{public}s", strError);
        return -1;
    }

    (void)dlerror(); // clear existing error
    headsetWakeupPriv_.getHeadsetWakeupInst = reinterpret_cast<GetHeadsetWakeupInstFunc>(dlsym(
        headsetWakeupPriv_.handle, "GetHeadsetWakeupInst"));
    if (headsetWakeupPriv_.getHeadsetWakeupInst == nullptr) {
        strError = dlerror();
        INTELL_VOICE_LOG_ERROR("dlsym GetHeadSetWakeupInst err:%{public}s", strError);
        dlclose(headsetWakeupPriv_.handle);
        headsetWakeupPriv_.handle = nullptr;
        return -1;
    }

    INTELL_VOICE_LOG_INFO("load headset lib success");
    return 0;
}

void HeadsetWakeupWrapper::UnloadHeadsetLib()
{
    if (headsetWakeupPriv_.handle != nullptr) {
        dlclose(headsetWakeupPriv_.handle);
        headsetWakeupPriv_.handle = nullptr;
    }
}

HeadsetWakeupWrapper::HeadsetWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (LoadHeadsetLib() == 0) {
        inst_ = headsetWakeupPriv_.getHeadsetWakeupInst();
        if (inst_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to get headset inst");
        }
    }
}

HeadsetWakeupWrapper::~HeadsetWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    UnloadHeadsetLib();
    inst_ = nullptr;
}

int32_t HeadsetWakeupWrapper::ReadHeadsetStream(std::vector<uint8_t> &audioStream, bool &hasAwakeWord)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }

    return inst_->ReadHeadsetStream(audioStream, hasAwakeWord);
}

int32_t HeadsetWakeupWrapper::NotifyVerifyResult(bool result)
{
    INTELL_VOICE_LOG_INFO("enter");
    // 使用独立notifyMutex_而非mutex_，避免被ReadHeadsetStream循环占用锁阻塞，保证验证结果及时上报
    std::lock_guard<std::mutex> lock(notifyMutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }

    return inst_->NotifyVerifyResult(result);
}

int32_t HeadsetWakeupWrapper::StopReadingStream()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }

    return inst_->StopReadingStream();
}
}  // namespace IntellVoice
}  // namespace OHOS
