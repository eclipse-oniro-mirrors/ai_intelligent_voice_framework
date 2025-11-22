/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef INTELLVOICE_TAIHE_UTILS_H
#define INTELLVOICE_TAIHE_UTILS_H

#include "intell_voice_manager.h"

#include "ohos.ai.intelligentVoice.proj.hpp"
#include "ohos.ai.intelligentVoice.impl.hpp"
#include "taihe/runtime.hpp"
#include "stdexcept"

namespace OHOS {
namespace IntellVoiceTaihe {
using namespace taihe;
using ohos::ai::intelligentVoice::EnrollResult;
using ohos::ai::intelligentVoice::EvaluationResultCode;
using ohos::ai::intelligentVoice::UploadFileType;
using ohos::ai::intelligentVoice::WakeupSourceFile;

class IntellVoiceTaiheUtils {
public:
    static taihe::array<::ohos::ai::intelligentVoice::UploadFile> ToTaiheArrayUploadFile(
        std::vector<IntellVoice::UploadFilesInfo> &uploadFiles);

    static taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile> ToTaiheArrayWakeupSourceFile(
        std::vector<IntellVoice::WakeupSourceFile> &cloneFile);

    template <typename EnumType, typename ValueType>
    static bool GetEnumKeyByValue(ValueType value, typename EnumType::key_t &key);
};
}  // namespace IntellVoiceTaihe
}  // namespace OHOS
#endif