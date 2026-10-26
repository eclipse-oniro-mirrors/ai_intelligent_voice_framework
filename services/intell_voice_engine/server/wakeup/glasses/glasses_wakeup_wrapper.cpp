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
#include "glasses_wakeup_wrapper.h"
 
#include <dlfcn.h>
#include "intell_voice_log.h"
 
#define LOG_TAG "GlassesWakeupWrapper"
 
namespace OHOS {
namespace IntellVoiceEngine {
static const std::string GLASSES_SO_PATH = "libxr_glass_app_client.z.so";
 
int32_t GlassesWakeupWrapper::LoadLib()
{
    const char *strError = nullptr;
    glassesWakeupPriv_.handle = dlopen(GLASSES_SO_PATH.c_str(), RTLD_LAZY);
    if (glassesWakeupPriv_.handle == nullptr) {
        strError = dlerror();
        INTELL_VOICE_LOG_ERROR("dlopen err:%{public}s", strError);
        return -1;
    }
 
    (void)dlerror(); // clear existing error
    glassesWakeupPriv_.getWakeupInst
        = reinterpret_cast<GetGlassesWakeupInstFunc>(dlsym(glassesWakeupPriv_.handle, "GetWakeupInst"));
    if (glassesWakeupPriv_.getWakeupInst == nullptr) {
        strError = dlerror();
        INTELL_VOICE_LOG_ERROR("dlsym GetGlassesWakeupInst err:%{public}s", strError);
        dlclose(glassesWakeupPriv_.handle);
        glassesWakeupPriv_.handle = nullptr;
        return -1;
    }
 
    INTELL_VOICE_LOG_INFO("load glasses lib success");
    return 0;
}
 
void GlassesWakeupWrapper::UnloadLib()
{
    if (glassesWakeupPriv_.handle != nullptr) {
        dlclose(glassesWakeupPriv_.handle);
        glassesWakeupPriv_.handle = nullptr;
    }
}
 
GlassesWakeupWrapper::GlassesWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (LoadLib() == 0) {
        inst_ = glassesWakeupPriv_.getWakeupInst();
        if (inst_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to get glasses inst");
            UnloadLib();
        }
    }
}

GlassesWakeupWrapper::~GlassesWakeupWrapper()
{
    INTELL_VOICE_LOG_INFO("enter");
    UnloadLib();
    inst_ = nullptr;
}

int32_t GlassesWakeupWrapper::ReadGlassesStream(std::vector<uint8_t> &audioStream, bool &hasAwakeWord)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }
 
    return inst_->ReadStream(audioStream, hasAwakeWord);
}

int32_t GlassesWakeupWrapper::StopReadingStream()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }
 
    return inst_->StopReadingStream();
}

int32_t GlassesWakeupWrapper::SetCallback(DeviceCallBack callFunc)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(mutex_);
    if (inst_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("inst is nullptr");
        return -1;
    }
 
    return inst_->SetCallback(callFunc);
}
}  // namespace IntellVoice
}  // namespace OHOS