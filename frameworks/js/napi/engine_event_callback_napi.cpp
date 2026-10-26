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
#include "intell_voice_common_napi.h"
#include "scope_guard.h"
#include "v1_2/intell_voice_engine_types.h"

#define LOG_TAG "EngineEventCallbackNapi"

using namespace std;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceEngine;

namespace OHOS {
namespace IntellVoiceNapi {
EngineEventCallbackNapi::EngineEventCallbackNapi(napi_env env) : env_(env)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (env_ != nullptr) {
        napi_get_uv_event_loop(env_, &loop_);
    }
}

void EngineEventCallbackNapi::SaveCallbackReference(napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it : callbackRefSet_) {
        if (IntellVoiceCommonNapi::IsSameCallback(env_, callback, it->ref_)) {
            INTELL_VOICE_LOG_INFO("the callback already exists");
            return;
        }
    }

    napi_ref cbRef = nullptr;
    constexpr int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, callback, refCount, &cbRef);
    CHECK_CONDITION_RETURN_VOID(((status != napi_ok) || (cbRef == nullptr)), "creating reference for callback failed");
    std::shared_ptr<IntellVoiceRef> callbackRef =  std::make_shared<IntellVoiceRef>(env_, cbRef);
    CHECK_CONDITION_RETURN_VOID((callbackRef == nullptr), "create callback ref failed");
    callbackRefSet_.insert(callbackRef);
    INTELL_VOICE_LOG_INFO("save callback reference success, size [%{public}zu]", callbackRefSet_.size());
}

void EngineEventCallbackNapi::RemoveCallbackReference(napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = callbackRefSet_.begin(); it != callbackRefSet_.end(); ++it) {
        if (IntellVoiceCommonNapi::IsSameCallback(env_, callback, (*it)->ref_)) {
            callbackRefSet_.erase(it);
            INTELL_VOICE_LOG_INFO("remove callback reference success, size [%{public}zu]", callbackRefSet_.size());
            return;
        }
    }

    INTELL_VOICE_LOG_ERROR("js callback not find");
}

void EngineEventCallbackNapi::RemoveAllCallbackReference()
{
    std::lock_guard<std::mutex> lock(mutex_);
    callbackRefSet_.clear();
    INTELL_VOICE_LOG_INFO("remove all callback reference");
}

uint32_t EngineEventCallbackNapi::GetCbReferenceSetSize()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return callbackRefSet_.size();
}

void EngineEventCallbackNapi::OnEvent(const IntellVoiceEngineCallBackEvent &event)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    int32_t eventId = -1;
    if (event.msgId == HDI::IntelligentVoice::Engine::V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE) {
        eventId = INTELLIGENT_VOICE_EVENT_RECOGNIZE_COMPLETE;
    } else if (event.msgId == static_cast<OHOS::HDI::IntelligentVoice::Engine::V1_0::IntellVoiceEngineMessageType>(
        OHOS::HDI::IntelligentVoice::Engine::V1_2::INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE)) {
        eventId = INTELLIGENT_VOICE_EVENT_HEADSET_RECOGNIZE_COMPLETE;
    } else {
        INTELL_VOICE_LOG_ERROR("error msgId:%{public}d", event.msgId);
        return;
    }
    EngineCallBackInfo cbInfo = {eventId, event.result == 0 ? true : false, event.info};
    INTELL_VOICE_LOG_INFO("OnEvent EngineCallBackInfo: eventId: %{public}d, isSuccess: %{public}u",
        cbInfo.eventId, cbInfo.isSuccess);

    for (auto it : callbackRefSet_) {
        OnJsCallbackEngineEvent(cbInfo, it);
    }
}

void EngineEventCallbackNapi::OnJsCallbackEngineEvent(const EngineCallBackInfo &cbInfo,
    std::shared_ptr<IntellVoiceRef> cbRef)
{
    CHECK_CONDITION_RETURN_VOID((loop_ == nullptr), "loop is nullptr");
    uv_work_t *work = new (nothrow) uv_work_t;
    CHECK_CONDITION_RETURN_VOID((work == nullptr), "Create uv work failed, no memory");
    work->data = new EngineEventCallbackData { cbInfo, cbRef };
    if (work->data == nullptr) {
        INTELL_VOICE_LOG_ERROR("Create uv work data failed, no memory");
        delete work;
        return;
    }

    int32_t ret = uv_queue_work_with_qos(loop_, work, [](uv_work_t *work) {}, [](uv_work_t *work, int uvStatus) {
        EngineEventCallbackData *callbackData = static_cast<EngineEventCallbackData *>(work->data);
        napi_handle_scope scope = nullptr;
        ON_SCOPE_EXIT {
            if (scope != nullptr && callbackData != nullptr && callbackData->callback != nullptr) {
                napi_close_handle_scope(callbackData->callback->env_, scope);
            }
            delete callbackData;
            delete work;
        };

        CHECK_CONDITION_RETURN_VOID((callbackData == nullptr), "uvCallback is nullptr");
        CHECK_CONDITION_RETURN_VOID((callbackData->callback == nullptr), "uvCallback callback is nullptr");
        napi_env env = callbackData->callback->env_;
        napi_status openStatus = napi_open_handle_scope(env, &scope);
        if (openStatus != napi_ok || scope == nullptr) {
            INTELL_VOICE_LOG_ERROR("open handle scope failed");
            return;
        }
        napi_value jsCallback = callbackData->callback->GetRefValue();
        CHECK_CONDITION_RETURN_VOID((jsCallback == nullptr), "get reference value failed");

        const size_t argc = ARGC_ONE;
        napi_value args[argc] = { GetCallBackInfoNapiValue(env, callbackData->cbInfo) };
        CHECK_CONDITION_RETURN_VOID((args[ARG_INDEX_0] == nullptr), "arg is nullptr");
        napi_value result = nullptr;
        napi_status status = napi_call_function(env, nullptr, jsCallback, argc, args, &result);
        if (status != napi_ok) {
            INTELL_VOICE_LOG_ERROR("failed to call engine event callback, error: %{public}d", status);
        }
    }, uv_qos_default);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to execute libuv work queue");
        delete reinterpret_cast<EngineEventCallbackData*>(work->data);
        delete work;
    }
}

napi_value EngineEventCallbackNapi::GetCallBackInfoNapiValue(napi_env env, const EngineCallBackInfo &callbackInfo)
{
    napi_value result;
    napi_status status = napi_create_object(env, &result);
    if (status != napi_ok || result == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create js callbackInfo, error: %{public}d", status);
        return nullptr;
    }

    napi_set_named_property(env, result, "eventId", SetValue(env, callbackInfo.eventId));
    napi_set_named_property(env, result, "isSuccess", SetValue(env, callbackInfo.isSuccess));
    napi_set_named_property(env, result, "context", SetValue(env, callbackInfo.context));
    return result;
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
