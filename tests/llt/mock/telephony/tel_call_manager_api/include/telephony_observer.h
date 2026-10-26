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

#ifndef MOCK_TELEPHONY_OBSERVER_H
#define MOCK_TELEPHONY_OBSERVER_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Telephony {

enum {
    OBSERVER_MASK_CALL_STATE = 1,
    OBSERVER_MASK_SMS_STATE = 2,
    OBSERVER_MASK_SIM_STATE = 4,
};

class TelephonyObserver {
public:
    virtual ~TelephonyObserver() = default;
    virtual void OnCallStateUpdated(int32_t slotId, int32_t callState, const std::u16string& phoneNumber) {}
};

class TelephonyObserverClient {
public:
    static TelephonyObserverClient& GetInstance()
    {
        static TelephonyObserverClient instance;
        return instance;
    }
    virtual ~TelephonyObserverClient() = default;

    int32_t AddStateObserver(TelephonyObserver* observer, int32_t slotId, int32_t mask, bool notifyNow)
    {
        (void)observer;
        (void)slotId;
        (void)mask;
        (void)notifyNow;
        return 0;
    }

    int32_t RemoveStateObserver(int32_t slotId, int32_t mask)
    {
        (void)slotId;
        (void)mask;
        return 0;
    }
};

class TelephonyObserverBroker {
public:
    static constexpr int32_t OBSERVER_MASK_CALL_STATE = 1;
    static constexpr int32_t OBSERVER_MASK_SMS_STATE = 2;
    static constexpr int32_t OBSERVER_MASK_SIM_STATE = 4;
};

} // namespace Telephony
} // namespace OHOS

#endif