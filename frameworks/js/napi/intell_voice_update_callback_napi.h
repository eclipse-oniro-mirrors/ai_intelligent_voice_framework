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

#ifndef INTELL_VOICEUPDATE_CALLBACK_NAPI_H
#define INTELL_VOICEUPDATE_CALLBACK_NAPI_H

#include <queue>
#include <map>
#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "intell_voice_napi_util.h"
#include "i_intell_voice_update_callback.h"
#include "intell_voice_napi_queue.h"
#include "intell_voice_info.h"

namespace OHOS {
namespace IntellVoiceNapi {
using namespace OHOS::IntellVoice;
using ProcessWorkFunc = std::function<int32_t(AsyncContext *)>;

class UpdateAsyncContext : public AsyncContext {
public:
    explicit UpdateAsyncContext(napi_env env) : AsyncContext(env) {};
    ProcessWorkFunc processWork = nullptr;
    int32_t result = OHOS::IntellVoice::EnrollResult::UNKNOWN_ERROR;
    std::vector <OHOS::IntellVoice::WakeupSourceFile> cloneFiles;
    std::string wakeupInfo;
};

class IntellVoiceUpdateCallbackNapi : public IIntellVoiceUpdateCallback {
public:
    explicit IntellVoiceUpdateCallbackNapi(const napi_env env);
    virtual ~IntellVoiceUpdateCallbackNapi();

    void OnUpdateComplete(const int result) override;
    void QueueAsyncWork(UpdateAsyncContext *context);
    void ClearAsyncWork(bool error, const std::string &msg);

private:
    OHOS::IntellVoice::EnrollResult ToResultEnum(const int result);
    void OnJsCallBack(UpdateAsyncContext *context);
    static void UvWorkCallBack(uv_work_t *work, int status);

private:
    std::mutex mutex_;
    napi_env env_ = nullptr;
    uv_loop_s *loop_ = nullptr;
    std::queue<UpdateAsyncContext *> context_;
};
}  // namespace IntellVoiceNapi
}  // namespace OHOS
#endif
