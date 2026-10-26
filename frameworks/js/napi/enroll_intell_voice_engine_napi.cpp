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

#include "enroll_intell_voice_engine_napi.h"

#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "intell_voice_common_napi.h"
#include "intell_voice_napi_util.h"
#include "intell_voice_napi_queue.h"
#include "intell_voice_log.h"
#include "i_intell_voice_engine_callback.h"

#define LOG_TAG "EnrollEngineNapi"

using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceEngine;
using namespace std;

namespace OHOS {
namespace IntellVoiceNapi {
class EvaluateContext : public AsyncContext {
public:
    explicit EvaluateContext(napi_env env) : AsyncContext(env) {};
    string word;
    EvaluationResult info;
};

static __thread napi_ref g_enrollEngineConstructor = nullptr;
EnrollIntelligentVoiceEngineDescriptor EnrollIntellVoiceEngineNapi::g_enrollEngineDesc_;
int32_t EnrollIntellVoiceEngineNapi::constructResult_ = NAPI_INTELLIGENT_VOICE_SUCCESS;

static const std::string ENROLL_ENGINE_NAPI_CLASS_NAME = "EnrollIntelligentVoiceEngine";

EnrollIntellVoiceEngineNapi::EnrollIntellVoiceEngineNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
}

EnrollIntellVoiceEngineNapi::~EnrollIntellVoiceEngineNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

napi_value EnrollIntellVoiceEngineNapi::Export(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getSupportedRegions", GetSupportedRegions),
        DECLARE_NAPI_FUNCTION("init", Init),
        DECLARE_NAPI_FUNCTION("enrollForResult", EnrollForResult),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("setSensibility", SetSensibility),
        DECLARE_NAPI_FUNCTION("setWakeupHapInfo", SetWakeupHapInfo),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getParameter", GetParameter),
        DECLARE_NAPI_FUNCTION("evaluateForResult", EvaluateForResult),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("commit", Commit),
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION(
            "createEnrollIntelligentVoiceEngine", CreateEnrollIntelligentVoiceEngine)
    };

    status = napi_define_class(env,
        ENROLL_ENGINE_NAPI_CLASS_NAME.c_str(),
        NAPI_AUTO_LENGTH,
        Construct,
        nullptr,
        sizeof(properties) / sizeof(properties[0]),
        properties,
        &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_enrollEngineConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, ENROLL_ENGINE_NAPI_CLASS_NAME.c_str(), constructor);
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

void EnrollIntellVoiceEngineNapi::Destruct(napi_env env, void *nativeObject, void *finalizeHint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<EnrollIntellVoiceEngineNapi *>(nativeObject);
        delete obj;
    }
}

napi_value EnrollIntellVoiceEngineNapi::Construct(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_status status;
    size_t argc = ARGC_ONE;
    napi_value args[ARGC_ONE] = { nullptr };
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    constructResult_ = NAPI_INTELLIGENT_VOICE_SUCCESS;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &jsThis, nullptr));

    unique_ptr<EnrollIntellVoiceEngineNapi> engineNapi = make_unique<EnrollIntellVoiceEngineNapi>();
    if (engineNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to create EnrollIntellVoiceEngineNapi, No memory");
        constructResult_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        return undefinedResult;
    }

    engineNapi->env_ = env;
    engineNapi->engine_ = std::make_shared<EnrollIntellVoiceEngine>(g_enrollEngineDesc_);
    if (engineNapi->engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create intell voice engine failed");
        constructResult_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        return undefinedResult;
    }

    constructResult_ = IntellVoiceCommonNapi::ConvertResultCode(engineNapi->engine_->result_);
    if (constructResult_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
        return undefinedResult;
    }

    engineNapi->callbackNapi_ =  std::make_shared<EnrollIntellVoiceEngineCallbackNapi>(env);
    if (engineNapi->callbackNapi_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create intell voice engine callback napi failed");
        constructResult_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        return undefinedResult;
    }

    if (engineNapi->engine_->SetCallback(engineNapi->callbackNapi_) != 0) {
        INTELL_VOICE_LOG_ERROR("set callback failed");
        constructResult_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
        return undefinedResult;
    }

    status = napi_wrap(env, jsThis, static_cast<void *>(engineNapi.get()),
        EnrollIntellVoiceEngineNapi::Destruct, nullptr, &(engineNapi->wrapper_));
    if (status == napi_ok) {
        engineNapi.release();
        return jsThis;
    }

    INTELL_VOICE_LOG_ERROR("wrap enroll intell voice engine failed");
    return undefinedResult;
}

