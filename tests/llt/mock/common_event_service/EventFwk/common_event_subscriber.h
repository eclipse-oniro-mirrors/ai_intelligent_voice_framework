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

#ifndef MOCK_COMMON_EVENT_SUBSCRIBER_H
#define MOCK_COMMON_EVENT_SUBSCRIBER_H

#include <memory>
#include <string>
#include <vector>
#include "want.h"

namespace OHOS {
namespace EventFwk {

class MatchingSkills {
public:
    MatchingSkills() = default;
    ~MatchingSkills() = default;
    void AddEvent(const std::string &event) { events_.push_back(event); }
private:
    std::vector<std::string> events_;
};

class CommonEventSubscribeInfo {
public:
    CommonEventSubscribeInfo() = default;
    explicit CommonEventSubscribeInfo(const MatchingSkills &matchingSkills) : matchingSkills_(matchingSkills) {}
    virtual ~CommonEventSubscribeInfo() = default;
    const MatchingSkills& GetMatchingSkills() const { return matchingSkills_; }
private:
    MatchingSkills matchingSkills_;
};

class CommonEventData {
public:
    CommonEventData() = default;
    ~CommonEventData() = default;
    const OHOS::AppExecFwk::Want& GetWant() const { return want_; }
private:
    OHOS::AppExecFwk::Want want_;
};

class CommonEventSubscriber {
public:
    explicit CommonEventSubscriber(const CommonEventSubscribeInfo &subscribeInfo) {}
    virtual ~CommonEventSubscriber() = default;
    virtual void OnReceiveEvent(const CommonEventData &eventData) {}
};

class CommonEventManager {
public:
    static bool SubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber)
    {
        return true;
    }
    static bool UnSubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber)
    {
        return true;
    }
};

class CommonEventSupport {
public:
    static const std::string COMMON_EVENT_SCREEN_ON;
    static const std::string COMMON_EVENT_SCREEN_OFF;
    static const std::string COMMON_EVENT_PACKAGE_REMOVED;
    static const std::string COMMON_EVENT_PACKAGE_REPLACED;
    static const std::string COMMON_EVENT_PACKAGE_DATA_CLEARED;
    static const std::string COMMON_EVENT_DATA_SHARE_READY;
};

}
}

#endif
