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

#include "wakeup_manager_napi.h"

#include "intell_voice_common_napi.h"
#include "intell_voice_napi_util.h"
#include "intell_voice_napi_queue.h"
#include "intell_voice_update_callback_napi.h"

#include "intell_voice_log.h"

#define LOG_TAG "WakeupManagerNapi"

using namespace std;
using namespace OHOS::IntellVoice;

namespace OHOS {
namespace IntellVoiceNapi {
static constexpr int32_t UPLOAD_NUM_MAX = 100;

class WakeupSourceFilesContext : public AsyncContext {
public:
    explicit WakeupSourceFilesContext(napi_env env) : AsyncContext(env) {};
    std::vector<WakeupSourceFile> cloneFile;
};

class UploadFilesContext : public AsyncContext {
public:
    explicit UploadFilesContext(napi_env env) : AsyncContext(env) {};
    int numMax = 0;
    std::vector<OHOS::IntellVoice::UploadFilesInfo> uploadFiles;
};

static __thread napi_ref g_wakeupManagerConstructor = nullptr;
int32_t WakeupManagerNapi::constructResult_ = NAPI_INTELLIGENT_VOICE_SUCCESS;

static const std::string WAKEUP_MANAGER_NAPI_CLASS_NAME = "WakeupManager";

WakeupManagerNapi::WakeupManagerNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
}

WakeupManagerNapi::~WakeupManagerNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
    callbackNapi_ = nullptr;
}

napi_value WakeupManagerNapi::Export(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getParameter", GetParameter),
        DECLARE_NAPI_FUNCTION("getUploadFiles", GetUploadFiles),
        DECLARE_NAPI_FUNCTION("getWakeupSourceFiles", GetWakeupSourceFiles),
        DECLARE_NAPI_FUNCTION("enrollWithWakeupFilesForResult", EnrollWithWakeupFilesForResult),
        DECLARE_NAPI_FUNCTION("clearUserData", ClearUserData),
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION(
            "getWakeupManager", GetWakeupManager)
    };

    status = napi_define_class(env,
        WAKEUP_MANAGER_NAPI_CLASS_NAME.c_str(),
        NAPI_AUTO_LENGTH,
        Construct,
        nullptr,
        sizeof(properties) / sizeof(properties[0]),
        properties,
        &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_wakeupManagerConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, WAKEUP_MANAGER_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            status = napi_define_properties(env, exports,
                                            sizeof(static_prop) / sizeof(static_prop[0]), static_prop);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    return result;
}

void WakeupManagerNapi::Destruct(napi_env env, void *nativeObject, void *finalizeHint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<WakeupManagerNapi *>(nativeObject);
        delete obj;
    }
}

napi_value WakeupManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_status status;
    size_t argc = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    status = napi_get_cb_info(env, info, &argc, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("invalid number of arguments");
        return undefinedResult;
    }

    unique_ptr<WakeupManagerNapi> wakeupMangerNapi = make_unique<WakeupManagerNapi>();
    if (wakeupMangerNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to create WakeupManagerNapi, No memory");
        return undefinedResult;
    }

    wakeupMangerNapi->env_ = env;
    status = napi_wrap(env, jsThis, static_cast<void *>(wakeupMangerNapi.get()), WakeupManagerNapi::Destruct,
        nullptr, &(wakeupMangerNapi->wrapper_));
    if (status == napi_ok) {
        wakeupMangerNapi.release();
        return jsThis;
    }

    INTELL_VOICE_LOG_ERROR("wrap wakeup intell voice engine failed");
    return undefinedResult;
}

napi_value WakeupManagerNapi::GetWakeupManager(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    if (!IntellVoiceCommonNapi::CheckIsSystemApp()) {
        INTELL_VOICE_LOG_ERROR("Not system application!");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_NOT_SYSTEM_APPLICATION);
        return nullptr;
    }

    napi_status status;
    size_t argCount = 0;

    status = napi_get_cb_info(env, info, &argCount, nullptr, nullptr, nullptr);
    if (status != napi_ok || argCount != 0) {
        INTELL_VOICE_LOG_ERROR("Invalid arguments");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return nullptr;
    }

    return WakeupManagerNapi::GetWakeupManagerWrapper(env);
}

