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
#ifndef HEADSET_WAKEUP_WRAPPER_H
#define HEADSET_WAKEUP_WRAPPER_H

#include <string>
#include <mutex>

#include "i_headset_wakeup.h"

namespace OHOS {
namespace IntellVoiceEngine {
using GetHeadsetWakeupInstFunc = IHeadsetWakeup *(*)();

struct HeadsetWakeupManagerPriv {
    void *handle { nullptr };
    GetHeadsetWakeupInstFunc getHeadsetWakeupInst { nullptr };
};

class HeadsetWakeupWrapper {
public:
    HeadsetWakeupWrapper();
    ~HeadsetWakeupWrapper();

    int32_t ReadHeadsetStream(std::vector<uint8_t> &audioStream);
    int32_t NotifyVerifyResult(bool result);
    int32_t StopReadingStream();
    int32_t GetHeadsetAwakeState();

private:
    int32_t LoadHeadsetLib();
    void UnloadHeadsetLib();

private:
    std::mutex mutex_ {};
    HeadsetWakeupManagerPriv headsetWakeupPriv_;
    IHeadsetWakeup *inst_ = nullptr;
};
}
}
#endif