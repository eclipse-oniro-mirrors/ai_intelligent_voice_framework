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
#include "service_change_callback_napi.h"

#include <uv.h>
#include "intell_voice_info.h"
#include "intell_voice_log.h"
#include "intell_voice_napi_util.h"
#include "scope_guard.h"
#include "intell_voice_common_napi.h"


#define LOG_TAG "ServiceChangeCallbackNapi"

using namespace std;
using namespace OHOS::IntellVoice;

namespace OHOS {
namespace IntellVoiceNapi {
ServiceChangeCallbackNapi::ServiceChangeCallbackNapi(napi_env env) : env_(env)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (env_ != nullptr) {
        napi_get_uv_event_loop(env_, &loop_);
    }
}

void ServiceChangeCallbackNapi::SaveCallbackReference(napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref cbRef = nullptr;
    constexpr int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, callback, refCount, &cbRef);
    CHECK_CONDITION_RETURN_VOID(((status != napi_ok) || (cbRef == nullptr)), "creating reference for callback failed");
    callbackRef_ =  std::make_shared<IntellVoiceRef>(env_, cbRef);
}

void ServiceChangeCallbackNapi::OnRemoteDied(const wptr <IRemoteObject> &remote)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("receive sa death callback");
    (void)remote;
    int32_t status = ServiceChangeType::SERVICE_UNAVAILABLE;

    OnJsCallbackServiceChange(status);
}

void ServiceChangeCallbackNapi::OnJsCallbackServiceChange(int32_t status)
{
    CHECK_CONDITION_RETURN_VOID((loop_ == nullptr), "loop is nullptr");
    uv_work_t *work = new (std::nothrow) uv_work_t;
    CHECK_CONDITION_RETURN_VOID((work == nullptr), "Create uv work failed, no memory");
    work->data = new ServiceChangeCallbackData { status, callbackRef_ };
    if (work->data == nullptr) {
        INTELL_VOICE_LOG_ERROR("Create uv work data failed, no memory");
        delete work;
        return;
    }

    int32_t ret = uv_queue_work_with_qos(loop_, work, [](uv_work_t *work) {}, [](uv_work_t *work, int uvStatus) {
        ServiceChangeCallbackData *callbackData = reinterpret_cast<ServiceChangeCallbackData *>(work->data);
        napi_handle_scope scope = nullptr;
        ON_SCOPE_EXIT {
            INTELL_VOICE_LOG_INFO("clear momery");
            if (scope != nullptr) {
                napi_close_handle_scope(callbackData->callback->env_, scope);
            }
            delete callbackData;
            delete work;
        };

        CHECK_CONDITION_RETURN_VOID((callbackData->callback == nullptr), "callback is nullptr");
        napi_env env = callbackData->callback->env_;
        napi_status openStatus = napi_open_handle_scope(env, &scope);
        if (openStatus != napi_ok || scope == nullptr) {
            INTELL_VOICE_LOG_ERROR("open handle scope failed");
            return;
        }
        napi_value jsCallback = callbackData->callback->GetRefValue();
        CHECK_CONDITION_RETURN_VOID((jsCallback == nullptr), "callback ref is nullptr");

        const size_t argc = ARGC_ONE;
        napi_value args[argc] = { SetValue(env, callbackData->status) };
        CHECK_CONDITION_RETURN_VOID((args[ARG_INDEX_0] == nullptr), "arg is nullptr");

        INTELL_VOICE_LOG_INFO("uv_queue_work start");
        napi_value result = nullptr;
        napi_status status = napi_call_function(env, nullptr, jsCallback, argc, args, &result);
        if (status != napi_ok) {
            INTELL_VOICE_LOG_ERROR("failed to call engine event callback, error: %{public}d", status);
        }
    }, uv_qos_default);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("failed to execute libuv work queue");
        delete reinterpret_cast<ServiceChangeCallbackData *>(work->data);
        delete work;
    }
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
