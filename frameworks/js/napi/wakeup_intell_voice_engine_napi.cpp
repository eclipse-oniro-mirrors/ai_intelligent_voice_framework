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

#include "wakeup_intell_voice_engine_napi.h"

#include "want.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "intell_voice_common_napi.h"
#include "intell_voice_napi_util.h"
#include "intell_voice_napi_queue.h"

#include "intell_voice_log.h"
#include "i_intell_voice_engine_callback.h"

#define LOG_TAG "WakeupIntellVoiceEngineNapi"

using namespace std;
using namespace OHOS::IntellVoice;

namespace OHOS {
namespace IntellVoiceNapi {
static __thread napi_ref g_wakeupEngineConstructor = nullptr;
WakeupIntelligentVoiceEngineDescriptor WakeupIntellVoiceEngineNapi::g_wakeupEngineDesc_;
int32_t WakeupIntellVoiceEngineNapi::constructResult_ = NAPI_INTELLIGENT_VOICE_SUCCESS;
static constexpr uint32_t MAX_CHANNEL_CNT = 4;

static const std::string WAKEUP_ENGINE_NAPI_CLASS_NAME = "WakeupIntelligentVoiceEngine";
static const std::string INTELL_VOICE_EVENT_CALLBACK_NAME = "wakeupIntelligentVoiceEvent";

WakeupIntellVoiceEngineNapi::WakeupIntellVoiceEngineNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
}

WakeupIntellVoiceEngineNapi::~WakeupIntellVoiceEngineNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

napi_value WakeupIntellVoiceEngineNapi::Export(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getSupportedRegions", GetSupportedRegions),
        DECLARE_NAPI_FUNCTION("setSensibility", SetSensibility),
        DECLARE_NAPI_FUNCTION("setWakeupHapInfo", SetWakeupHapInfo),
        DECLARE_NAPI_FUNCTION("setParameter", SetParameter),
        DECLARE_NAPI_FUNCTION("getParameter", GetParameter),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("onWakeupIntelligentVoiceEvent", OnWakeupIntelligentVoiceEvent),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("offWakeupIntelligentVoiceEvent", OffWakeupIntelligentVoiceEvent),
        DECLARE_NAPI_FUNCTION("startCapturer", StartCapturer),
        DECLARE_NAPI_FUNCTION("read", Read),
        DECLARE_NAPI_FUNCTION("stopCapturer", StopCapturer),
        DECLARE_NAPI_FUNCTION("getPcm", GetPcm),
    };

    napi_property_descriptor static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION(
            "createWakeupIntelligentVoiceEngine", CreateWakeupIntelligentVoiceEngine)
    };

    status = napi_define_class(env,
        WAKEUP_ENGINE_NAPI_CLASS_NAME.c_str(),
        NAPI_AUTO_LENGTH,
        Construct,
        nullptr,
        sizeof(properties) / sizeof(properties[0]),
        properties,
        &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_wakeupEngineConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, WAKEUP_ENGINE_NAPI_CLASS_NAME.c_str(), constructor);
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

void WakeupIntellVoiceEngineNapi::Destruct(napi_env env, void *nativeObject, void *finalizeHint)
{
    if (nativeObject != nullptr) {
        auto obj = static_cast<WakeupIntellVoiceEngineNapi *>(nativeObject);
        delete obj;
    }
}

napi_value WakeupIntellVoiceEngineNapi::Construct(napi_env env, napi_callback_info info)
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

    unique_ptr<WakeupIntellVoiceEngineNapi> engineNapi = make_unique<WakeupIntellVoiceEngineNapi>();
    if (engineNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to create WakeupIntellVoiceEngineNapi, No memory");
        constructResult_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        return undefinedResult;
    }

    engineNapi->env_ = env;
    engineNapi->engine_ = std::make_shared<WakeupIntellVoiceEngine>(g_wakeupEngineDesc_);
    if (engineNapi->engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create intell voice engine failed");
        constructResult_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
        return undefinedResult;
    }

    constructResult_ = IntellVoiceCommonNapi::ConvertResultCode(engineNapi->engine_->result_);
    if (constructResult_ != NAPI_INTELLIGENT_VOICE_SUCCESS) {
        return undefinedResult;
    }

    status = napi_wrap(env, jsThis, static_cast<void *>(engineNapi.get()), WakeupIntellVoiceEngineNapi::Destruct,
        nullptr, &(engineNapi->wrapper_));
    if (status == napi_ok) {
        engineNapi.release();
        return jsThis;
    }

    INTELL_VOICE_LOG_ERROR("wrap wakeup intell voice engine failed");
    return undefinedResult;
}

