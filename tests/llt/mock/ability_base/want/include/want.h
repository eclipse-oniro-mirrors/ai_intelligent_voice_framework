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

#ifndef MOCK_WANT_H
#define MOCK_WANT_H

#include <memory>
#include <string>
#include <map>
#include <vector>

namespace OHOS {
namespace AppExecFwk {

class Want {
public:
    Want() = default;
    virtual ~Want() = default;
    void SetElementName(const std::string& bundleName, const std::string& abilityName)
    {
        (void)bundleName;
        (void)abilityName;
    }
    void SetParam(const std::string& key, const std::string& value)
    {
        params_[key] = value;
    }
    void SetParam(const std::string& key, int32_t value)
    {
        params_[key] = std::to_string(value);
    }
    void SetParam(const std::string& key, bool value)
    {
        params_[key] = value ? "true" : "false";
    }
    std::string GetStringParam(const std::string& key) const
    {
        auto it = params_.find(key);
        if (it != params_.end()) {
            return it->second;
        }
        return "";
    }
    std::string GetAction() const { return action_; }
    void SetAction(const std::string& action) { action_ = action; }

private:
    std::map<std::string, std::string> params_;
    std::string action_;
};

} // namespace AppExecFwk

namespace AAFwk = AppExecFwk;

} // namespace OHOS

#endif