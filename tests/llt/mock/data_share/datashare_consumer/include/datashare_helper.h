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

#ifndef MOCK_DATA_SHARE_HELPER_H
#define MOCK_DATA_SHARE_HELPER_H

#include <string>
#include <memory>
#include <vector>

namespace OHOS {
namespace DataShare {

constexpr int32_t E_OK = 0;
constexpr int32_t E_RESULTSET_BUSY = 1;
constexpr int32_t E_DATA_SHARE_NOT_READY = 2;

class DataShareBusinessError {
public:
    DataShareBusinessError() : code_(0) {}
    explicit DataShareBusinessError(int32_t code) : code_(code) {}
    int32_t GetCode() const { return code_; }
private:
    int32_t code_;
};

using DatashareBusinessError = DataShareBusinessError;

class DataShareResultSet;
class DataSharePredicates;

class Uri {
public:
    Uri(const std::string &uriString) : uriString_(uriString) {}
    ~Uri() = default;

    std::string GetUriString() const
    {
        return uriString_;
    }

private:
    std::string uriString_;
};

class DataShareHelper {
public:
    static std::shared_ptr<DataShareHelper> Creator(void *remote, const std::string &uri)
    {
        (void)remote;
        (void)uri;
        return nullptr;
    }

    static std::shared_ptr<DataShareHelper> Creator(void *remote, const std::string &uri, const std::string &extUri)
    {
        (void)remote;
        (void)uri;
        (void)extUri;
        return nullptr;
    }

    static std::pair<int, std::shared_ptr<DataShareHelper>> Create(void *remote, const std::string &uri,
        const std::string &extUri)
    {
        (void)remote;
        (void)uri;
        (void)extUri;
        return {0, nullptr};
    }

    ~DataShareHelper() {}

    int RegisterObserver(const Uri &uri, void *observer)
    {
        (void)uri;
        (void)observer;
        return 0;
    }

    int UnregisterObserver(const Uri &uri, void *observer)
    {
        (void)uri;
        (void)observer;
        return 0;
    }

    std::shared_ptr<DataShareResultSet> Query(const Uri &uri, const DataSharePredicates &predicates,
        const std::vector<std::string> &columns)
    {
        (void)uri;
        (void)predicates;
        (void)columns;
        return nullptr;
    }

    std::shared_ptr<DataShareResultSet> Query(const Uri &uri, const DataSharePredicates &predicates,
        const std::vector<std::string> &columns, DataShareBusinessError *error)
    {
        (void)uri;
        (void)predicates;
        (void)columns;
        (void)error;
        return nullptr;
    }

    int Create(const std::string &uri)
    {
        (void)uri;
        return 0;
    }

    int Release()
    {
        return 0;
    }
};

class DataShareResultSet {
public:
    DataShareResultSet() = default;
    virtual ~DataShareResultSet() = default;

    int Close()
    {
        return 0;
    }

    int GoToRow(int32_t index)
    {
        (void)index;
        return 0;
    }

    int GetRowCount(int32_t &count)
    {
        count = 0;
        return 0;
    }

    int GetString(int32_t col, std::string &value)
    {
        (void)col;
        value = "";
        return 0;
    }

    int GetInt(int32_t col, int32_t &value)
    {
        (void)col;
        value = 0;
        return 0;
    }
};

class DataSharePredicates {
public:
    DataSharePredicates() = default;
    virtual ~DataSharePredicates() = default;

    DataSharePredicates& EqualTo(const std::string &field, const std::string &value)
    {
        (void)field;
        (void)value;
        return *this;
    }
};

} // namespace DataShare

using Uri = DataShare::Uri;

} // namespace OHOS

#endif
