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

#ifndef MOCK_ABILITY_MANAGER_CLIENT_H
#define MOCK_ABILITY_MANAGER_CLIENT_H

#include <memory>
#include <string>

namespace OHOS {
namespace AAFwk {

class Want {
public:
    Want() = default;
    virtual ~Want() = default;
    void SetElementName(const std::string& bundleName, const std::string& abilityName) {}
    void SetParam(const std::string& key, const std::string& value) {}
    void SetParam(const std::string& key, int32_t value) {}
    void SetParam(const std::string& key, bool value) {}
};

class AbilityManagerClient {
public:
    virtual ~AbilityManagerClient() = default;
    static std::shared_ptr<AbilityManagerClient> GetInstance()
    {
        static std::shared_ptr<AbilityManagerClient> instance(std::make_shared<AbilityManagerClient>());
        return instance;
    }

    int StartAbility(const Want &want)
    {
        return 0;
    }
};

static constexpr int ERR_OK = 0;

} // namespace AAFwk
} // namespace OHOS

#endif
