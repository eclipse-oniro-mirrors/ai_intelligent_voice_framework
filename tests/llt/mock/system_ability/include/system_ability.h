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

#ifndef MOCK_SYSTEM_ABILITY_H
#define MOCK_SYSTEM_ABILITY_H

#include <string>
#include <memory>
#include <cstdint>
#include <refbase.h>
#include <iremote_object.h>

namespace OHOS {

enum {
    COMMON_EVENT_SERVICE_ID = 3299,
    DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID = 1301,
    TELEPHONY_STATE_REGISTRY_SYS_ABILITY_ID = 4009,
    AUDIO_DISTRIBUTED_SERVICE_ID = 3001,
    AUDIO_POLICY_SERVICE_ID = 3009,
    POWER_MANAGER_SERVICE_ID = 3301,
    DISPLAY_MANAGER_SERVICE_ID = 3308,
    BUNDLE_MGR_SERVICE_SYS_ABILITY_ID = 401,
    INTELL_VOICE_SERVICE_ID = 312,
};

class SystemAbilityOnDemandReason {
public:
    SystemAbilityOnDemandReason() : id_(-1) {}
    virtual ~SystemAbilityOnDemandReason() {}

    void SetId(int32_t id)
    {
        id_ = id;
    }

    int32_t GetId() const
    {
        return id_;
    }

    void SetName(const std::string& name)
    {
        name_ = name;
    }

    const std::string& GetName() const
    {
        return name_;
    }

    void SetValue(const std::string& value)
    {
        value_ = value;
    }

    const std::string& GetValue() const
    {
        return value_;
    }

private:
    int32_t id_;
    std::string name_;
    std::string value_;
};

enum class OnDemandReasonId : int32_t {
    COMMON_EVENT = 0,
    INTERFACE_CALL = 1,
    PERFORMANCE = 2,
    DEVICE_STATE = 3,
    LOCATION = 4,
    TIME = 5,
    NETWORK = 6,
    LOADING = 7,
    OTHER = 100,
};

class SystemAbility : public RefBase {
public:
    SystemAbility() {}
    SystemAbility(int32_t systemAbilityId, bool runOnCreate = false)
        : saId_(systemAbilityId), runOnCreate_(runOnCreate) {}
    virtual ~SystemAbility() {}

    bool AddSystemAbilityListener(int32_t systemAbilityId)
    {
        (void)systemAbilityId;
        return false;
    }

    bool Publish(sptr<SystemAbility> ability)
    {
        (void)ability;
        return true;
    }

    int32_t GetSystemAbilityId() const
    {
        return saId_;
    }

public:
    virtual void OnStart(const SystemAbilityOnDemandReason &startReason) {}
    virtual void OnStop() {}
    virtual int32_t OnIdle(const SystemAbilityOnDemandReason &idleReason) { return 0; }
    virtual void OnActive(const SystemAbilityOnDemandReason &activeReason) {}
    virtual void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) {}
    virtual void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) {}

private:
    int32_t saId_;
    bool runOnCreate_;
};

class SystemAbilityManagerClient {
public:
    static SystemAbilityManagerClient& GetInstance()
    {
        static SystemAbilityManagerClient instance;
        return instance;
    }
    virtual ~SystemAbilityManagerClient() {}

    class SystemAbilityManager {
    public:
        SystemAbilityManager() {}
        ~SystemAbilityManager() {}

        sptr<IRemoteObject> CheckSystemAbility(int32_t systemAbilityId)
        {
            (void)systemAbilityId;
            return nullptr;
        }

        sptr<IRemoteObject> GetSystemAbility(int32_t systemAbilityId)
        {
            (void)systemAbilityId;
            return nullptr;
        }

        int32_t UnloadSystemAbility(int32_t systemAbilityId)
        {
            (void)systemAbilityId;
            return 0;
        }
    };

    SystemAbilityManager* GetSystemAbilityManager()
    {
        return &manager_;
    }

private:
    SystemAbilityManager manager_;
};

#define DECLARE_SYSTEM_ABILITY(className) \
public: \
static int32_t GetSystemAbilityId() { return 0; } \
std::string GetClassName() { return #className; }

} // namespace OHOS

#endif
