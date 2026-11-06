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

#include "intell_voice_napi_queue.h"
#include "intell_voice_common_napi.h"
#include "intell_voice_napi_util.h"
#include "intell_voice_log.h"

#define LOG_TAG "IntellVoiceNapiQueue"

using namespace std;

namespace OHOS {
namespace IntellVoiceNapi {
AsyncContext::AsyncContext(napi_env env) : env_(env)
{
    INTELL_VOICE_LOG_INFO("enter");
}

AsyncContext::~AsyncContext()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (env_ != nullptr) {
        if (work_ != nullptr) {
            napi_delete_async_work(env_, work_);
        }
        if (callbackRef_ != nullptr) {
            napi_delete_reference(env_, callbackRef_);
        }
        env_ = nullptr;
    }
}

bool AsyncContext::GetCbInfo(napi_env env, napi_callback_info info, size_t callBackIndex, CbInfoParser parser)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_status status;
    napi_value jsThis = nullptr;
    const int32_t refCount = 1;
    size_t argc = ARGC_MAX;
    napi_value args[ARGC_MAX] = { nullptr };

    status = napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr);
    CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get cb info");

    status = napi_unwrap(env, jsThis, &instanceNapi_);
    CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get napi instance");

    if (argc > callBackIndex) {
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, args[callBackIndex], &valueType);
        CHECK_CONDITION_RETURN_FALSE((valueType != napi_function), "Failed to get asyncCallback, type mismatch");

        status = napi_create_reference(env, args[callBackIndex], refCount, &callbackRef_);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to create callback reference");
    }

    if (parser) {
        return parser(argc, args);
    }

    return true;
}

napi_value NapiAsync::AsyncWork(napi_env env, shared_ptr<AsyncContext> context, const string &name,
    AsyncExecute execute)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    if (context->callbackRef_ == nullptr) {
        napi_create_promise(env, &context->deferred_, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &resource);
    napi_status status = napi_create_async_work(env, nullptr, resource, execute, AsyncCompleteCallback,
        static_cast<void *>(context.get()), &context->work_);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to create async work");
        result = nullptr;
        return result;
    }

    status = napi_queue_async_work(env, context->work_);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to queue async work");
        result = nullptr;
    } else {
        context->contextSp_ = context;
    }

    return result;
}

napi_value NapiAsync::AsyncWork(napi_env env, shared_ptr<AsyncContext> context, const string &name,
    AsyncExecute execute, napi_async_complete_callback complete)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    if (context->callbackRef_ == nullptr) {
        napi_create_promise(env, &context->deferred_, &result);
    } else {
        napi_get_undefined(env, &result);
    }

    napi_value resource = nullptr;
    napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &resource);
    napi_status status = napi_create_async_work(env, nullptr, resource, execute, complete,
        static_cast<void *>(context.get()), &context->work_);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to create async work");
        result = nullptr;
        return result;
    }

    status = napi_queue_async_work(env, context->work_);
    context->contextSp_ = context;

    return result;
}

void NapiAsync::CommonCallbackRoutine(napi_env env, AsyncContext *context, const napi_value &data)
{
    CHECK_CONDITION_RETURN_VOID((context == nullptr), "async context is null");

    if (context->deferred_) {
        HandlePromise(env, context, data);
    } else {
        HandleAsyncCallback(env, context, data);
    }
    context->contextSp_.reset();
}

void NapiAsync::HandlePromise(napi_env env, AsyncContext *context, const napi_value &data)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_handle_scope scope;
    napi_open_handle_scope(env, &scope);
    napi_value result = nullptr;
    napi_value error = nullptr;
    napi_get_undefined(env, &result);
    napi_get_undefined(env, &error);

    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        result = data;
        napi_resolve_deferred(env, context->deferred_, result);
        napi_close_handle_scope(env, scope);
        return;
    }
    napi_value errMessage = nullptr;
    napi_value errCode = nullptr;
    std::string msg = IntellVoiceCommonNapi::GetMessageByCode(context->result_);
    napi_create_string_utf8(env, msg.c_str(), NAPI_AUTO_LENGTH, &errMessage);
    napi_create_string_utf8(env, std::to_string(context->result_).c_str(), NAPI_AUTO_LENGTH, &errCode);
    napi_create_error(env, errCode, errMessage, &error);
    napi_reject_deferred(env, context->deferred_, error);
    napi_close_handle_scope(env, scope);
}

void NapiAsync::HandleAsyncCallback(napi_env env, AsyncContext *context, const napi_value &data)
{
    napi_handle_scope scope;
    napi_open_handle_scope(env, &scope);
    napi_value args[ARGC_TWO] = {nullptr, nullptr};
    napi_value callback = nullptr;
    napi_value ret = nullptr;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        napi_get_undefined(env, &args[ARG_INDEX_0]);
        args[ARG_INDEX_1] = data;
    } else {
        napi_value errMessage = nullptr;
        napi_value errCode = nullptr;
        std::string msg = IntellVoiceCommonNapi::GetMessageByCode(context->result_);
        napi_create_string_utf8(env, msg.c_str(), NAPI_AUTO_LENGTH, &errMessage);
        napi_create_string_utf8(env, std::to_string(context->result_).c_str(), NAPI_AUTO_LENGTH, &errCode);
        napi_create_error(env, errCode, errMessage, &args[ARG_INDEX_0]);
        napi_get_undefined(env, &args[ARG_INDEX_1]);
    }
    napi_get_reference_value(env, context->callbackRef_, &callback);
    napi_call_function(env, nullptr, callback, ARGC_TWO, args, &ret);
    napi_close_handle_scope(env, scope);
}

void NapiAsync::AsyncCompleteCallback(napi_env env, napi_status status, void *data)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_VOID((data == nullptr), "async complete callback data is null");
    napi_value result = nullptr;
    auto context = static_cast<AsyncContext *>(data);

    if (context->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
        napi_get_undefined(env, &result);
        NapiAsync::CommonCallbackRoutine(env, context, result);
        return;
    }

    if (context->complete_ != nullptr) {
        context->complete_(env, context, result);
    } else {
        napi_get_undefined(env, &result);
    }
    CommonCallbackRoutine(env, context, result);
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