napi_value WakeupManagerNapi::GetWakeupManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, g_wakeupManagerConstructor, &constructor);
    if (status == napi_ok) {
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        if (status == napi_ok) {
            return result;
        }
    }

    INTELL_VOICE_LOG_ERROR("failed to get intell voice manager wrapper");
    IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_NO_MEMORY);
    napi_get_undefined(env, &result);

    return result;
}

napi_value WakeupManagerNapi::SetParameter(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_2;
    class SetParameterContext : public AsyncContext {
    public:
        explicit SetParameterContext(napi_env env): AsyncContext(env) {};
        string key;
        string value;
    };
    shared_ptr<SetParameterContext> context = make_shared<SetParameterContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create set parameter aync context failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_TWO), "argc less than 2");
        napi_status status = GetValue(env, argv[0], context->key);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get key");
        status = GetValue(env, argv[1], context->value);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get value");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto setParamContext = reinterpret_cast<SetParameterContext *>(data);
            IntellVoiceManager *manager = IntellVoiceManager::GetInstance();
            if (manager == nullptr) {
                INTELL_VOICE_LOG_ERROR("manager is nullptr");
                setParamContext->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
                return;
            }
            setParamContext->result_ = IntellVoiceCommonNapi::ConvertResultCode(
                manager->SetParameter(setParamContext->key, setParamContext->value));
            if (setParamContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
                INTELL_VOICE_LOG_ERROR("set parameter failed, ret:%{public}d", setParamContext->result_);
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "SetParameter", execute);
}

napi_value WakeupManagerNapi::GetParameter(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    class GetParamContext : public AsyncContext {
    public:
        explicit GetParamContext(napi_env env) : AsyncContext(env) {};
        std::string key;
        std::string val;
    };
    auto context = make_shared<GetParamContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create AsyncContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_status status = GetValue(env, argv[0], context->key);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get key");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = reinterpret_cast<GetParamContext *>(data);
            IntellVoiceManager *manager = IntellVoiceManager::GetInstance();
            if (manager == nullptr) {
                INTELL_VOICE_LOG_ERROR("manager is nullptr");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
                return;
            }
            asyncContext->val = manager->GetParameter(asyncContext->key);
        };

        context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
            auto getParamAsynContext = reinterpret_cast<GetParamContext *>(asyncContext);
            if (getParamAsynContext != nullptr) {
                result = SetValue(env, getParamAsynContext->val);
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "GetParameter", execute);
}

napi_value WakeupManagerNapi::GetUploadFiles(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    auto context = std::make_shared<UploadFilesContext>(env);
    CHECK_CONDITION_RETURN_RET(context == nullptr, nullptr, "create upload files context failed");

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) {
        napi_status status = GetValue(env, argv[ARG_INDEX_0], context->numMax);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get numMax");
        if ((context->numMax <= 0) || (context->numMax > UPLOAD_NUM_MAX)) {
            INTELL_VOICE_LOG_ERROR("numMax:%{public}d is invalid", context->numMax);
            return false;
        }
        return true;
    };
    context->result_ = (context->GetCbInfo(env, info, ARG_INDEX_1, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            auto *context = static_cast<UploadFilesContext *>(data);
            IntellVoiceManager *manager = IntellVoiceManager::GetInstance();
            if (manager == nullptr) {
                context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
                return;
            }

            std::vector<UploadFilesInfo>().swap(context->uploadFiles);
            context->result_ = IntellVoiceCommonNapi::ConvertResultCode(
                manager->GetUploadFiles(context->numMax, context->uploadFiles));
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        GetUploadFilesComplete(env, asyncContext, result);
    };
    return NapiAsync::AsyncWork(env, context, "GetUploadFiles", execute);
}

