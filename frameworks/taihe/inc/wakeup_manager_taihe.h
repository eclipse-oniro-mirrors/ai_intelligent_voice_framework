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
#ifndef WAKEUP_MANAGER_TAIHE_H
#define WAKEUP_MANAGER_TAIHE_H

#include "intell_voice_manager.h"
#include "intell_voice_update_callback_taihe.h"

#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using ohos::ai::intelligentVoice::WakeupManager;

class WakeupManagerImpl {
public:
    WakeupManagerImpl();

    void SetParameterSync(::taihe::string_view key, ::taihe::string_view value);

    ::taihe::string GetParameterSync(::taihe::string_view key);

    ::taihe::array<::ohos::ai::intelligentVoice::UploadFile> GetUploadFilesSync(int32_t maxCount);

    ::taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile> GetWakeupSourceFilesSync();

    ::ohos::ai::intelligentVoice::EnrollResult EnrollWithWakeupFilesForResultSync(
        ::taihe::array_view<::ohos::ai::intelligentVoice::WakeupSourceFile> wakeupFiles,
        ::taihe::string_view wakeupInfo);

    void ClearUserDataSync();

private:
    std::shared_ptr<IntellVoiceUpdateCallbackTaihe> callback_ = nullptr;
};

}  // namespace IntellVoiceTaihe
}  // namespace OHOS

#endif