napi_value EnrollIntellVoiceEngineNapi::CreateEnrollIntelligentVoiceEngine(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;

    auto context = make_shared<AsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create Context failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[0], &valueType);
        CHECK_CONDITION_RETURN_FALSE((valueType != napi_object), "arg type mismatch");
        napi_value temp = nullptr;
        napi_status status = napi_get_named_property(env, argv[0], "wakeupPhrase", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp, g_enrollEngineDesc_.wakeupPhrase);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get wakeup phrase failed");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {};

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        napi_value constructor = nullptr;
        napi_status status = napi_get_reference_value(env, g_enrollEngineConstructor, &constructor);
        if (status != napi_ok) {
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
            napi_get_undefined(env, &result);
            return;
        }

        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        asyncContext->result_ = constructResult_;
        if (status != napi_ok) {
            napi_get_undefined(env, &result);
        }
    };

    return NapiAsync::AsyncWork(env, context, "CreateEnrollIntelligentVoiceEngine", execute);
}

napi_value EnrollIntellVoiceEngineNapi::GetSupportedRegions(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;
    shared_ptr<AsyncContext> context = make_shared<AsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create AsyncContext failed, No memory");
        return nullptr;
    }

    context->result_ = (context->GetCbInfo(env, info, cbIndex, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {};

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        napi_create_array(env, &result);
        napi_set_element(env, result, 0, SetValue(env, "CN"));
    };

    return NapiAsync::AsyncWork(env, context, "GetSupportedRegions", execute);
}

napi_value EnrollIntellVoiceEngineNapi::Init(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    auto context = make_shared<EnrollAsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create InitContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[0], &valueType);
        CHECK_CONDITION_RETURN_FALSE((valueType != napi_object), "arg type mismatch");
        napi_value temp = nullptr;
        napi_status status = napi_get_named_property(env, argv[0], "language", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp,
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->config_.language);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get language failed");
        status = napi_get_named_property(env, argv[0], "region", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp,
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->config_.region);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get region failed");
        status = napi_get_named_property(env, argv[0], "enrollScenario", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp,
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->config_.enrollScenario);
        if (status != napi_ok) {
            INTELL_VOICE_LOG_WARN("get property enrollScenario failed, using default");
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->
                config_.enrollScenario = "";
        }
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    context->type = ASYNC_WORK_INIT;
    context->callbackInfo.result = UNKNOWN_ERROR;
    context->processWork = [&](AsyncContext *asyncContext) -> int32_t {
        auto engineNapi = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_);
        auto engine = engineNapi->engine_;
        if (engine == nullptr) {
            INTELL_VOICE_LOG_ERROR("get engine instance failed");
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_INIT_FAILED;
            return -1;
        }

        auto ret = engine->Init(engineNapi->config_);
        if (ret != 0) {
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_INIT_FAILED;
        }
        return ret;
    };

    AsyncExecute execute = [](napi_env env, void *data) {};

    return NapiAsync::AsyncWork(env, context, "Init", execute, CompleteCallback);
}

