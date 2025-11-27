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

#include "intellvoice_taihe_utils.h"

#include "intell_voice_log.h"

#define LOG_TAG "WakeupManagerTaihe"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {

taihe::array<::ohos::ai::intelligentVoice::UploadFile> IntellVoiceTaiheUtils::ToTaiheArrayUploadFile(
    std::vector<UploadFilesInfo> &uploadFiles)
{
    std::vector<::ohos::ai::intelligentVoice::UploadFile> arrayUploadFile;

    if (uploadFiles.empty()) {
        INTELL_VOICE_LOG_ERROR("no upload files");
        return taihe::array<::ohos::ai::intelligentVoice::UploadFile>(arrayUploadFile);
    }
    INTELL_VOICE_LOG_INFO("upload files size:%{public}u", static_cast<uint32_t>(uploadFiles.size()));
    for (auto uploadFile : uploadFiles) {
        std::vector<taihe::array<uint8_t>> arrVec;
        for (auto content : uploadFile.filesContent) {
            arrVec.push_back(taihe::array<uint8_t>(content));
        }
        ::ohos::ai::intelligentVoice::UploadFileType::key_t resultCodeKey;
        GetEnumKeyByValue<::ohos::ai::intelligentVoice::UploadFileType>(uploadFile.type, resultCodeKey);
        ::ohos::ai::intelligentVoice::UploadFile filesInfo{
            .type = ohos::ai::intelligentVoice::UploadFileType(resultCodeKey),
            .filesDescription = uploadFile.filesDescription,
            .filesContent = taihe::array<taihe::array<uint8_t>>(arrVec),
        };
        arrayUploadFile.push_back(filesInfo);
    }
    std::vector<UploadFilesInfo>().swap(uploadFiles);
    return taihe::array<::ohos::ai::intelligentVoice::UploadFile>(arrayUploadFile);
}

taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile> IntellVoiceTaiheUtils::ToTaiheArrayWakeupSourceFile(
    std::vector<IntellVoice::WakeupSourceFile> &cloneFile)
{
    std::vector<::ohos::ai::intelligentVoice::WakeupSourceFile> wakeupSourceFile;

    if (cloneFile.empty()) {
        INTELL_VOICE_LOG_ERROR("no upload files");
        return taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile>(wakeupSourceFile);
    }
    INTELL_VOICE_LOG_INFO("cloneFile size:%{public}u", static_cast<uint32_t>(cloneFile.size()));
    for (auto file : cloneFile) {
        ::ohos::ai::intelligentVoice::WakeupSourceFile filesInfo{
            .filePath = file.filePath,
            .fileContent = taihe::array<uint8_t>(file.fileContent),
        };
        wakeupSourceFile.push_back(filesInfo);
    }
    std::vector<IntellVoice::WakeupSourceFile>().swap(cloneFile);
    return taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile>(wakeupSourceFile);
}

template <typename EnumType, typename ValueType>
bool IntellVoiceTaiheUtils::GetEnumKeyByValue(ValueType value, typename EnumType::key_t &key)
{
    for (size_t index = 0; index < std::size(EnumType::table); ++index) {
        if (EnumType::table[index] == value) {
            key = static_cast<typename EnumType::key_t>(index);
            return true;
        }
    }
    return false;
}

template bool IntellVoiceTaiheUtils::GetEnumKeyByValue<EvaluationResultCode, int32_t>(
    int32_t value, typename EvaluationResultCode::key_t &key);

template bool IntellVoiceTaiheUtils::GetEnumKeyByValue<EnrollResult, int32_t>(
    int32_t value, typename EnrollResult::key_t &key);

template bool IntellVoiceTaiheUtils::GetEnumKeyByValue<UploadFileType, int32_t>(
    int32_t value, typename UploadFileType::key_t &key);

}  // namespace IntellVoiceTaihe
}  // namespace OHOS