napi_value WakeupIntellVoiceEngineNapi::CreateWakeupIntelligentVoiceEngine(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    auto context = std::make_shared<AsyncContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create context failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        napi_valuetype valueType = napi_undefined;
        napi_typeof(env, argv[0], &valueType);
        CHECK_CONDITION_RETURN_FALSE((valueType != napi_object), "arg type mismatch");
        napi_value temp = nullptr;
        napi_status status = napi_get_named_property(env, argv[0], "needReconfirm", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp, g_wakeupEngineDesc_.needReconfirm);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get need reconfirm failed");
        status = napi_get_named_property(env, argv[0], "wakeupPhrase", &temp);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get property failed");
        status = GetValue(env, temp, g_wakeupEngineDesc_.wakeupPhrase);
        CHECK_CONDITION_RETURN_FALSE((status != napi_ok), "get wakeup phrase failed");
        INTELL_VOICE_LOG_INFO("needReconfirm: %{public}d", g_wakeupEngineDesc_.needReconfirm);
        INTELL_VOICE_LOG_DEBUG("wakeupPhrase: %{public}s", g_wakeupEngineDesc_.wakeupPhrase.c_str());
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {};

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        napi_value constructor = nullptr;
        napi_status status = napi_get_reference_value(env, g_wakeupEngineConstructor, &constructor);
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

    return NapiAsync::AsyncWork(env, context, "CreateWakeupIntelligentVoiceEngine", execute);
}

napi_value WakeupIntellVoiceEngineNapi::GetSupportedRegions(napi_env env, napi_callback_info info)
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

napi_value WakeupIntellVoiceEngineNapi::SetParameter(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_2;
    class SetParameterContext : public AsyncContext {
    public:
        explicit SetParameterContext(napi_env napiEnv) : AsyncContext(napiEnv) {};
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
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, argv[ARG_INDEX_0], context->key) != napi_ok), "Failed to get key");
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, argv[ARG_INDEX_1], context->value) != napi_ok),
            "Failed to get value");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<SetParameterContext *>(data);
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            engine->SetParameter(asyncContext->key, asyncContext->value);
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }
    return NapiAsync::AsyncWork(env, context, "SetParameter", execute);
}

napi_value WakeupIntellVoiceEngineNapi::GetParameter(napi_env env, napi_callback_info info)
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
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
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

napi_value WakeupIntellVoiceEngineNapi::SetSensibility(napi_env env, napi_callback_info info)
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
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, argv[0], context->sensibility) != napi_ok),
            "Failed to get sensibility");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {
        CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
        auto asyncContext = static_cast<SetSensibilityContext *>(data);
        CHECK_CONDITION_RETURN_VOID((asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS), "no need to execute");
        auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
        CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
        engine->SetSensibility(asyncContext->sensibility);
    };

    return NapiAsync::AsyncWork(env, context, "SetSensibility", execute);
}