void WakeupManagerNapi::GetUploadFilesComplete(napi_env env, AsyncContext *data, napi_value &result)
{
    INTELL_VOICE_LOG_INFO("enter");
    auto *context = reinterpret_cast<UploadFilesContext *>(data);
    napi_get_undefined(env, &result);
    std::vector<UploadFilesInfo> &uploadFiles = context->uploadFiles;

    if (uploadFiles.size() == 0) {
        context->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
        goto EXIT;
    }

    if (napi_create_array(env, &result) != napi_ok) {
        context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        goto EXIT;
    }

    for (uint32_t index = 0; index < uploadFiles.size(); index++) {
        napi_value obj;
        if (napi_create_object(env, &obj) != napi_ok) {
            INTELL_VOICE_LOG_ERROR("napi_create_object failed");
            context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
            goto EXIT;
        }

        napi_set_named_property(env, obj, "type", SetValue(env, uploadFiles[index].type));
        napi_set_named_property(env, obj, "filesDescription", SetValue(env, uploadFiles[index].filesDescription));
        napi_value arrayContents = nullptr;
        if (napi_create_array(env, &arrayContents) != napi_ok) {
            context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
            goto EXIT;
        }

        for (uint32_t j = 0; j < uploadFiles[index].filesContent.size(); j++) {
            napi_value content = SetValue(env, uploadFiles[index].filesContent[j]);
            if (content == nullptr) {
                context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
                goto EXIT;
            }
            napi_set_element(env, arrayContents, j, content);
        }
        napi_set_named_property(env, obj, "filesContent", arrayContents);
        napi_set_element(env, result, index, obj);
    }

EXIT:
    std::vector<UploadFilesInfo>().swap(uploadFiles);
}

napi_value WakeupManagerNapi::GetWakeupSourceFiles(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;

    auto context = std::make_shared<WakeupSourceFilesContext>(env);
    CHECK_CONDITION_RETURN_RET(context == nullptr, nullptr, "create clone file context failed");

    context->result_ = (context->GetCbInfo(env, info, cbIndex, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            auto *context = static_cast<WakeupSourceFilesContext *>(data);
            IntellVoiceManager *manager = IntellVoiceManager::GetInstance();
            if (manager == nullptr) {
                context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
                return;
            }

            context->result_ = IntellVoiceCommonNapi::ConvertResultCode(
                manager->GetWakeupSourceFiles(context->cloneFile));
            if (context->result_ != 0) {
                INTELL_VOICE_LOG_ERROR("get clone files error, ret:%{public}d", context->result_);
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        GetCloneCompleteCallback(env, asyncContext, result);
    };
    return NapiAsync::AsyncWork(env, context, "GetWakeupSourceFiles", execute);
}

void WakeupManagerNapi::GetCloneCompleteCallback(napi_env env, AsyncContext *data, napi_value &result)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_VOID(data == nullptr, "get clone file context null");
    auto *context = reinterpret_cast<WakeupSourceFilesContext *>(data);
    napi_get_undefined(env, &result);

    std::vector<WakeupSourceFile> &files = context->cloneFile;
    if (files.size() == 0) {
        context->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
        goto EXIT;
    }

    if (napi_create_array(env, &result) != napi_ok) {
        context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        goto EXIT;
    }

    for (uint32_t filesIndex = 0; filesIndex < files.size(); filesIndex++) {
        napi_value obj;
        if (napi_create_object(env, &obj) != napi_ok) {
            context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
            goto EXIT;
        }
        napi_set_named_property(env, obj, "filePath", SetValue(env, files[filesIndex].filePath));
        napi_set_named_property(env, obj, "fileContent", SetValue(env, files[filesIndex].fileContent));
        napi_set_element(env, result, filesIndex, obj);
    }

EXIT:
    vector<WakeupSourceFile>().swap(files);
}

void WakeupManagerNapi::CloneForResultCompleteCallback(napi_env env, napi_status status, void *data)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_VOID((data == nullptr), "async complete callback data is nullptr");
    napi_value result = nullptr;

    auto asyncContext = static_cast<UpdateAsyncContext *>(data);
    if (asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
        napi_get_undefined(env, &result);
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    if (asyncContext->processWork == nullptr) {
        INTELL_VOICE_LOG_ERROR("process work is nullptr");
        napi_get_undefined(env, &result);
        asyncContext->result_ = NAPI_INTELLIGENT_VOICE_INVALID_PARAM;
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    auto cb = reinterpret_cast<WakeupManagerNapi *>(asyncContext->instanceNapi_)->callbackNapi_;
    if (cb == nullptr) {
        INTELL_VOICE_LOG_ERROR("get callback napi failed");
        napi_get_undefined(env, &result);
        asyncContext->result_ = NAPI_INTELLIGENT_VOICE_INVALID_PARAM;
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    cb->QueueAsyncWork(asyncContext);
    if (asyncContext->processWork(asyncContext) != 0) {
        INTELL_VOICE_LOG_ERROR("clone process work failed");
        cb->ClearAsyncWork(true, "the request was aborted because intelligent voice processWork error");
    }
}

bool WakeupManagerNapi::WakeupFilesForResultParser(std::shared_ptr<UpdateAsyncContext> context,
    napi_env env, size_t argc, napi_value *argv)
{
    CHECK_CONDITION_RETURN_FALSE((argc < ARGC_TWO), "argc less than 2");

    bool isArray = false;
    napi_status status = napi_is_array(env, argv[0], &isArray);
    CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
    CHECK_CONDITION_RETURN_FALSE((!isArray), "argv 0 is not array");

    uint32_t arrayLength = 0;
    status = napi_get_array_length(env, argv[0], &arrayLength);
    CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get array length failed");
    CHECK_CONDITION_RETURN_FALSE((arrayLength == 0), "array length is 0");

    napi_value value = nullptr;
    napi_value tempResult = nullptr;

    context->cloneFiles.clear();
    context->cloneFiles.reserve(arrayLength);
    for (size_t index = 0; index < arrayLength; index++) {
        status = napi_get_element(env, argv[0], index, &value);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get array element failed");
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, value, &valueType);
        CHECK_CONDITION_RETURN_FALSE((valueType != napi_object), "get array element object mismatch");

        WakeupSourceFile fileInfo;
        status = napi_get_named_property(env, value, "filePath", &tempResult);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, tempResult, fileInfo.filePath);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get filePath failed");

        status = napi_get_named_property(env, value, "fileContent", &tempResult);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, tempResult, fileInfo.fileContent);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get fileContent failed");
        context->cloneFiles.push_back(fileInfo);
    }

    status = GetValue(env, argv[1], context->wakeupInfo);
    CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get clone info failed");
    return true;
}

