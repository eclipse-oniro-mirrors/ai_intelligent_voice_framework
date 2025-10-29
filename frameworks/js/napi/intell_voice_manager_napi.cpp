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
#include "intell_voice_manager_napi.h"
#include "intell_voice_log.h"
#include "intell_voice_common_napi.h"
#include "intell_voice_napi_util.h"
#include "enroll_intell_voice_engine_napi.h"
#include "wakeup_intell_voice_engine_napi.h"
#include "wakeup_manager_napi.h"
#include "intell_voice_update_callback_napi.h"
#include "intell_voice_info.h"

#define LOG_TAG "IntellVoiceManagerNapi"

using namespace std;
using namespace OHOS::IntellVoice;

namespace OHOS {
namespace IntellVoiceNapi {
static const std::string INTELLIGENT_VOICE_MANAGER_NAPI_CLASS_NAME = "IntelligentVoiceManager";
static const std::string SERVICE_CHANGE_CALLBACK_NAME = "serviceChange";

static __thread napi_ref g_managerConstructor = nullptr;
napi_ref IntellVoiceManagerNapi::serviceChangeTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::engineTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::sensibilityTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::enrollEventTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::wakeupEventTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::errorCodeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::enrollResultRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::uploadFileTypeRef_ = nullptr;
napi_ref IntellVoiceManagerNapi::evaluationResultCodeRef_ = nullptr;

static const std::map<std::string, OHOS::IntellVoice::ServiceChangeType> SERVICE_CHANGE_TYPE_MAP = {
    {"SERVICE_UNAVAILABLE", SERVICE_UNAVAILABLE},
};

static const std::map<std::string, OHOS::IntellVoice::IntelligentVoiceEngineType> ENGINE_TYPE_MAP = {
    {"ENROLL_ENGINE_TYPE", ENROLL_ENGINE_TYPE},
    {"WAKEUP_ENGINE_TYPE", WAKEUP_ENGINE_TYPE},
    {"UPDATE_ENGINE_TYPE", UPDATE_ENGINE_TYPE},
};

static const std::map<std::string, OHOS::IntellVoice::SensibilityType> SENSIBILITY_TYPE_MAP = {
    {"LOW_SENSIBILITY", LOW_SENSIBILITY},
    {"MIDDLE_SENSIBILITY", MIDDLE_SENSIBILITY},
    {"HIGH_SENSIBILITY", HIGH_SENSIBILITY},
};

static const std::map<std::string, OHOS::IntellVoice::EnrollIntelligentVoiceEventType> ENROLL_EVENT_TYPE_MAP = {
    {"INTELLIGENT_VOICE_EVENT_ENROLL_NONE", INTELLIGENT_VOICE_EVENT_ENROLL_NONE},
    {"INTELLIGENT_VOICE_EVENT_ENROLL_INIT_DONE", INTELLIGENT_VOICE_EVENT_ENROLL_INIT_DONE},
    {"INTELLIGENT_VOICE_EVENT_ENROLL_COMPLETE", INTELLIGENT_VOICE_EVENT_ENROLL_COMPLETE},
    {"INTELLIGENT_VOICE_EVENT_COMMIT_ENROLL_COMPLETE", INTELLIGENT_VOICE_EVENT_COMMIT_ENROLL_COMPLETE},
};

static const std::map<std::string, OHOS::IntellVoice::WakeupIntelligentVoiceEventType> WAKEUP_EVENT_TYPE_MAP = {
    {"INTELLIGENT_VOICE_EVENT_WAKEUP_NONE", INTELLIGENT_VOICE_EVENT_WAKEUP_NONE},
    {"INTELLIGENT_VOICE_EVENT_RECOGNIZE_COMPLETE", INTELLIGENT_VOICE_EVENT_RECOGNIZE_COMPLETE},
    {"INTELLIGENT_VOICE_EVENT_HEADSET_RECOGNIZE_COMPLETE", INTELLIGENT_VOICE_EVENT_HEADSET_RECOGNIZE_COMPLETE},
};

static const std::map<std::string, OHOS::IntellVoice::IntelligentVoiceErrorCode> ERROR_CODE_MAP = {
    {"INTELLIGENT_VOICE_NO_MEMORY", INTELLIGENT_VOICE_NO_MEMORY},
    {"INTELLIGENT_VOICE_INVALID_PARAM", INTELLIGENT_VOICE_INVALID_PARAM},
    {"INTELLIGENT_VOICE_INIT_FAILED", INTELLIGENT_VOICE_INIT_FAILED},
    {"INTELLIGENT_VOICE_COMMIT_ENROLL_FAILED", INTELLIGENT_VOICE_COMMIT_ENROLL_FAILED},
    {"INTELLIGENT_VOICE_START_CAPTURER_FAILED", INTELLIGENT_VOICE_START_CAPTURER_FAILED},
    {"INTELLIGENT_VOICE_READ_FAILED", INTELLIGENT_VOICE_READ_FAILED},
    {"INTELLIGENT_VOICE_SYSTEM_ERROR", INTELLIGENT_VOICE_SYSTEM_ERROR},
};

static const std::map<std::string, OHOS::IntellVoice::EnrollResult> ENROLL_RESULT_MAP = {
    {"SUCCESS", OHOS::IntellVoice::EnrollResult::SUCCESS},
    {"VPR_TRAIN_FAILED", OHOS::IntellVoice::EnrollResult::VPR_TRAIN_FAILED},
    {"WAKEUP_PHRASE_NOT_MATCH", OHOS::IntellVoice::EnrollResult::WAKEUP_PHRASE_NOT_MATCH},
    {"TOO_NOISY", OHOS::IntellVoice::EnrollResult::TOO_NOISY},
    {"TOO_LOUD", OHOS::IntellVoice::EnrollResult::TOO_LOUD},
    {"INTERVAL_LARGE", OHOS::IntellVoice::EnrollResult::INTERVAL_LARGE},
    {"DIFFERENT_PERSON", OHOS::IntellVoice::EnrollResult::DIFFERENT_PERSON},
    {"UNKNOWN_ERROR", OHOS::IntellVoice::EnrollResult::UNKNOWN_ERROR},
};

static const std::map<std::string, OHOS::IntellVoice::UploadFileType> UPLOAD_FILE_TYPE_MAP = {
    {"ENROLL_FILE", OHOS::IntellVoice::UploadFileType::ENROLL_FILE},
    {"WAKEUP_FILE", OHOS::IntellVoice::UploadFileType::WAKEUP_FILE},
};

static const std::map<std::string, OHOS::IntellVoice::EvaluationResultCode> EVALUATION_RESULT_CODE_MAP = {
    {"UNKNOWN", OHOS::IntellVoice::UNKNOWN},
    {"PASS", OHOS::IntellVoice::PASS},
    {"WORD_EMPTY", OHOS::IntellVoice::WORD_EMPTY},
    {"CHINESE_ONLY", OHOS::IntellVoice::CHINESE_ONLY},
    {"INVALID_LENGTH", OHOS::IntellVoice::INVALID_LENGTH},
    {"UNUSUAL_WORD", OHOS::IntellVoice::UNUSUAL_WORD},
    {"CONSECUTIVE_SAME_WORD", OHOS::IntellVoice::CONSECUTIVE_SAME_WORD},
    {"TOO_FEW_PHONEMES", OHOS::IntellVoice::TOO_FEW_PHONEMES},
    {"TOO_MANY_PHONEMES", OHOS::IntellVoice::TOO_MANY_PHONEMES},
    {"COMMON_INSTRUCTION", OHOS::IntellVoice::COMMON_INSTRUCTION},
    {"COMMON_SPOKEN_LANGUAGE", OHOS::IntellVoice::COMMON_SPOKEN_LANGUAGE},
    {"SENSITIVE_WORD", OHOS::IntellVoice::SENSITIVE_WORD},
    {"NO_INITIAL_CONSONANT", OHOS::IntellVoice::NO_INITIAL_CONSONANT},
    {"REPEATED_PHONEME", OHOS::IntellVoice::REPEATED_PHONEME},
};

IntellVoiceManagerNapi::IntellVoiceManagerNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
}

IntellVoiceManagerNapi::~IntellVoiceManagerNapi()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (wrapper_ != nullptr) {
        napi_delete_reference(env_, wrapper_);
    }
}

