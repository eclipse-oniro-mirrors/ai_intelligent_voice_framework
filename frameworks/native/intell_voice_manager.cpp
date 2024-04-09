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
#include "intell_voice_manager.h"

#include <chrono>
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "memory_guard.h"
#include "scope_guard.h"
#include "intell_voice_log.h"
#include "intell_voice_load_callback.h"
#include "intell_voice_service_proxy.h"

#define LOG_TAG "IntellVoiceManager"

using namespace std;
using namespace OHOS::IntellVoiceEngine;

namespace OHOS {
namespace IntellVoice {
constexpr int32_t LOAD_SA_TIMEOUT_MS = 5000;

IntellVoiceManager::IntellVoiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
}

IntellVoiceManager::~IntellVoiceManager()
{
    INTELL_VOICE_LOG_INFO("enter");
}

IntellVoiceManager *IntellVoiceManager::GetInstance()
{
    static IntellVoiceManager manager;
    if (!manager.Init()) {
        return nullptr;
    }
    return &manager;
}

bool IntellVoiceManager::Init()
{
    std::unique_lock<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        INTELL_VOICE_LOG_ERROR("get sa manager failed");
        return false;
    }

    sptr<IntellVoiceLoadCallback> loadCallback = new (std::nothrow)
        IntellVoiceLoadCallback(std::bind(&IntellVoiceManager::LoadSystemAbilitySuccess, this, std::placeholders::_1),
            std::bind(&IntellVoiceManager::LoadSystemAbilityFail, this));
    if (loadCallback == nullptr) {
        INTELL_VOICE_LOG_ERROR("loadCallback is nullptr");
        return false;
    }

    int32_t ret = samgr->LoadSystemAbility(INTELL_VOICE_SERVICE_ID, loadCallback);
    if (ret != ERR_OK) {
        INTELL_VOICE_LOG_ERROR("Failed to load systemAbility");
        return false;
    }

    auto waitStatus = proxyConVar_.wait_for(lock, std::chrono::milliseconds(LOAD_SA_TIMEOUT_MS));
    if (waitStatus != std::cv_status::no_timeout) {
        INTELL_VOICE_LOG_ERROR("Load systemAbility timeout");
        return false;
    }
    INTELL_VOICE_LOG_INFO("Load systemAbility success");
    return true;
}

void IntellVoiceManager::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &object)
{
    std::unique_lock<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("IntellVoiceManager finish start systemAbility");
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("get system ability failed");
        return;
    }
    g_sProxy = iface_cast<IIntellVoiceService>(object);
    if (g_sProxy != nullptr) {
        INTELL_VOICE_LOG_INFO("init Service Proxy success");
    }
    proxyConVar_.notify_one();
}

void IntellVoiceManager::LoadSystemAbilityFail()
{
    std::unique_lock<std::mutex> lock(mutex_);
    g_sProxy = nullptr;
    proxyConVar_.notify_one();
}

int32_t IntellVoiceManager::CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }
    return g_sProxy->CreateIntellVoiceEngine(type, inst);
}

int32_t IntellVoiceManager::ReleaseIntellVoiceEngine(IntellVoiceEngineType type)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }
    return g_sProxy->ReleaseIntellVoiceEngine(type);
}

int32_t IntellVoiceManager::RegisterServiceDeathRecipient(sptr<OHOS::IRemoteObject::DeathRecipient> callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }

    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("service death recipient is null");
        return -1;
    }

    bool ret = g_sProxy->AsObject()->AddDeathRecipient(callback);
    if (!ret) {
        INTELL_VOICE_LOG_ERROR("failed to add death recipient");
        return -1;
    }
    return 0;
}

int32_t IntellVoiceManager::DeregisterServiceDeathRecipient(sptr<OHOS::IRemoteObject::DeathRecipient> callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }

    if (callback == nullptr) {
        INTELL_VOICE_LOG_ERROR("service death recipient is null");
        return -1;
    }

    bool ret = g_sProxy->AsObject()->RemoveDeathRecipient(callback);
    if (!ret) {
        INTELL_VOICE_LOG_ERROR("failed to remove death recipient");
        return -1;
    }
    return 0;
}

