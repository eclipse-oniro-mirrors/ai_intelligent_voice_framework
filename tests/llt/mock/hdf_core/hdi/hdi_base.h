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

#ifndef HDI_BASE_H
#define HDI_BASE_H

#include <cstdint>
#include <string>
#include <cstddef>
#include <refbase.h>
#include "hdf_base.h"

namespace OHOS {
namespace HDI {

class HdiBase : virtual public RefBase {
public:
    HdiBase() {}
    virtual ~HdiBase() {}
    virtual int32_t GetVersion(uint32_t& majorVer, uint32_t& minorVer)
    {
        majorVer = 1;
        minorVer = 0;
        return HDF_SUCCESS;
    }
    virtual bool IsProxy() { return false; }
    virtual const std::u16string GetDesc() { return metaDescriptor_; }
    std::u16string metaDescriptor_;
};

} // namespace HDI
} // namespace OHOS

#endif