void IntellVoiceManagerNapi::Destruct(napi_env env, void *nativeObject, void *finalizeHint)
{
    INTELL_VOICE_LOG_INFO("enter");
    if (nativeObject != nullptr) {
        auto obj = static_cast<IntellVoiceManagerNapi *>(nativeObject);
        delete obj;
    }
}

napi_value IntellVoiceManagerNapi::Construct(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_status status;
    size_t argCount = 0;
    napi_value jsThis = nullptr;
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("invalid number of arguments");
        return undefinedResult;
    }

    unique_ptr<IntellVoiceManagerNapi> managerNapi = make_unique<IntellVoiceManagerNapi>();
    if (managerNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("no memory");
        return undefinedResult;
    }
    managerNapi->env_ = env;
    managerNapi->manager_ = IntellVoiceManager::GetInstance();
    if (managerNapi->manager_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("create native manager failed");
        return undefinedResult;
    }

    status = napi_wrap(env,
        jsThis,
        static_cast<void *>(managerNapi.get()),
        IntellVoiceManagerNapi::Destruct,
        nullptr,
        &(managerNapi->wrapper_));
    if (status == napi_ok) {
        managerNapi.release();
        return jsThis;
    }

    INTELL_VOICE_LOG_ERROR("failed to construct intell voice manager napi");
    return undefinedResult;
}

