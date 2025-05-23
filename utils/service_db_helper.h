/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef INTELL_VOICE_SERVICE_DB_HELPER_H
#define INTELL_VOICE_SERVICE_DB_HELPER_H

#include "distributed_kv_data_manager.h"

namespace OHOS {
namespace IntellVoiceUtils {
class ServiceDbHelper {
public:
    ServiceDbHelper(const std::string &inAppId, const std::string &inStoreId);
    ~ServiceDbHelper();

    void SetValue(const std::string &key, const std::string &value);
    std::string GetValue(const std::string &key);
    void Delete(const std::string &key);

private:
    std::shared_ptr<DistributedKv::SingleKvStore> kvStore_ = nullptr;
};
}
}
#endif