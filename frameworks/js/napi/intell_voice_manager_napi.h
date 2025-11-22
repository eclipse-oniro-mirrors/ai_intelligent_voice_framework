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

#ifndef INTELL_VOICE_MANAGER_NAPI_H
#define INTELL_VOICE_MANAGER_NAPI_H

#include <functional>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "service_change_callback_napi.h"
#include "intell_voice_manager.h"
#include "intell_voice_napi_queue.h"
#include "intell_voice_update_callback_napi.h"

namespace OHOS {
namespace IntellVoiceNapi {
using OHOS::IntellVoice::IntellVoiceManager;

class IntellVoiceManagerNapi {
public:
    IntellVoiceManagerNapi();
    ~IntellVoiceManagerNapi();

    static napi_value Export(napi_env env, napi_value exports);

private:
    static void Destruct(napi_env env, void *nativeObject, void *finalizeHint);
    static napi_value Construct(napi_env env, napi_callback_info info);

    static napi_value GetCapabilityInfo(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value OnServiceChange(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);
    static napi_value OffServiceChange(napi_env env, napi_callback_info info);

    static napi_value GetIntelligentVoiceManager(napi_env env, napi_callback_info info);
    static napi_value GetIntelligentVoiceManagerWrapper(napi_env env);
    template <typename T>
    static napi_value CreatePropertyBase(napi_env env, T &propertyMap, napi_ref ref);
    static napi_value RegisterCallback(napi_env env, napi_value jsThis, napi_value *args);
    static napi_value DeregisterCallback(napi_env env, napi_value jsThis);

private:
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    sptr<ServiceChangeCallbackNapi> serviceChangeCb_ = nullptr;
    IntellVoiceManager *manager_ = nullptr;
    static napi_ref serviceChangeTypeRef_;
    static napi_ref engineTypeRef_;
    static napi_ref sensibilityTypeRef_;
    static napi_ref enrollEventTypeRef_;
    static napi_ref wakeupEventTypeRef_;
    static napi_ref errorCodeRef_;
    static napi_ref enrollResultRef_;
    static napi_ref uploadFileTypeRef_;
    static napi_ref cloneResultRef_;
    static napi_ref evaluationResultCodeRef_;
};
}  // namespace IntellVoiceNapi
}  // namespace OHOS
#endif
