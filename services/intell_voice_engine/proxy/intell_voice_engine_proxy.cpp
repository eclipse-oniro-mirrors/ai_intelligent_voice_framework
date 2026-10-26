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

#include "intell_voice_engine_proxy.h"
#include "intell_voice_log.h"

#define LOG_TAG "IntellVoiceEngineProxy"

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t BUFFER_SIZE_MAX = (10 * 1024 * 1024);

void IntellVoiceEngineProxy::SetCallback(sptr<IRemoteObject> object)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    if (object == nullptr) {
        INTELL_VOICE_LOG_ERROR("object is null");
        return;
    }

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteRemoteObject(object);
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_SET_CALLBACK, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("send request error: %{public}d", error);
    }
}

int32_t IntellVoiceEngineProxy::Attach(const IntellVoiceEngineInfo &info)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(info.wakeupPhrase);
    data.WriteBool(info.isPcmFromExternal);
    data.WriteInt32(info.minBufSize);
    data.WriteInt32(info.sampleChannels);
    data.WriteInt32(info.bitsPerSample);
    data.WriteInt32(info.sampleRate);

    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_ATTACH, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("attach error: %{public}d", error);
        return -1;
    }

    INTELL_VOICE_LOG_INFO("Attach call");
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::Detach(void)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_DETACH, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("detach error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::SetParameter(const std::string &keyValueList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(keyValueList);
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_SET_PARAMETER, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("set parameter error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

std::string IntellVoiceEngineProxy::GetParameter(const std::string &key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(key);

    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_GET_PARAMETER, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("get parameter error: %{public}d", error);
        return "";
    }
    return reply.ReadString();
}

int32_t IntellVoiceEngineProxy::Start(bool isLast)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteBool(isLast);

    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_START, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("start error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::Stop(void)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());

    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_STOP, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("stop error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::WriteAudio(const uint8_t *buffer, uint32_t size)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteUint32(size);
    data.WriteBuffer(buffer, size);

    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_WRITE_AUDIO, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("write audio error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::StartCapturer(int32_t channels)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteInt32(channels);
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_STAET_CAPTURER, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("start capturer error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::Read(std::vector<uint8_t> &data)
{
    MessageParcel parcelData;
    MessageParcel reply;
    MessageOption option;

    parcelData.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_READ, parcelData, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("read error: %{public}d", error);
        return -1;
    }

    int ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to read wakeup pcm, ret:%{public}d", ret);
        return ret;
    }
    uint32_t size = reply.ReadUint32();
    if (size == 0) {
        INTELL_VOICE_LOG_ERROR("buffer size is zero");
        return -1;
    }
    if (size > BUFFER_SIZE_MAX) {
        INTELL_VOICE_LOG_ERROR("buffer size is too big");
        return -1;
    }
    const uint8_t *buff = reply.ReadBuffer(size);
    if (buff == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is nullptr");
        return -1;
    }
    data.resize(size);
    std::copy(buff, buff + size, data.begin());
    return ret;
}

int32_t IntellVoiceEngineProxy::StopCapturer()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_STOP_CAPTURER, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("stop capturer error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::GetWakeupPcm(std::vector<uint8_t> &data)
{
    MessageParcel parcelData;
    MessageParcel reply;
    MessageOption option;

    parcelData.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_GET_WAKEUP_PCM, parcelData, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("get wakeup pcm error: %{public}d", error);
        return -1;
    }
    int ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get wakeup pcm, ret:%{public}d", ret);
        return ret;
    }
    uint32_t size = reply.ReadUint32();
    if (size == 0) {
        INTELL_VOICE_LOG_ERROR("buffer size is zero");
        return -1;
    }
    if (size > BUFFER_SIZE_MAX) {
        INTELL_VOICE_LOG_ERROR("buffer size is too big");
        return -1;
    }
    const uint8_t *buff = reply.ReadBuffer(size);
    if (buff == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is nullptr");
        return -1;
    }
    data.resize(size);
    std::copy(buff, buff + size, data.begin());
    return ret;
}

int32_t IntellVoiceEngineProxy::Evaluate(const std::string &word, EvaluationResult &result)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(word);
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_EVALUATE, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("evaluate error: %{public}d", error);
        return -1;
    }
    int32_t ret = reply.ReadInt32();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to evaluate");
        return ret;
    }

    result.score = reply.ReadInt32();
    result.resultCode = reply.ReadInt32();
    return ret;
}

int32_t IntellVoiceEngineProxy::NotifyDeviceWakeupEvent(const std::string &macAddress, const EventDetail &event)
{
    INTELL_VOICE_LOG_INFO("enter");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(macAddress);
    data.WriteInt32(event.wakeupEvent);
    data.WriteInt32(event.endFrameIndex);
    int32_t error = Remote()->SendRequest(INTELL_VOICE_ENGINE_NOTIFY_DEVICE_WAKEUP_EVENT, data, reply, option);
    if (error != 0) {
        INTELL_VOICE_LOG_ERROR("notify headset host event error: %{public}d", error);
        return -1;
    }
    return reply.ReadInt32();
}
}
}
