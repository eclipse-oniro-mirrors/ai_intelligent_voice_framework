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

#ifndef MOCK_BUNDLE_MGR_INTERFACE_H
#define MOCK_BUNDLE_MGR_INTERFACE_H

#include <cstdint>
#include <string>
#include <vector>

namespace OHOS {
namespace AppExecFwk {

class IBundleMgr {
public:
    virtual ~IBundleMgr() = default;
};

class BundleInfo {
public:
    std::string name;
    std::string bundleName;
    int32_t versionCode = 0;
};

} // namespace AppExecFwk
} // namespace OHOS

#endif