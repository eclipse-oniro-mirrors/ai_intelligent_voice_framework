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

#ifndef INTELL_VOICE_ENGINE_STUB_H
#define INTELL_VOICE_ENGINE_STUB_H
#include <map>
#include <functional>
#include "iremote_stub.h"
#include "i_intell_voice_engine.h"
namespace OHOS {
namespace IntellVoiceEngine {
class IntellVoiceEngineStub : public IRemoteStub<IIntellVoiceEngine> {
public:
    IntellVoiceEngineStub();
    ~IntellVoiceEngineStub();
    int32_t OnRemoteRequest(uint32_t code,
        MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    int32_t SetCallbackInner(MessageParcel &data, MessageParcel &reply);
    int32_t AttachInner(MessageParcel &data, MessageParcel &reply);
    int32_t DetachInner(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameterInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetParameterInner(MessageParcel &data, MessageParcel &reply);
    int32_t StartInner(MessageParcel &data, MessageParcel &reply);
    int32_t StopInner(MessageParcel &data, MessageParcel &reply);
    int32_t WriteAudioInner(MessageParcel &data, MessageParcel &reply);
    int32_t ReadInner(MessageParcel &data, MessageParcel &reply);
    int32_t StartCapturerInner(MessageParcel &data, MessageParcel &reply);
    int32_t StopCapturerInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetWakeupPcmInner(MessageParcel &data, MessageParcel &reply);
    int32_t EvaluateInner(MessageParcel &data, MessageParcel &reply);
    int32_t NotifyDeviceWakeupEventInner(MessageParcel &data, MessageParcel &reply);

    std::map<uint32_t, std::function<int32_t(MessageParcel &data, MessageParcel &reply)>> processFuncMap_;
};
}
}
#endif
