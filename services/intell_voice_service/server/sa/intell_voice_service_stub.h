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

#ifndef INTELL_VOICE_SERVICE_STUB_H
#define INTELL_VOICE_SERVICE_STUB_H
#include <map>
#include <functional>
#include "iremote_stub.h"
#include "i_intell_voice_service.h"

namespace OHOS {
namespace IntellVoiceEngine {
class IntellVoiceServiceStub : public IRemoteStub<IIntellVoiceService> {
public:
    int32_t OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    IntellVoiceServiceStub();
    ~IntellVoiceServiceStub();

protected:
    virtual bool RegisterDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object) = 0;
    virtual bool DeregisterDeathRecipient(IntellVoiceEngineType type) = 0;

private:
    std::map<uint32_t, std::function<int32_t(MessageParcel &data, MessageParcel &reply)>> processServiceFuncMap_;
    int32_t CreateEngineInner(MessageParcel &data, MessageParcel &reply);
    int32_t ReleaseEngineInner(MessageParcel &data, MessageParcel &reply);
    int32_t SetParameterInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetParameterInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetReportedFilesInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetCloneFileListInner(MessageParcel &data, MessageParcel &reply);
    int32_t GetCloneFileInner(MessageParcel &data, MessageParcel &reply);
    int32_t SendCloneFileInner(MessageParcel &data, MessageParcel &reply);
    int32_t CloneForResultInner(MessageParcel &data, MessageParcel &reply);
    int32_t ClearUserDataInner(MessageParcel &data, MessageParcel &reply);
};
}  // namespace IntellVoiceEngine
}  // namespace OHOS
#endif