napi_value WakeupIntellVoiceEngineNapi::SetWakeupHapInfo(napi_env env, napi_callback_info info)
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
        CHECK_CONDITION_RETURN_FALSE((napi_get_named_property(env, argv[0], "bundleName", &temp) != napi_ok),
            "get property failed");
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, temp, context->hapInfo.bundleName) != napi_ok),
            "get bundle name failed");
        CHECK_CONDITION_RETURN_FALSE((napi_get_named_property(env, argv[0], "abilityName", &temp) != napi_ok),
            "get property failed");
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, temp, context->hapInfo.abilityName) != napi_ok),
            "get ability name failed");
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute = [](napi_env env, void *data) {
        CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
        auto asyncContext = static_cast<SetWakeupHapContext *>(data);
        CHECK_CONDITION_RETURN_VOID((asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS), "no need to execute");
        auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
        CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
        engine->SetWakeupHapInfo(asyncContext->hapInfo);
    };

    return NapiAsync::AsyncWork(env, context, "SetWakeupHapInfo", execute);
}

napi_value WakeupIntellVoiceEngineNapi::Release(napi_env env, napi_callback_info info)
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

    AsyncExecute execute = [](napi_env env, void *data) {
        CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
        auto asyncContext = static_cast<AsyncContext *>(data);
        CHECK_CONDITION_RETURN_VOID((asyncContext->result_ != NAPI_INTELLIGENT_VOICE_SUCCESS), "no need to execute");
        auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
        CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
        engine->Release();
        reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_ = nullptr;
    };

    return NapiAsync::AsyncWork(env, context, "Release", execute);
}

napi_value WakeupIntellVoiceEngineNapi::On(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = ARGC_TWO;
    napi_value args[ARGC_TWO] = { nullptr, nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount != ARGC_TWO) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }

    napi_valuetype eventType = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_0], &eventType) != napi_ok || eventType != napi_string) {
        INTELL_VOICE_LOG_ERROR("callback event name type mismatch");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }

    string callbackName = "";
    status = GetValue(env, args[ARG_INDEX_0], callbackName);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to get callbackName");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }
    INTELL_VOICE_LOG_INFO("callbackName: %{public}s", callbackName.c_str());

    if (callbackName != INTELL_VOICE_EVENT_CALLBACK_NAME) {
        INTELL_VOICE_LOG_ERROR("no such supported event");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }

    napi_valuetype handler = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_1], &handler) != napi_ok || handler != napi_function) {
        INTELL_VOICE_LOG_ERROR("callback handler type mismatch");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }

    return RegisterCallback(env, jsThis, args);
}

napi_value WakeupIntellVoiceEngineNapi::OnWakeupIntelligentVoiceEvent(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    size_t argCount = ARGC_ONE;
    napi_value args[ARGC_ONE] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount != ARGC_ONE) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }

    napi_valuetype handler = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_0], &handler) != napi_ok || handler != napi_function) {
        INTELL_VOICE_LOG_ERROR("callback handler type mismatch");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_INVALID_PARAM);
        return undefinedResult;
    }
    napi_value args1[ARGC_TWO] = { nullptr, args[ARG_INDEX_0] };

    return RegisterCallback(env, jsThis, args1);
}

napi_value WakeupIntellVoiceEngineNapi::RegisterCallback(napi_env env, napi_value jsThis, napi_value *args)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    WakeupIntellVoiceEngineNapi *engineNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&engineNapi));
    if (status != napi_ok || engineNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to get engine napi instance");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR);
        return result;
    }

    if (engineNapi->engine_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("engine is null");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_PERMISSION_DENIED);
        return result;
    }

    engineNapi->callbackNapi_ = std::make_shared<EngineEventCallbackNapi>(env);
    if (engineNapi->callbackNapi_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("allocate callback napi failed");
        IntellVoiceCommonNapi::ThrowError(env, NAPI_INTELLIGENT_VOICE_NO_MEMORY);
        return result;
    }
    engineNapi->callbackNapi_->SaveCallbackReference(args[ARG_INDEX_1]);
    engineNapi->engine_->SetCallback(engineNapi->callbackNapi_);
    INTELL_VOICE_LOG_INFO("Set callback finish");
    return result;
}

