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

#ifndef MOCK_DISTRIBUTED_KV_DATA_MANAGER_H
#define MOCK_DISTRIBUTED_KV_DATA_MANAGER_H

#include <memory>
#include <string>
#include <vector>

namespace OHOS {
namespace DistributedKv {

enum Status {
    SUCCESS = 0,
    ERROR = 1,
    INVALID_ARGUMENT = 2,
    NOT_FOUND = 3,
    KV_EKEYREVOKED = 4,
    EKEYID = 5,
};

enum class KvStoreType {
    SINGLE_VERSION = 0,
    MULTI_VERSION = 1,
    DEVICE_COLLABORATION = 2,
};

enum class SecurityLevel {
    S1 = 0,
    S2 = 1,
    S3 = 2,
    S4 = 3,
};

enum class Area {
    EL1 = 0,
    EL2 = 1,
    EL3 = 2,
    EL4 = 3,
};

struct StoreId {
    std::string storeId;
};

struct AppId {
    std::string appId;
};

struct Options {
    bool createIfMissing = true;
    bool encrypt = false;
    bool autoSync = false;
    SecurityLevel securityLevel = SecurityLevel::S1;
    Area area = Area::EL1;
    KvStoreType kvStoreType = KvStoreType::SINGLE_VERSION;
    std::string dataDir = "";
    std::string baseDir = "";
};

struct Value {
    std::vector<uint8_t> data;
    Value() = default;
    Value(const std::string &str)
    {
        data.assign(str.begin(), str.end());
    }
    std::string ToString() const
    {
        return std::string(data.begin(), data.end());
    }
};

class SingleKvStore {
public:
    virtual ~SingleKvStore() = default;
    virtual Status Put(const std::string &key, const std::string &value) = 0;
    virtual Status Get(const std::string &key, Value &value) = 0;
    virtual Status Delete(const std::string &key) = 0;
};

class IDistributedKvDataManager {
public:
    virtual ~IDistributedKvDataManager() = default;
    virtual Status GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
        std::shared_ptr<SingleKvStore> &singleKvStore) = 0;
    virtual Status CloseKvStore(const AppId &appId, const StoreId &storeId) = 0;
    virtual Status DeleteKvStore(const AppId &appId, const StoreId &storeId, const std::string &path = "") = 0;
};

class DistributedKvDataManager {
public:
    DistributedKvDataManager() = default;
    ~DistributedKvDataManager() = default;

    Status GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
        std::shared_ptr<SingleKvStore> &singleKvStore)
    {
        (void)options;
        (void)appId;
        (void)storeId;
        singleKvStore = nullptr;
        return Status::SUCCESS;
    }

    Status CloseKvStore(const AppId &appId, const StoreId &storeId)
    {
        (void)appId;
        (void)storeId;
        return Status::SUCCESS;
    }

    Status DeleteKvStore(const AppId &appId, const StoreId &storeId, const std::string &path = "")
    {
        (void)appId;
        (void)storeId;
        (void)path;
        return Status::SUCCESS;
    }
};

} // namespace DistributedKv
} // namespace OHOS

#endif
