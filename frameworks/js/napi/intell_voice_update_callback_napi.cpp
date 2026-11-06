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

#include "intell_voice_update_callback_napi.h"
#include "intell_voice_log.h"
#include "intell_voice_common_napi.h"

#define LOG_TAG "UpdateCallbackNapi"

using namespace std;
using OHOS::HDI::IntelligentVoice::Engine::V1_3::IntellVoiceEngineErrors;

namespace OHOS {
namespace IntellVoiceNapi {
IntellVoiceUpdateCallbackNapi::IntellVoiceUpdateCallbackNapi(const napi_env env) : env_(env)
{
    if (env_ != nullptr) {
        napi_get_uv_event_loop(env_, &loop_);
    }
}

IntellVoiceUpdateCallbackNapi::~IntellVoiceUpdateCallbackNapi()
{
}

void IntellVoiceUpdateCallbackNapi::QueueAsyncWork(UpdateAsyncContext *context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    context_.push(context);
}

void IntellVoiceUpdateCallbackNapi::ClearAsyncWork(bool error, const std::string &msg)
{
    INTELL_VOICE_LOG_INFO("%{public}s", msg.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    while (!context_.empty()) {
        UpdateAsyncContext *context = context_.front();
        context_.pop();
        if (error) {
            INTELL_VOICE_LOG_WARN("error occured");
        }
        if (context == nullptr) {
            continue;
        }

        INTELL_VOICE_LOG_WARN("fail occured");
        context->result_ = EnrollResult::UNKNOWN_ERROR;

        OnJsCallBack(context);
    }
}

void IntellVoiceUpdateCallbackNapi::OnUpdateComplete(const int result)
{
    INTELL_VOICE_LOG_INFO("OnUpdateComplete: result: %{public}d", result);
    std::lock_guard<std::mutex> lock(mutex_);
    if (context_.empty()) {
        INTELL_VOICE_LOG_WARN("queue is empty");
        return;
    }

    UpdateAsyncContext *context = context_.front();
    context_.pop();
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("context is nullptr");
        return;
    }
    context->result = ToResultEnum(result);
    OnJsCallBack(context);
}

EnrollResult IntellVoiceUpdateCallbackNapi::ToResultEnum(const int result)
{
    IntellVoiceEngineErrors errorCode = static_cast<IntellVoiceEngineErrors>(result);
    switch (errorCode) {
        case IntellVoiceEngineErrors::INTELL_VOICE_ENGINE_OK:
            return EnrollResult::SUCCESS;
        case IntellVoiceEngineErrors::INTELL_VOICE_ENGINE_ERROR_OFFSET:
            return EnrollResult::UNKNOWN_ERROR;
        case IntellVoiceEngineErrors::INTELL_VOICE_ENGINE_ENROLL_VPR_TRAIN_FAILED:
            return EnrollResult::VPR_TRAIN_FAILED;
        case IntellVoiceEngineErrors::INTELL_VOICE_ENGINE_WAKEUP_PHRASE_NOT_MATCH:
            return EnrollResult::WAKEUP_PHRASE_NOT_MATCH;
        case IntellVoiceEngineErrors::INTELL_VOICE_ENGINE_ENROLL_DIFFERENT_PERSON:
            return EnrollResult::DIFFERENT_PERSON;
        default:
            return EnrollResult::UNKNOWN_ERROR;
    }
}

void IntellVoiceUpdateCallbackNapi::UvWorkCallBack(uv_work_t *work, int status)
{
    INTELL_VOICE_LOG_INFO("enter");
    auto asyncContext = reinterpret_cast<UpdateAsyncContext *>(work->data);
    napi_value result = nullptr;
    if (asyncContext != nullptr) {
        napi_env env = asyncContext->env_;
        napi_handle_scope scope = nullptr;
        napi_status openStatus = napi_open_handle_scope(env, &scope);
        if (openStatus != napi_ok || scope == nullptr) {
            INTELL_VOICE_LOG_ERROR("open handle scope failed");
            asyncContext->contextSp_.reset();
            delete work;
            return;
        }
        result = SetValue(env, asyncContext->result);
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        napi_close_handle_scope(env, scope);
    }

    delete work;
}

void IntellVoiceUpdateCallbackNapi::OnJsCallBack(UpdateAsyncContext *context)
{
    INTELL_VOICE_LOG_INFO("enter, result:%{public}d", context->result_);
    if (loop_ != nullptr) {
        uv_work_t *work = new (std::nothrow) uv_work_t;
        if (work != nullptr) {
            work->data = reinterpret_cast<void *>(context);
            int ret = uv_queue_work(
                loop_, work, [](uv_work_t *work) {}, UvWorkCallBack);
            if (ret != 0) {
                INTELL_VOICE_LOG_INFO("Failed to execute libuv work queue");
                context->contextSp_.reset();
                delete work;
            }
        } else {
            context->contextSp_.reset();
        }
    } else {
        context->contextSp_.reset();
    }
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS