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

#include "intell_voice_service_proxy.h"

#include <cinttypes>
#include "intell_voice_log.h"
#include "intell_voice_death_recipient_stub.h"

#define LOG_TAG "IntellVoiceServiceProxy"
namespace OHOS {
namespace IntellVoiceEngine {
#define CROSS_PROCESS_BUF_SIZE_LIMIT (256 *1024)

int32_t IntellVoiceServiceProxy::CreateIntellVoiceEngine(IntellVoiceEngineType type, sptr<IIntellVoiceEngine> &inst)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteInt32(static_cast<int>(type));

    auto stub = new IntellVoiceDeathRecipientStub();
    data.WriteRemoteObject(stub);

    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_CREATE_ENGINE, data, reply, option);
    int32_t ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to create intell voice engine, type:%{public}d", type);
        return ret;
    }

    sptr<IRemoteObject> remote = reply.ReadRemoteObject();
    if (remote == nullptr) {
        INTELL_VOICE_LOG_ERROR("create engine failed, engine is null");
        return -1;
    }
    inst = iface_cast<IIntellVoiceEngine>(remote);
    return 0;
}

int32_t IntellVoiceServiceProxy::ReleaseIntellVoiceEngine(IntellVoiceEngineType type)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteInt32(type);

    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_RELEASE_ENGINE, data, reply, option);
    return reply.ReadInt32();
}

int32_t IntellVoiceServiceProxy::GetUploadFiles(int numMax, std::vector<UploadHdiFile> &files)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());

    data.WriteInt32(numMax);
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_GET_REPORTED_FILES, data, reply, option);
    int32_t ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get reported files, ret:%{public}d", ret);
        return ret;
    }
    int32_t size = reply.ReadInt32();
    INTELL_VOICE_LOG_INFO("reported files info size:%{public}d", size);
    if (size <= 0) {
        return -1;
    }

    for (int32_t i = 0; i < size; i++) {
        UploadHdiFile file;
        file.type = static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_2::UploadHdiFileType>(reply.ReadInt32());
        file.filesDescription = reply.ReadString();
        int32_t filesContentSize = reply.ReadInt32();
        INTELL_VOICE_LOG_INFO("filesContentSize:%{public}d", filesContentSize);
        for (int32_t j = 0; j < filesContentSize; j++) {
            file.filesContent.push_back(reply.ReadAshmem());
        }
        files.push_back(file);
    }
    return ret;
}

std::string IntellVoiceServiceProxy::GetParameter(const std::string &key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteString(key);
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_GET_PARAMETER, data, reply, option);
    return reply.ReadString();
}

int32_t IntellVoiceServiceProxy::GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_GET_CLONE_FILES_LIST, data, reply, option);
    int32_t ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get wakeup source files list failed, ret:%{public}d", ret);
        return ret;
    }

    uint32_t size = reply.ReadUint32();
    cloneFiles.reserve(size);
    for (uint32_t i = 0; i < size; i++) {
        cloneFiles.push_back(reply.ReadString());
    }

    return ret;
}

int32_t IntellVoiceServiceProxy::GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteString(filePath);
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_GET_CLONE_FILE, data, reply, option);
    int32_t ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("get wakeup source file failed, ret:%{public}d", ret);
        return ret;
    }

    uint32_t size = reply.ReadUint32();
    const uint8_t *buff = reply.ReadBuffer(size);
    if (buff == nullptr) {
        INTELL_VOICE_LOG_ERROR("buf is null");
        return -1;
    }

    if (size == 0 || size > CROSS_PROCESS_BUF_SIZE_LIMIT) {
        INTELL_VOICE_LOG_ERROR("buf size is invalid %{public}u", size);
        return -1;
    }

    buffer.resize(size);
    std::copy(buff, buff + size, buffer.data());

    return ret;
}

int32_t IntellVoiceServiceProxy::SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (buffer.data() == nullptr) {
        INTELL_VOICE_LOG_ERROR("buf is null");
        return -1;
    }

    if (buffer.size() == 0 || buffer.size() > CROSS_PROCESS_BUF_SIZE_LIMIT) {
        INTELL_VOICE_LOG_ERROR("buffer size is invalid %{public}u", static_cast<uint32_t>(buffer.size()));
        return -1;
    }

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteString(filePath);
    data.WriteUint32(buffer.size());
    data.WriteBuffer(buffer.data(), buffer.size());
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_SEND_CLONE_FILE, data, reply, option);
    return reply.ReadInt32();
}

int32_t IntellVoiceServiceProxy::EnrollWithWakeupFilesForResult(const std::string &wakeupInfo,
    sptr<IRemoteObject> object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceService::GetDescriptor());
    data.WriteString(wakeupInfo);
    data.WriteRemoteObject(object);
    Remote()->SendRequest(HDI_INTELL_VOICE_SERVICE_CLONE_FOR_RESULT, data, reply, option);
    return reply.ReadInt32();
}
}  // namespace IntellVoiceEngine
}  // namespace OHOS
