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

#include "wakeup_manager_taihe.h"
#include "intellvoice_taihe_utils.h"
#include "intell_voice_log.h"

#define LOG_TAG "WakeupManagerTaihe"

using namespace std;
using namespace taihe;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceTaihe;

namespace OHOS {
namespace IntellVoiceTaihe {

WakeupManagerImpl::WakeupManagerImpl()
{}

void WakeupManagerImpl::SetParameterSync(::taihe::string_view key, ::taihe::string_view value)
{
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return;
    }
    manager->SetParameter(std::string(key), std::string(value));
}

::taihe::string WakeupManagerImpl::GetParameterSync(::taihe::string_view key)
{
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return "";
    }
    return manager->GetParameter(std::string(key));
}

taihe::array<::ohos::ai::intelligentVoice::UploadFile> WakeupManagerImpl::GetUploadFilesSync(int32_t maxCount)
{
    std::vector<UploadFilesInfo> uploadFiles;
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return IntellVoiceTaiheUtils::ToTaiheArrayUploadFile(uploadFiles);
    }
    manager->GetUploadFiles(maxCount, uploadFiles);
    return IntellVoiceTaiheUtils::ToTaiheArrayUploadFile(uploadFiles);
}

taihe::array<::ohos::ai::intelligentVoice::WakeupSourceFile> WakeupManagerImpl::GetWakeupSourceFilesSync()
{
    std::vector<IntellVoice::WakeupSourceFile> cloneFile;
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return IntellVoiceTaiheUtils::ToTaiheArrayWakeupSourceFile(cloneFile);
    }
    manager->GetWakeupSourceFiles(cloneFile);
    return IntellVoiceTaiheUtils::ToTaiheArrayWakeupSourceFile(cloneFile);
}

::ohos::ai::intelligentVoice::EnrollResult WakeupManagerImpl::EnrollWithWakeupFilesForResultSync(
    ::taihe::array_view<::ohos::ai::intelligentVoice::WakeupSourceFile> wakeupFiles, ::taihe::string_view wakeupInfo)
{
    ::ohos::ai::intelligentVoice::EnrollResult::key_t resultCodeKey;
    int retCode = UNKNOWN;
    IntellVoiceTaiheUtils::GetEnumKeyByValue<::ohos::ai::intelligentVoice::EnrollResult>(retCode, resultCodeKey);
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return ohos::ai::intelligentVoice::EnrollResult(resultCodeKey);
    }
    callback_ =  std::make_shared<IntellVoiceUpdateCallbackTaihe>();
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create intell voice update callback taihe failed");
        return ohos::ai::intelligentVoice::EnrollResult(resultCodeKey);
    }
    std::vector<IntellVoice::WakeupSourceFile> cloneFiles;
    for (int i = 0; i < wakeupFiles.size(); i++) {
        std::vector<uint8_t> dataVec;
        for (int j = 0; j < wakeupFiles[i].fileContent.size(); j++) {
            dataVec.push_back(wakeupFiles[i].fileContent[j]);
        }
        IntellVoice::WakeupSourceFile cloneFile {
            .filePath = std::string(wakeupFiles[i].filePath),
            .fileContent = dataVec,
        };
        cloneFiles.push_back(cloneFile);
    }
    int ret = manager->EnrollWithWakeupFilesForResult(cloneFiles, std::string(wakeupInfo), callback_);
    IntellVoiceTaiheUtils::GetEnumKeyByValue<::ohos::ai::intelligentVoice::EnrollResult>(ret, resultCodeKey);
    return ohos::ai::intelligentVoice::EnrollResult(resultCodeKey);
}

void WakeupManagerImpl::ClearUserDataSync()
{
    auto manager = IntellVoiceManager::GetInstance();
    if (manager == nullptr) {
        INTELL_VOICE_LOG_ERROR("manager is nullptr");
        return;
    }
    manager->ClearUserData();
}

WakeupManager GetWakeupManager()
{
    return taihe::make_holder<WakeupManagerImpl, WakeupManager>();
}
}  // namespace IntellVoiceTaihe
}  // namespace OHOS

TH_EXPORT_CPP_API_GetWakeupManager(OHOS::IntellVoiceTaihe::GetWakeupManager);
