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

#ifndef HDI_DEVICE_MANAGER_HDI_INF_H
#define HDI_DEVICE_MANAGER_HDI_INF_H

#include <string>
#include "hdi_base.h"

namespace OHOS {
namespace HDI {
namespace DeviceManager {
namespace V1_0 {

class IDeviceManager : public HdiBase {
public:
    DECLARE_HDI_DESCRIPTOR(u"ohos.hdi.devicemanager.v1_0.IDeviceManager")

    virtual ~IDeviceManager() = default;

    static sptr<IDeviceManager> Get();

    virtual int32_t LoadDevice(const std::string &serviceName) = 0;
    virtual int32_t UnloadDevice(const std::string &serviceName) = 0;
};

} // V1_0
} // DeviceManager
} // HDI
} // OHOS

#endif // HDI_DEVICE_MANAGER_HDI_INF_H
