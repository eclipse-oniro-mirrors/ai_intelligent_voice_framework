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

#ifndef INTELL_VOICE_MANAGER_H
#define INTELL_VOICE_MANAGER_H

#include <condition_variable>
#include "iremote_object.h"
#include "iremote_broker.h"
#include "i_intell_voice_engine.h"
#include "i_intell_voice_service.h"
#include "update_callback_inner.h"
#include "intell_voice_info.h"
#include "wakeup_intell_voice_engine.h"

namespace OHOS {
namespace IntellVoice {
using OHOS::IntellVoiceEngine::IntellVoiceEngineType;
using OHOS::IntellVoiceEngine::IIntellVoiceEngine;
using OHOS::IntellVoiceEngine::WakeupDeviceInfo;

struct UploadFilesInfo {
    int32_t type;
    std::string filesDescription;
    std::vector<std::vector<uint8_t>> filesContent;
};

class IntellVoiceManager {
public:
    static IntellVoiceManager *GetInstance();

    int32_t CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst);
    int32_t ReleaseIntellVoiceEngine(IntellVoiceEngineType type);
    bool Init();

    std::shared_ptr<WakeupIntellVoiceEngine> CreateHeadsetWakeupEngine();

    int32_t RegisterServiceDeathRecipient(sptr<OHOS::IRemoteObject::DeathRecipient> callback);
    int32_t DeregisterServiceDeathRecipient(sptr<OHOS::IRemoteObject::DeathRecipient> callback);
    int32_t RegisterDeviceWakeupInfo(const std::string &macAddress, const WakeupDeviceInfo &info);
    int32_t DeregisterDeviceWakeupInfo(const std::string &macAddress);
    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesInfo> &reportedFilesInfo);

    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    int32_t GetWakeupSourceFiles(std::vector<WakeupSourceFile> &cloneFileInfo);
    int32_t EnrollWithWakeupFilesForResult(const std::vector<WakeupSourceFile> &cloneFileInfo,
        const std::string &wakeupInfo, const std::shared_ptr<IIntellVoiceUpdateCallback> callback);
    int32_t ClearUserData();
private:
    IntellVoiceManager();
    ~IntellVoiceManager();
    int32_t GetFileDataFromAshmem(sptr<Ashmem> ashmem, std::vector<uint8_t> &fileData);

    std::mutex mutex_;
    std::condition_variable proxyConVar_;
    sptr<OHOS::IntellVoiceEngine::IIntellVoiceService> g_sProxy;
    sptr<UpdateCallbackInner> callback_ = nullptr;
};
}  // namespace IntellVoice
}  // namespace OHOS

#endif