napi_value IntellVoiceManagerNapi::Export(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value constructor;
    napi_value result = nullptr;
    const int32_t refCount = 1;
    napi_get_undefined(env, &result);

    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("getCapabilityInfo", GetCapabilityInfo),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("onServiceChange", OnServiceChange),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("offServiceChange", OffServiceChange),
    };

    napi_property_descriptor staticProperties[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getIntelligentVoiceManager", GetIntelligentVoiceManager),
        DECLARE_NAPI_PROPERTY("ServiceChangeType",
            CreatePropertyBase(env, SERVICE_CHANGE_TYPE_MAP, serviceChangeTypeRef_)),
        DECLARE_NAPI_PROPERTY("IntelligentVoiceEngineType", CreatePropertyBase(env, ENGINE_TYPE_MAP, engineTypeRef_)),
        DECLARE_NAPI_PROPERTY("SensibilityType", CreatePropertyBase(env, SENSIBILITY_TYPE_MAP, sensibilityTypeRef_)),
        DECLARE_NAPI_PROPERTY("EnrollIntelligentVoiceEventType",
            CreatePropertyBase(env, ENROLL_EVENT_TYPE_MAP, enrollEventTypeRef_)),
        DECLARE_NAPI_PROPERTY("WakeupIntelligentVoiceEventType",
            CreatePropertyBase(env, WAKEUP_EVENT_TYPE_MAP, wakeupEventTypeRef_)),
        DECLARE_NAPI_PROPERTY("IntelligentVoiceErrorCode", CreatePropertyBase(env, ERROR_CODE_MAP, errorCodeRef_)),
        DECLARE_NAPI_PROPERTY("EnrollResult", CreatePropertyBase(env, ENROLL_RESULT_MAP, enrollResultRef_)),
        DECLARE_NAPI_PROPERTY("UploadFileType", CreatePropertyBase(env, UPLOAD_FILE_TYPE_MAP, uploadFileTypeRef_)),
        DECLARE_NAPI_PROPERTY("EvaluationResultCode",
            CreatePropertyBase(env, EVALUATION_RESULT_CODE_MAP, evaluationResultCodeRef_)),
    };

    status = napi_define_class(env, INTELLIGENT_VOICE_MANAGER_NAPI_CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Construct,
        nullptr, sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    if (status != napi_ok) {
        return result;
    }

    status = napi_create_reference(env, constructor, refCount, &g_managerConstructor);
    if (status == napi_ok) {
        status = napi_set_named_property(env, exports, INTELLIGENT_VOICE_MANAGER_NAPI_CLASS_NAME.c_str(), constructor);
        if (status == napi_ok) {
            status = napi_define_properties(
                env, exports, sizeof(staticProperties) / sizeof(staticProperties[0]), staticProperties);
            if (status == napi_ok) {
                return exports;
            }
        }
    }

    INTELL_VOICE_LOG_ERROR("failed to export intelligent voice manager napi");
    return result;
}

napi_value IntellVoiceManagerNapi::GetCapabilityInfo(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    napi_create_array(env, &result);
    napi_set_element(env, result, 0, SetValue(env, ENROLL_ENGINE_TYPE));
    napi_set_element(env, result, 1, SetValue(env, WAKEUP_ENGINE_TYPE));
    return result;
}

napi_value IntellVoiceManagerNapi::On(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t expectArgCount = ARGC_TWO;
    size_t argCount = ARGC_TWO;
    napi_value args[expectArgCount] = { nullptr, nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount != expectArgCount) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        return undefinedResult;
    }

    napi_valuetype eventType = napi_undefined;
    if (napi_typeof(env, args[0], &eventType) != napi_ok || eventType != napi_string) {
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
    if (callbackName != SERVICE_CHANGE_CALLBACK_NAME) {
        INTELL_VOICE_LOG_ERROR("No such off callback supported");
        return undefinedResult;
    }

    napi_valuetype handler = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_1], &handler) != napi_ok || handler != napi_function) {
        INTELL_VOICE_LOG_ERROR("callback handler type mismatch");
        return undefinedResult;
    }

    return RegisterCallback(env, jsThis, args);
}

napi_value IntellVoiceManagerNapi::OnServiceChange(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");

    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t expectArgCount = ARGC_ONE;
    size_t argCount = ARGC_ONE;
    napi_value args[expectArgCount] = { nullptr };
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount != expectArgCount) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        return undefinedResult;
    }

    napi_valuetype handler = napi_undefined;
    if (napi_typeof(env, args[ARG_INDEX_0], &handler) != napi_ok || handler != napi_function) {
        INTELL_VOICE_LOG_ERROR("callback handler type mismatch");
        return undefinedResult;
    }
    napi_value args1[ARGC_TWO] = { nullptr, args[ARG_INDEX_0] };

    return RegisterCallback(env, jsThis, args1);
}