int32_t IntellVoiceManager::GetUploadFiles(int numMax, std::vector<UploadFilesInfo> &files)
{
    INTELL_VOICE_LOG_INFO("enter, numMax: %{public}d", numMax);
    CHECK_CONDITION_RETURN_RET(g_sProxy == nullptr, -1, "IntellVoiceService Proxy is null");
    std::vector<UploadHdiFile> hdiFiles;
    int32_t ret = g_sProxy->GetUploadFiles(numMax, hdiFiles);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Get upload files failed, ret:%{public}d", ret);
        return ret;
    }

    if (hdiFiles.empty()) {
        INTELL_VOICE_LOG_ERROR("no upload files");
        return -1;
    }
    INTELL_VOICE_LOG_INFO("upload files size:%{public}u", static_cast<uint32_t>(hdiFiles.size()));
    for (auto hdiFile : hdiFiles) {
        UploadFilesInfo filesInfo;
        filesInfo.type = hdiFile.type;
        filesInfo.filesDescription = hdiFile.filesDescription;
        for (auto content : hdiFile.filesContent) {
            if (content == nullptr) {
                INTELL_VOICE_LOG_ERROR("fileContent is nullptr");
                continue;
            }
            std::vector<uint8_t> fileData;
            if (GetFileDataFromAshmem(content, fileData) != 0) {
                INTELL_VOICE_LOG_ERROR("failed to file data from ashmem");
                continue;
            }
            filesInfo.filesContent.push_back(fileData);
        }
        files.push_back(filesInfo);
    }
    std::vector<UploadHdiFile>().swap(hdiFiles);
    return 0;
}

int32_t IntellVoiceManager::GetFileDataFromAshmem(sptr<Ashmem> ashmem, std::vector<uint8_t> &fileData)
{
    if (ashmem == nullptr) {
        INTELL_VOICE_LOG_ERROR("ashmem is nullptr");
        return -1;
    }

    ON_SCOPE_EXIT {
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
    };

    uint32_t size = static_cast<uint32_t>(ashmem->GetAshmemSize());
    if (size == 0) {
        INTELL_VOICE_LOG_ERROR("size is zero");
        return -1;
    }

    if (!ashmem->MapReadOnlyAshmem()) {
        INTELL_VOICE_LOG_ERROR("map ashmem failed");
        return -1;
    }

    const uint8_t *buffer = static_cast<const uint8_t *>(ashmem->ReadFromAshmem(size, 0));
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("read from ashmem failed");
        return -1;
    }

    fileData.insert(fileData.begin(), buffer, buffer + size);
    return 0;
}

int32_t IntellVoiceManager::SetParameter(const std::string &key, const std::string &value)
{
    INTELL_VOICE_LOG_INFO("enter, key:%{public}s, value:%{public}s", key.c_str(), value.c_str());
    return 0;
}

std::string IntellVoiceManager::GetParameter(const std::string &key)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return "";
    }

    if (key.empty()) {
        INTELL_VOICE_LOG_ERROR("key empty");
        return "";
    }

    return g_sProxy->GetParameter(key);
}

int32_t IntellVoiceManager::GetWakeupSourceFiles(std::vector<WakeupSourceFile> &cloneFileInfo)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService Proxy is null");
        return -1;
    }

    std::vector<std::string> cloneFiles;
    int ret = g_sProxy->GetWakeupSourceFilesList(cloneFiles);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get clone list err");
        return -1;
    }

    WakeupSourceFile fileInfo;
    size_t fileCount = cloneFiles.size();
    cloneFiles.reserve(fileCount);

    for (size_t index = 0; index < fileCount; ++index) {
        fileInfo.filePath = cloneFiles[index];
        ret = g_sProxy->GetWakeupSourceFile(cloneFiles[index], fileInfo.fileContent);
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("get clone file err");
            return -1;
        }
        cloneFileInfo.push_back(fileInfo);
    }

    return 0;
}

int32_t IntellVoiceManager::EnrollWithWakeupFilesForResult(const std::vector<WakeupSourceFile> &cloneFileInfo,
    const std::string &wakeupInfo, const shared_ptr<IIntellVoiceUpdateCallback> callback)
{
    INTELL_VOICE_LOG_INFO("enter");

    if (g_sProxy == nullptr) {
        INTELL_VOICE_LOG_ERROR("IntellVoiceService proxy is null");
        return -1;
    }

    size_t fileCount = cloneFileInfo.size();
    for (size_t index = 0; index < fileCount; ++index) {
        int ret = g_sProxy->SendWakeupFile(cloneFileInfo[index].filePath, cloneFileInfo[index].fileContent);
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("send clone file err, index:%{public}zu, size:%{public}zu, ret:%{public}d",
                index, fileCount, ret);
            return -1;
        }
    }

    callback_ = sptr<UpdateCallbackInner>(new (std::nothrow) UpdateCallbackInner());
    if (callback_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback_ is nullptr");
        return -1;
    }
    callback_->SetUpdateCallback(callback);

    return g_sProxy->EnrollWithWakeupFilesForResult(wakeupInfo, callback_->AsObject());
}
}  // namespace IntellVoice
}  // namespace OHOS