napi_value EnrollIntellVoiceEngineNapi::EnrollForResult(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    auto context = make_shared<EnrollAsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create StartContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_status status = GetValue(env, argv[0],
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->isLast_);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get isLast");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    context->type = ASYNC_WORK_START;
    context->callbackInfo.result = UNKNOWN_ERROR;
    context->processWork = [&](AsyncContext *asyncContext) {
        EnrollIntellVoiceEngineNapi *engineNapi = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(
            asyncContext->instanceNapi_);
        auto engine = engineNapi->engine_;
        if (engine == nullptr) {
            INTELL_VOICE_LOG_ERROR("get engine instance failed");
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_PERMISSION_DENIED;
            return -1;
        }

        auto ret = engine->Start(engineNapi->isLast_);
        asyncContext->result_ = IntellVoiceCommonNapi::ConvertResultCode(ret);
        return ret;
    };

    AsyncExecute execute = [](napi_env env, void *data) {};

    return NapiAsync::AsyncWork(env, context, "Start", execute, CompleteCallback);
}

napi_value EnrollIntellVoiceEngineNapi::Stop(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;
    auto context = make_shared<AsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create AsyncContext failed, No memory");
        return nullptr;
    }

    context->result_ = (context->GetCbInfo(env, info, cbIndex, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    if (context->instanceNapi_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("instanceNapi_ is nullptr");
        context->result_ = NAPI_INTELLIGENT_VOICE_INVALID_PARAM;
    } else {
        auto cb = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->callbackNapi_;
        if (cb != nullptr) {
            cb->ClearAsyncWork(false, "the requests was aborted because user called stop");
        }
    }

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<AsyncContext *>(data);
            auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            engine->Stop();
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "Stop", execute);
}

napi_value EnrollIntellVoiceEngineNapi::SetParameter(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_2;
    class SetParameterContext : public AsyncContext {
    public:
        explicit SetParameterContext(napi_env napiEnv): AsyncContext(napiEnv) {};
        string key;
        string value;
    };
    auto context = make_shared<SetParameterContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create SetParameterContext failed, No memory");
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
            auto asyncContext = static_cast<SetParameterContext *>(data);
            auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            engine->SetParameter(asyncContext->key, asyncContext->value);
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "SetParameter", execute);
}

napi_value EnrollIntellVoiceEngineNapi::GetParameter(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    class GetParamContext : public AsyncContext {
    public:
        explicit GetParamContext(napi_env env) : AsyncContext(env) {};
        std::string key;
        std::string value;
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
            auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            asyncContext->value = engine->GetParameter(asyncContext->key);
        };
        context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
            auto getParamAsynContext = reinterpret_cast<GetParamContext *>(asyncContext);
            if (getParamAsynContext != nullptr) {
                result = SetValue(env, getParamAsynContext->value);
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "GetParameter", execute);
}

napi_value EnrollIntellVoiceEngineNapi::SetSensibility(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    class SetSensibilityContext : public AsyncContext {
    public:
        explicit SetSensibilityContext(napi_env napiEnv) : AsyncContext(napiEnv) {};
        int32_t sensibility = 1;
    };
    auto context = make_shared<SetSensibilityContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create SetSensibilityContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_status status = GetValue(env, argv[0], context->sensibility);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "Failed to get sensibility");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {
        CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
        auto asyncContext = static_cast<SetSensibilityContext *>(data);
        CHECK_CONDITION_RETURN_VOID((asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS), "no need to execute");
        auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
        CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
        engine->SetSensibility(asyncContext->sensibility);
    };

    return NapiAsync::AsyncWork(env, context, "SetSensibility", execute);
}

napi_value EnrollIntellVoiceEngineNapi::SetWakeupHapInfo(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    class SetWakeupHapContext : public AsyncContext {
    public:
        explicit SetWakeupHapContext(napi_env napiEnv) : AsyncContext(napiEnv) {};
        WakeupHapInfo hapInfo;
    };
    auto context = make_shared<SetWakeupHapContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create SetWakeupHapContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_value temp = nullptr;
        napi_status status = napi_get_named_property(env, argv[0], "bundleName", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp, context->hapInfo.bundleName);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get bundle name failed");
        status = napi_get_named_property(env, argv[0], "abilityName", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp, context->hapInfo.abilityName);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get ability name failed");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {
        CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
        auto asyncContext = static_cast<SetWakeupHapContext *>(data);
        CHECK_CONDITION_RETURN_VOID((asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS), "no need to execute");
        auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
        CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
        engine->SetWakeupHapInfo(asyncContext->hapInfo);
    };

    return NapiAsync::AsyncWork(env, context, "SetWakeupHapInfo", execute);
}

