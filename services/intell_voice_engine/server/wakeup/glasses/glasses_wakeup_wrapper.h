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
#ifndef GLASSES_WAKEUP_WRAPPER_H
#define GLASSES_WAKEUP_WRAPPER_H
 
#include <string>
#include <mutex>
 
#include "i_device_wakeup.h"
 
namespace OHOS {
namespace IntellVoiceEngine {
using GetGlassesWakeupInstFunc = IDeviceWakeup *(*)();
 
struct GlassesWakeupManagerPriv {
    void *handle { nullptr };
    GetGlassesWakeupInstFunc getWakeupInst { nullptr };
};
 
class GlassesWakeupWrapper {
public:
    GlassesWakeupWrapper();
    ~GlassesWakeupWrapper();
 
    static GlassesWakeupWrapper &GetInstance()
    {
        static GlassesWakeupWrapper glassesWakeupWrapper;
        return glassesWakeupWrapper;
    }
 
    int32_t ReadGlassesStream(std::vector<uint8_t> &audioStream, bool &hasAwakeWord);
    int32_t StopReadingStream();
    int32_t SetCallback(DeviceCallBack callFunc);
 
private:
    int32_t LoadLib();
    void UnloadLib();
 
private:
    std::mutex mutex_ {};
    GlassesWakeupManagerPriv glassesWakeupPriv_;
    IDeviceWakeup *inst_ = nullptr;
};
}
}
#endif