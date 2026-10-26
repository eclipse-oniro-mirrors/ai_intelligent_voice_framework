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

#ifndef INTELL_VOICE_GENERIC_FACTORY_H
#define INTELL_VOICE_GENERIC_FACTORY_H
#include <memory>
#include "refbase.h"

namespace OHOS {
namespace IntellVoiceUtils {
template <typename B, typename D>
class EngineImplFactory {
public:
    template<typename ...Args>
    static std::shared_ptr<B> CreateInstance(Args && ...args)
    {
        B *instance = new(std::nothrow) D {};
        if (instance == nullptr) {
            return nullptr;
        }
        if (!instance->Create(std::forward<Args>(args)...)) {
            delete instance;
            return nullptr;
        }
        return std::shared_ptr<B>(instance);
    }
};

template <typename T>
class UniquePtrFactory {
public:
    using UniqueProductPtr = std::unique_ptr<T, void (*)(T *)>;

    template<typename ...Args>
    static UniqueProductPtr CreateInstance(Args && ...args)
    {
        T *instance = new(std::nothrow) T {};
        if (instance == nullptr) {
            return UniqueProductPtr {nullptr, nullptr};
        }
        if (!instance->Init(std::forward<Args>(args)...)) {
            delete instance;
            return UniqueProductPtr {nullptr, nullptr};
        }
        return UniqueProductPtr(instance, DestroyInstance);
    }

private:
    static void DestroyInstance(T *ptr)
    {
        delete ptr;
    }
};

template<typename T>
using UniqueProductType = typename UniquePtrFactory<T>::UniqueProductPtr;

template <typename B, typename D>
class SptrFactory {
public:
    using SptrProductPtr = OHOS::sptr<B>;

    template<typename ...Args>
    static SptrProductPtr CreateInstance(Args && ...args)
    {
        B *instance = new(std::nothrow) D {};
        if (instance == nullptr) {
            return SptrProductPtr { nullptr };
        }
        if (!instance->Init(std::forward<Args>(args)...)) {
            delete instance;
            return SptrProductPtr { nullptr };
        }
        return SptrProductPtr(instance);
    }
};

template<typename B, typename D>
using SptrProductType = typename SptrFactory<B, D>::SptrProductPtr;
}
}
#endif
