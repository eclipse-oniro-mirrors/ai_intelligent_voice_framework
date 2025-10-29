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
#ifndef WAKEUP_INTELL_VOICE_ENGINE_NAPI_H
#define WAKEUP_INTELL_VOICE_ENGINE_NAPI_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "engine_event_callback_napi.h"
#include "wakeup_intell_voice_engine.h"

namespace OHOS {
namespace IntellVoiceNapi {
using OHOS::IntellVoice::WakeupIntellVoiceEngine;
using OHOS::IntellVoice::WakeupIntelligentVoiceEngineDescriptor;

class WakeupIntellVoiceEngineNapi {
public:
    WakeupIntellVoiceEngineNapi();
    ~WakeupIntellVoiceEngineNapi();

    static napi_value Export(napi_env env, napi_value exports);

private:
    static void Destruct(napi_env env, void *nativeObject, void *finalizeHint);
    static napi_value Construct(napi_env env, napi_callback_info info);
    static napi_value CreateWakeupIntelligentVoiceEngine(napi_env env, napi_callback_info info);

    static napi_value GetSupportedRegions(napi_env env, napi_callback_info info);
    static napi_value SetSensibility(napi_env env, napi_callback_info info);
    static napi_value SetWakeupHapInfo(napi_env env, napi_callback_info info);
    static napi_value SetParameter(napi_env env, napi_callback_info info);
    static napi_value GetParameter(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value OnWakeupIntelligentVoiceEvent(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);
    static napi_value OffWakeupIntelligentVoiceEvent(napi_env env, napi_callback_info info);
    static napi_value StartCapturer(napi_env env, napi_callback_info info);
    static napi_value Read(napi_env env, napi_callback_info info);
    static napi_value StopCapturer(napi_env env, napi_callback_info info);
    static napi_value GetPcm(napi_env env, napi_callback_info info);

    static napi_value RegisterCallback(napi_env env, napi_value jsThis, napi_value *args);
    static napi_value UnregisterCallback(napi_env env, napi_value jsThis, const std::string &callbackName,
        napi_value callback);

private:
    napi_env env_ = nullptr;
    napi_ref wrapper_ = nullptr;
    static WakeupIntelligentVoiceEngineDescriptor g_wakeupEngineDesc_;
    static int32_t constructResult_;
    std::shared_ptr<WakeupIntellVoiceEngine> engine_ = nullptr;
    std::shared_ptr<EngineEventCallbackNapi> callbackNapi_ = nullptr;
};
}  // namespace IntellVoiceNapi
}  // namespace OHOS

#endif