napi_value WakeupIntellVoiceEngineNapi::Off(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t minArgCount = ARGC_ONE;
    size_t argCount = ARGC_TWO;
    napi_value args[ARGC_TWO] = { nullptr, nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < minArgCount) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        return undefinedResult;
    }

    napi_valuetype eventType = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_0], &eventType) != napi_ok || eventType != napi_string) {
        INTELL_VOICE_LOG_ERROR("callback event name type mismatch");
        return undefinedResult;
    }

    string callbackName = "";
    status = GetValue(env, args[ARG_INDEX_0], callbackName);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to get callbackName");
        return undefinedResult;
    }
    INTELL_VOICE_LOG_INFO("callbackName: %{public}s", callbackName.c_str());

    if (argCount > minArgCount) {
        napi_valuetype secondArgType = napi_undefined;
        if (napi_typeof(env, args[ARG_INDEX_1], &secondArgType) != napi_ok || secondArgType != napi_function) {
            INTELL_VOICE_LOG_ERROR("failed to get callback function instance");
            return undefinedResult;
        }
    } else {
        args[ARG_INDEX_1] = nullptr;
    }

    return UnregisterCallback(env, jsThis, callbackName, args[ARG_INDEX_1]);
}

napi_value WakeupIntellVoiceEngineNapi::OffWakeupIntelligentVoiceEvent(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t maxArgCount = ARGC_ONE;
    size_t argCount = ARGC_ONE;
    napi_value args[ARGC_ONE] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount > maxArgCount) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        return undefinedResult;
    }

    if (argCount == ARGC_ONE) {
        napi_valuetype secondArgType = napi_undefined;
        if (napi_typeof(env, args[ARG_INDEX_0], &secondArgType) != napi_ok || secondArgType != napi_function) {
            INTELL_VOICE_LOG_ERROR("failed to get callback function instance");
            return undefinedResult;
        }
    }

    return UnregisterCallback(env, jsThis, INTELL_VOICE_EVENT_CALLBACK_NAME, args[ARG_INDEX_0]);
}

napi_value WakeupIntellVoiceEngineNapi::UnregisterCallback(napi_env env, napi_value jsThis,
    const std::string &callbackName, napi_value callback)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    WakeupIntellVoiceEngineNapi *engineNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&engineNapi));
    if (status != napi_ok || engineNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to get engine napi instance");
        return result;
    }

    if (callbackName != INTELL_VOICE_EVENT_CALLBACK_NAME) {
        INTELL_VOICE_LOG_ERROR("No such off callback supported");
        return result;
    }

    if (engineNapi->callbackNapi_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("callback napi is nullptr");
        return result;
    }

    if (callback != nullptr) {
        engineNapi->callbackNapi_->RemoveCallbackReference(callback);
        if (engineNapi->callbackNapi_->GetCbReferenceSetSize() == 0) {
            engineNapi->callbackNapi_ = nullptr;
        }
    } else {
        engineNapi->callbackNapi_->RemoveAllCallbackReference();
        engineNapi->callbackNapi_ = nullptr;
    }
    return result;
}

napi_value WakeupIntellVoiceEngineNapi::StartCapturer(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    size_t cbIndex = ARG_INDEX_1;
    class StartCapturerContext : public AsyncContext {
    public:
        explicit StartCapturerContext(napi_env napiEnv) : AsyncContext(napiEnv) {};
        int32_t channels = 0;
    };
    auto context = make_shared<StartCapturerContext>(env);
    if (context == nullptr) {
        INTELL_VOICE_LOG_ERROR("create StartCapturerContext failed, No memory");
        return nullptr;
    }

    CbInfoParser parser = [env, context](size_t argc, napi_value *argv) -> bool {
        CHECK_CONDITION_RETURN_FALSE((argc < ARGC_ONE), "argc less than 1");
        CHECK_CONDITION_RETURN_FALSE((GetValue(env, argv[0], context->channels) != napi_ok),
            "Failed to get channels");
        if ((context->channels <= 0) || (context->channels >= (0x1 << MAX_CHANNEL_CNT))) {
            INTELL_VOICE_LOG_ERROR("channels:%{public}d is invalid", context->channels);
            return false;
        }
        return true;
    };

    context->result_ = (context->GetCbInfo(env, info, cbIndex, parser) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<StartCapturerContext *>(data);
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            if (engine == nullptr) {
                INTELL_VOICE_LOG_ERROR("get engine instance failed");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_START_CAPTURER_FAILED;
                return;
            }
            asyncContext->result_ = IntellVoiceCommonNapi::ConvertResultCode(
                engine->StartCapturer(asyncContext->channels));
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "StartCapturer", execute);
}

