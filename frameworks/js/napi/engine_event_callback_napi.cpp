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
#include "engine_event_callback_napi.h"
#include <uv.h>
#include "intell_voice_info.h"
#include "intell_voice_log.h"

#define LOG_TAG "EngineEventCallbackNapi"

using namespace std;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceEngine;

namespace OHOS {
namespace IntellVoiceNapi {
EngineEventCallbackNapi::EngineEventCallbackNapi(napi_env env, napi_value callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    env_ = env;
    if (env_ != nullptr) {
        napi_get_uv_event_loop(env_, &loop_);
    }
    callbackRef_ = make_shared<IntellVoiceRef>(env_, callback);
}

EngineEventCallbackNapi::~EngineEventCallbackNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
    callbackRef_ = nullptr;
}

void EngineEventCallbackNapi::OnEvent(const IntellVoiceEngineCallBackEvent &event)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (callbackRef_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to get engine event callback");
        return;
    }

    if (event.msgId != HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        INTELL_VOICE_LOG_ERROR("error msgId");
        return;
    }

    EngineCallBackInfo cbInfo = {INTELLIGENT_VOICE_EVENT_RECOGNIZE_COMPLETE,
        event.result == 0 ? true : false, event.info};

    INTELL_VOICE_LOG_INFO("OnEvent EngineCallBackInfo: eventId: %{public}d, isSuccess: %{public}u, context: %{public}s",
        cbInfo.eventId,
        cbInfo.isSuccess,
        cbInfo.context.c_str());

    return OnEventUvCallback(cbInfo);
}

void EngineEventCallbackNapi::OnEventUvCallback(EngineCallBackInfo &cbInfo)
{
    CHECK_RETURN_VOID(loop_ != nullptr, "loop is nullptr");
    uv_work_t *work = new (nothrow) uv_work_t;
    CHECK_RETURN_VOID(work != nullptr, "Create uv work failed, no memory");

    work->data = new EngineEventUvCallback {env_, cbInfo, callbackRef_};

    uv_queue_work(
        loop_,
        work,
        [](uv_work_t *work) {},
        [](uv_work_t *work, int uvStatus) {
            shared_ptr<EngineEventUvCallback> uvCallback(
                static_cast<EngineEventUvCallback *>(work->data), [work](EngineEventUvCallback *data) {
                    delete data;
                    delete work;
                });
            CHECK_RETURN_VOID(uvCallback != nullptr, "uvCallback is nullptr");
            CHECK_RETURN_VOID(uvCallback->callback != nullptr, "uvCallback callback is nullptr");
            napi_env env = uvCallback->env_;
            INTELL_VOICE_LOG_INFO("uv_queue_work start");

            napi_value jsCallback = uvCallback->callback->GetRefValue();
            if (jsCallback == nullptr) {
                INTELL_VOICE_LOG_ERROR("get reference value fail");
                return;
            }

            const size_t argc = 1;
            napi_value args[argc] = {nullptr};
            GetJsCallbackInfo(env, uvCallback->cbInfo, args[0]);
            if (args[0] == nullptr) {
                INTELL_VOICE_LOG_ERROR("failed to create engine event callback");
                return;
            }

            napi_value result = nullptr;
            napi_status status = napi_call_function(env, nullptr, jsCallback, argc, args, &result);
            if (status != napi_ok) {
                INTELL_VOICE_LOG_ERROR("failed to call engine event callback, error: %{public}d", status);
            }
        });
}

void GetJsCallbackInfo(const napi_env &env, const EngineCallBackInfo &callbackInfo, napi_value &jsObj)
{
    napi_status status = napi_create_object(env, &jsObj);
    if (status != napi_ok || jsObj == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create js callbackInfo, error: %{public}d", status);
        return;
    }

    napi_set_named_property(env, jsObj, "eventId", SetValue(env, callbackInfo.eventId));
    napi_set_named_property(env, jsObj, "isSuccess", SetValue(env, callbackInfo.isSuccess));
    napi_set_named_property(env, jsObj, "context", SetValue(env, callbackInfo.context));
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