napi_value EnrollIntellVoiceEngineNapi::Commit(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;
    auto context = make_shared<EnrollAsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create IntellVoiceContext failed, No memory");
        return nullptr;
    }

    context->result_ = (context->GetCbInfo(env, info, cbIndex, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    context->type = ASYNC_WORK_COMMIT;
    context->callbackInfo.result = UNKNOWN_ERROR;
    context->processWork = [&](AsyncContext *asyncContext) {
        auto engineNapi = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_);
        auto engine = engineNapi->engine_;
        if (engine == nullptr) {
            INTELL_VOICE_LOG_ERROR("get engine instance failed");
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_COMMIT_ENROLL_FAILED;
            return -1;
        }

        auto ret = engine->Commit();
        if (ret != 0) {
            asyncContext->result_ = NAPI_INTELLIGENT_VOICE_COMMIT_ENROLL_FAILED;
        }
        return ret;
    };

    AsyncExecute execute = [](napi_env env, void *data) {};

    return NapiAsync::AsyncWork(env, context, "Commit", execute, CompleteCallback);
}

napi_value EnrollIntellVoiceEngineNapi::EvaluateForResult(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    auto context = make_shared<EvaluateContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create EvaluateContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, argv[ARG_INDEX_0], context->word) != napi_ok),
            "failed to get word");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto context = static_cast<EvaluateContext *>(data);
            auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(context->instanceNapi_)->engine_;
            if (engine == nullptr) {
                INTELL_VOICE_LOG_ERROR("engine is nullptr");
                context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
                return;
            }
            auto ret = engine->Evaluate(context->word, context->info);
            if (ret != 0) {
                INTELL_VOICE_LOG_ERROR("failed to evaluate");
                context->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        auto context = static_cast<EvaluateContext *>(asyncContext);
        napi_status status = napi_create_object(env, &result);
        if (status != napi_ok || result == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to create object, status: %{public}d", status);
            context->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
            return;
        }
        napi_set_named_property(env, result, "score", SetValue(env, context->info.score));
        napi_set_named_property(env, result, "resultCode", SetValue(env, context->info.resultCode));
    };
    return NapiAsync::AsyncWork(env, context, "Evaluate", execute);
}

napi_value EnrollIntellVoiceEngineNapi::Release(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_0;
    shared_ptr<AsyncContext> context = make_shared<AsyncContext>(env);
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
            auto cb = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->callbackNapi_;
            if (cb != nullptr) {
                cb->ClearAsyncWork(false, "the requests was aborted because user called release");
            }
            auto engine = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            engine->Release();
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_ = nullptr;
            reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->callbackNapi_ = nullptr;
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "Release", execute);
}

void EnrollIntellVoiceEngineNapi::CompleteCallback(napi_env env, napi_status status, void *data)
{
    INTELL_VOICE_LOG_INFO("enter");
    CHECK_CONDITION_RETURN_VOID((data == nullptr), "async complete callback data is nullptr");
    napi_value result = nullptr;

    auto asyncContext = static_cast<EnrollAsyncContext *>(data);
    if (asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
        napi_get_undefined(env, &result);
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    if (asyncContext->processWork == nullptr) {
        INTELL_VOICE_LOG_ERROR("process work is nullptr");
        napi_get_undefined(env, &result);
        asyncContext->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    auto cb = reinterpret_cast<EnrollIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->callbackNapi_;
    if (cb == nullptr) {
        INTELL_VOICE_LOG_ERROR("get callback napi failed");
        napi_get_undefined(env, &result);
        asyncContext->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        NapiAsync::CommonCallbackRoutine(env, asyncContext, result);
        return;
    }

    cb->QueueAsyncWork(asyncContext);
    if (asyncContext->processWork(asyncContext) != 0) {
        INTELL_VOICE_LOG_ERROR("process work failed");
        cb->ClearAsyncWork(true, "the request was aborted because intelligent voice processWork error");
    }
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