napi_value WakeupIntellVoiceEngineNapi::Read(napi_env env, napi_callback_info info)
{
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    class GetAudioContext : public AsyncContext {
    public:
        explicit GetAudioContext(napi_env napiEnv) : AsyncContext(napiEnv) {};
        std::vector<uint8_t> data;
    };

    shared_ptr<GetAudioContext> context = make_shared<GetAudioContext>(env);
    CHECK_CONDITION_RETURN_RET(context == nullptr, undefined, "create context fail");

    context->result_ = (context->GetCbInfo(env, info, 0, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<GetAudioContext *>(data);
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            if (engine == nullptr) {
                INTELL_VOICE_LOG_ERROR("get engine instance failed");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_READ_FAILED;
                return;
            }
            if (engine->Read(asyncContext->data) != 0) {
                INTELL_VOICE_LOG_ERROR("failed to read");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_READ_FAILED;
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        CHECK_CONDITION_RETURN_VOID((asyncContext == nullptr), "async context is null");
        auto context = static_cast<GetAudioContext *>(asyncContext);
        result = SetValue(env, context->data);
        vector<uint8_t>().swap(context->data);
    };

    return NapiAsync::AsyncWork(env, context, "Read", execute);
}

napi_value WakeupIntellVoiceEngineNapi::StopCapturer(napi_env env, napi_callback_info info)
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
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            CHECK_CONDITION_RETURN_VOID((engine == nullptr), "get engine instance failed");
            engine->StopCapturer();
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    return NapiAsync::AsyncWork(env, context, "StopCapturer", execute);
}

napi_value WakeupIntellVoiceEngineNapi::GetPcm(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    class GetPcmContext : public AsyncContext {
    public:
        explicit GetPcmContext(napi_env env) : AsyncContext(env) {};
        std::vector<uint8_t> data;
    };

    auto context = make_shared<GetPcmContext>(env);
    CHECK_CONDITION_RETURN_RET(context == nullptr, undefined, "create context fail");

    context->result_ = (context->GetCbInfo(env, info, 0, nullptr) ? NAPI_INTELLIGENT_VOICE_SUCCESS :
        NAPI_INTELLIGENT_VOICE_INVALID_PARAM);

    AsyncExecute execute;
    if (context->result_ == NAPI_INTELLIGENT_VOICE_SUCCESS) {
        execute = [](napi_env env, void *data) {
            CHECK_CONDITION_RETURN_VOID((data == nullptr), "data is nullptr");
            auto asyncContext = static_cast<GetPcmContext *>(data);
            auto engine = reinterpret_cast<WakeupIntellVoiceEngineNapi *>(asyncContext->instanceNapi_)->engine_;
            if (engine == nullptr) {
                INTELL_VOICE_LOG_ERROR("get engine instance failed");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_NO_MEMORY;
                return;
            }
            if (engine->GetWakeupPcm(asyncContext->data) != 0) {
                INTELL_VOICE_LOG_ERROR("failed to get wakeup pcm");
                asyncContext->result_ = NAPI_INTELLIGENT_VOICE_SYSTEM_ERROR;
            }
        };
    } else {
        execute = [](napi_env env, void *data) {};
    }

    context->complete_ = [](napi_env env, AsyncContext *asyncContext, napi_value &result) {
        CHECK_CONDITION_RETURN_VOID((asyncContext == nullptr), "async context is null");
        auto context = static_cast<GetPcmContext *>(asyncContext);
        result = SetValue(env, context->data);
        vector<uint8_t>().swap(context->data);
    };

    return NapiAsync::AsyncWork(env, context, "GetPcm", execute);
}

}  // namespace IntellVoiceNapi
}  // namespace OHOS