napi_value IntellVoiceManagerNapi::RegisterCallback(napi_env env, napi_value jsThis, napi_value *args)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    IntellVoiceManagerNapi *managerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&managerNapi));
    if (status != napi_ok || managerNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to get engine napi instance");
        return result;
    }

    managerNapi->serviceChangeCb_ = new (std::nothrow) ServiceChangeCallbackNapi(env);
    if (managerNapi->serviceChangeCb_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("allocate service change callback failed");
        return result;
    }

    managerNapi->serviceChangeCb_->SaveCallbackReference(args[ARG_INDEX_1]);
    auto mgr = IntellVoiceManager::GetInstance();
    if (mgr != nullptr) {
        mgr->RegisterServiceDeathRecipient(managerNapi->serviceChangeCb_);
    }

    return result;
}

napi_value IntellVoiceManagerNapi::Off(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t expectArgCount = ARGC_TWO;
    size_t argCount = ARGC_TWO;
    napi_value args[expectArgCount] = {0};
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount < ARGC_ONE || argCount > ARGC_TWO) {
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
    if (callbackName != SERVICE_CHANGE_CALLBACK_NAME) {
        INTELL_VOICE_LOG_ERROR("No such off callback supported");
        return undefinedResult;
    }

    return DeregisterCallback(env, jsThis);
}

napi_value IntellVoiceManagerNapi::OffServiceChange(napi_env env, napi_callback_info info)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value undefinedResult = nullptr;
    napi_get_undefined(env, &undefinedResult);

    const size_t expectArgCount = ARGC_ONE;
    size_t argCount = ARGC_ONE;
    napi_value args[expectArgCount] = {0};
    napi_value jsThis = nullptr;

    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || argCount > ARGC_ONE) {
        INTELL_VOICE_LOG_ERROR("failed to get parameters");
        return undefinedResult;
    }

    return DeregisterCallback(env, jsThis);
}

napi_value IntellVoiceManagerNapi::DeregisterCallback(napi_env env, napi_value jsThis)
{
    INTELL_VOICE_LOG_INFO("enter");
    napi_value result = nullptr;
    napi_get_undefined(env, &result);

    IntellVoiceManagerNapi *managerNapi = nullptr;
    napi_status status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&managerNapi));
    if (status != napi_ok || managerNapi == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to get engine napi instance");
        return result;
    }
    if (managerNapi->serviceChangeCb_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("service change callback is nullptr");
        return result;
    }

    auto mgr = IntellVoiceManager::GetInstance();
    if (mgr != nullptr) {
        mgr->DeregisterServiceDeathRecipient(managerNapi->serviceChangeCb_);
    }

    managerNapi->serviceChangeCb_ = nullptr;
    return result;
}

napi_value IntellVoiceManagerNapi::GetIntelligentVoiceManager(napi_env env, napi_callback_info info)
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

    return IntellVoiceManagerNapi::GetIntelligentVoiceManagerWrapper(env);
}

napi_value IntellVoiceManagerNapi::GetIntelligentVoiceManagerWrapper(napi_env env)
{
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, g_managerConstructor, &constructor);
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

template <typename T>
napi_value IntellVoiceManagerNapi::CreatePropertyBase(napi_env env, T &propertyMap, napi_ref ref)
{
    napi_value result = nullptr;
    napi_status status;
    napi_value enumNapiValue;

    status = napi_create_object(env, &result);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to create object");
        napi_get_undefined(env, &result);
        return result;
    }

    for (const auto &iter : propertyMap) {
        std::string propName = iter.first;
        int32_t enumValue = iter.second;

        enumNapiValue = SetValue(env, enumValue);
        status = napi_set_named_property(env, result, propName.c_str(), enumNapiValue);
        if (status != napi_ok) {
            break;
        }
        propName.clear();
    }

    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to set base property");
        napi_get_undefined(env, &result);
        return result;
    }

    status = napi_create_reference(env, result, 1, &ref);
    if (status != napi_ok) {
        INTELL_VOICE_LOG_ERROR("failed to create base property");
        napi_get_undefined(env, &result);
        return result;
    }

    INTELL_VOICE_LOG_INFO("create base property success");
    return result;
}

static napi_value Export(napi_env env, napi_value exports)
{
    IntellVoiceManagerNapi::Export(env, exports);
    WakeupManagerNapi::Export(env, exports);
    EnrollIntellVoiceEngineNapi::Export(env, exports);
    WakeupIntellVoiceEngineNapi::Export(env, exports);
    return exports;
}

static napi_module g_module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "ai.intelligentVoice",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&g_module);
}
}  // namespace IntellVoiceNapi
}  // namespace OHOS
