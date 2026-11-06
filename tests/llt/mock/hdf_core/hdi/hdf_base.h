/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef HDF_BASE_H
#define HDF_BASE_H

#include <cstdint>
#include <string>
#include <cstddef>

#define HDF_SUCCESS 0
#define HDF_FAILURE (-1)

#ifndef HDI_BUFF_MAX_SIZE
#define HDI_BUFF_MAX_SIZE (1024 * 200)
#endif

#ifndef HDI_VECTOR_MAX_SIZE
#define HDI_VECTOR_MAX_SIZE 102400000
#endif

#ifndef HDI_MAP_MAX_SIZE
#define HDI_MAP_MAX_SIZE 102400000
#endif

#define DECLARE_HDI_DESCRIPTOR(descriptor) \
    __attribute__((visibility("hidden"))) \
    const static inline std::u16string metaDescriptor_ = {descriptor}; \
    const static inline std::u16string &GetDescriptor() \
    { \
        return metaDescriptor_; /* NOLINT */ \
    }

#endif