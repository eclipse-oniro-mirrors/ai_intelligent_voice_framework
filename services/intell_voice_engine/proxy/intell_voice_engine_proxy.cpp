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
    Remote()->SendRequest(INTELL_VOICE_ENGINE_SET_CALLBACK, data, reply, option);
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

    Remote()->SendRequest(INTELL_VOICE_ENGINE_ATTACH, data, reply, option);

    INTELL_VOICE_LOG_INFO("Attach call");
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::Detach(void)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    Remote()->SendRequest(INTELL_VOICE_ENGINE_DETACH, data, reply, option);
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::SetParameter(const std::string &keyValueList)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(keyValueList);
    Remote()->SendRequest(INTELL_VOICE_ENGINE_SET_PARAMETER, data, reply, option);
    return reply.ReadInt32();
}

std::string IntellVoiceEngineProxy::GetParameter(const std::string &key)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());
    data.WriteString(key);

    Remote()->SendRequest(INTELL_VOICE_ENGINE_GET_PARAMETER, data, reply, option);
    return reply.ReadString();
}

int32_t IntellVoiceEngineProxy::Start(bool isLast)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());

    Remote()->SendRequest(INTELL_VOICE_ENGINE_START, data, reply, option);
    return reply.ReadInt32();
}

int32_t IntellVoiceEngineProxy::Stop(void)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(IIntellVoiceEngine::GetDescriptor());

    Remote()->SendRequest(INTELL_VOICE_ENGINE_STOP, data, reply, option);
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

    Remote()->SendRequest(INTELL_VOICE_ENGINE_WRITE_AUDIO, data, reply, option);
    return reply.ReadInt32();
}
}
}
