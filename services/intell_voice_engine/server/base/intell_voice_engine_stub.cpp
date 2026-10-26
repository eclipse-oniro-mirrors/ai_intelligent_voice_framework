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

#include "intell_voice_engine_stub.h"
#include "securec.h"
#include "intell_voice_log.h"

#define LOG_TAG "IntellVoiceEngineStub"

namespace OHOS {
namespace IntellVoiceEngine {
IntellVoiceEngineStub::IntellVoiceEngineStub()
{
    processFuncMap_[INTELL_VOICE_ENGINE_SET_CALLBACK] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->SetCallbackInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_ATTACH] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->AttachInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_DETACH] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->DetachInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_SET_PARAMETER] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->SetParameterInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_GET_PARAMETER] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->GetParameterInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_START] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->StartInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_STOP] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->StopInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_WRITE_AUDIO] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->WriteAudioInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_STAET_CAPTURER] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->StartCapturerInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_READ] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->ReadInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_STOP_CAPTURER] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->StopCapturerInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_GET_WAKEUP_PCM] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->GetWakeupPcmInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_EVALUATE] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->EvaluateInner(data, reply); };
    processFuncMap_[INTELL_VOICE_ENGINE_NOTIFY_DEVICE_WAKEUP_EVENT] = [this](MessageParcel &data,
        MessageParcel &reply) -> int32_t { return this->NotifyDeviceWakeupEventInner(data, reply); };
}

IntellVoiceEngineStub::~IntellVoiceEngineStub()
{
    processFuncMap_.clear();
}

int32_t IntellVoiceEngineStub::OnRemoteRequest(uint32_t code,
    MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (data.ReadInterfaceToken() != IIntellVoiceEngine::GetDescriptor()) {
        INTELL_VOICE_LOG_ERROR("token mismatch");
        return -1;
    }

    auto it = processFuncMap_.find(code);
    if ((it != processFuncMap_.end()) && (it->second != nullptr)) {
        return it->second(data, reply);
    }

    INTELL_VOICE_LOG_WARN("invalid code:%{public}u", code);
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

int32_t IntellVoiceEngineStub::SetCallbackInner(MessageParcel &data, MessageParcel & /* reply */)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    SetCallback(object);
    return 0;
}

int32_t IntellVoiceEngineStub::AttachInner(MessageParcel &data, MessageParcel &reply)
{
    IntellVoiceEngineInfo info = {
        .wakeupPhrase = data.ReadString(),
        .isPcmFromExternal = data.ReadBool(),
        .minBufSize = data.ReadInt32(),
        .sampleChannels = data.ReadInt32(),
        .bitsPerSample = data.ReadInt32(),
        .sampleRate = data.ReadInt32(),
    };

    int32_t ret = Attach(info);
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::DetachInner(MessageParcel & /* data */, MessageParcel &reply)
{
    int32_t ret = Detach();
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::SetParameterInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = SetParameter(data.ReadString());
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::GetParameterInner(MessageParcel &data, MessageParcel &reply)
{
    std::string value = GetParameter(data.ReadString());
    reply.WriteString(value);
    return 0;
}

int32_t IntellVoiceEngineStub::StartInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = Start(data.ReadBool());
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::StopInner(MessageParcel & /* data */, MessageParcel &reply)
{
    int32_t ret = Stop();
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::WriteAudioInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t size = data.ReadInt32();
    if (size <= 0) {
        INTELL_VOICE_LOG_ERROR("size(%{public}d) is invalid", size);
        return -1;
    }
    const uint8_t *buffer = data.ReadBuffer(size);
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is nullptr");
        return -1;
    }

    int32_t ret = WriteAudio(buffer, size);
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::StartCapturerInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = StartCapturer(data.ReadInt32());
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::ReadInner(MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint8_t> pcmData;
    int32_t ret = Read(pcmData);
    reply.WriteInt32(ret);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to read wakeup pcm, ret:%{public}d", ret);
        return ret;
    }

    reply.WriteUint32(pcmData.size());
    reply.WriteBuffer(pcmData.data(), pcmData.size());
    return ret;
}

int32_t IntellVoiceEngineStub::StopCapturerInner(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = StopCapturer();
    reply.WriteInt32(ret);
    return ret;
}

int32_t IntellVoiceEngineStub::GetWakeupPcmInner(MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint8_t> pcmData;
    int32_t ret = GetWakeupPcm(pcmData);
    reply.WriteInt32(ret);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to get cloud pcm, ret:%{public}d", ret);
        return ret;
    }

    reply.WriteUint32(pcmData.size());
    reply.WriteBuffer(pcmData.data(), pcmData.size());
    return ret;
}

int32_t IntellVoiceEngineStub::EvaluateInner(MessageParcel &data, MessageParcel &reply)
{
    EvaluationResult result;
    int32_t ret = Evaluate(data.ReadString(), result);
    reply.WriteInt32(ret);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to evaluate");
        return ret;
    }
    reply.WriteInt32(result.score);
    reply.WriteInt32(result.resultCode);
    return ret;
}

int32_t IntellVoiceEngineStub::NotifyDeviceWakeupEventInner(MessageParcel &data, MessageParcel &reply)
{
    INTELL_VOICE_LOG_INFO("enter");
    std::string macAddress = data.ReadString();
    int32_t eventCode = data.ReadInt32();
    int32_t endFrameIndex = data.ReadInt32();
    if (eventCode >= OHOS::IntellVoiceEngine::DeviceWakeupEvent::EVENT_MAX
        || eventCode < OHOS::IntellVoiceEngine::DeviceWakeupEvent::VOICE_RECOGNIZE_EVENT) {
        INTELL_VOICE_LOG_ERROR("failed to evaluate");
        return 1;
    }
    DeviceWakeupEvent event = static_cast<OHOS::IntellVoiceEngine::DeviceWakeupEvent>(eventCode);
    int32_t ret = NotifyDeviceWakeupEvent(macAddress, {event, endFrameIndex});
 
    reply.WriteInt32(ret);
    return ret;
}
}
}