napi_value WakeupManagerNapi::EnrollWithWakeupFilesForResult(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_2;
    auto context = make_shared<UpdateAsyncContext>(env);
    CHECK_CONDITION_RETURN_RET(context == nullptr, nullptr, "create context failed, no memory");
    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        return WakeupFilesForResultParser(context, env, argc, argv);
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        reinterpret_cast<WakeupManagerNapi *>(context->instanceNapi_)->callbackNapi_ =
            std::make_shared<IntellVoiceUpdateCallbackNapi>(env);
    }

    context->result = EnrollResult::UNKNOWN_ERROR;
    context->processWork = [&](AsyncContext *asyncContext) -> int32_t {
        auto context = reinterpret_cast<UpdateAsyncContext *>(asyncContext);
        auto wakeupManagerNapi = reinterpret_cast<WakeupManagerNapi *>(context->instanceNapi_);
        auto manager = IntellVoiceManager::GetInstance();
        if (wakeupManagerNapi == nullptr) {
            INTELL_VOICE_LOG_ERROR("get manager instance failed");
            goto PROCESS_ERR_EXIT;
        }
        if (manager == nullptr) {
            INTELL_VOICE_LOG_ERROR("manager null");
            goto PROCESS_ERR_EXIT;
        }

        if (wakeupManagerNapi->callbackNapi_ == nullptr) {
            INTELL_VOICE_LOG_ERROR("callback napi null");
            goto PROCESS_ERR_EXIT;
        }

        if (manager->EnrollWithWakeupFilesForResult(context->cloneFiles, context->wakeupInfo,
            wakeupManagerNapi->callbackNapi_) != 0) {
            INTELL_VOICE_LOG_ERROR("clone for result failed");
            wakeupManagerNapi->callbackNapi_ = nullptr;
            goto PROCESS_ERR_EXIT;
        }

        context->cloneFiles.clear();
        return 0;

PROCESS_ERR_EXIT:
        context->cloneFiles.clear();
        return -1;
    };

    AsyncExecute execute = [](napi_env env, void *data) {};

    return NapiAsync::AsyncWork(env, context, "EnrollWithWakeupFilesForResult", execute,
        CloneForResultCompleteCallback);
}

napi_value WakeupManagerNapi::ClearUserData(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;
    auto context = std::make_shared<AsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create AsyncContext failed, No memory");
        return nullptr;
    }
    context->result_ = (context->GetCbInfo(env, info, cbIndex, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<AsyncContext *>(data);
            IntellVoiceManager *manager = IntellVoiceManager::GetInstance();
            if (manager == nullptr) {
                INTELL_VOICE_LOG_ERROR("manager is nullptr");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
                return;
            }
            asyncContext->result_ = IntellVoiceCommonNapi::ConvertResultCode(
                manager->ClearUserData());
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "ClearUserData", execute);
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
