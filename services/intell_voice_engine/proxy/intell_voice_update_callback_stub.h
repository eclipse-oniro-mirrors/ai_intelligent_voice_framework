/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef INTELL_VOICE_UPDATE_CALLBACK_STUB_H
#define INTELL_VOICE_UPDATE_CALLBACK_STUB_H

#include "iremote_stub.h"
#include "i_intell_voice_update_callback.h"

namespace OHOS {
namespace IntellVoice {
using namespace OHOS::IntellVoice;
class IntellVoiceUpdateCallbackStub : public IRemoteStub<IIntelligentVoiceUpdateCallback> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
            MessageParcel &reply, MessageOption &option) override;
private:
    void OnUpdateCompleteCallback(MessageParcel &data);
};
}
}
#endif