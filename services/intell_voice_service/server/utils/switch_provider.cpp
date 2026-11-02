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
#include "switch_provider.h"
#include "intell_voice_log.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#define LOG_TAG "SwitchProvider"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
const std::string SWITCH_URI_PROXY = "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
const std::string SWITCH_DATA_EXT_URI = "datashare:///com.ohos.settingsdata.DataAbility";
const std::string WAKEUP_REASON_KEY = "intell_voice_switch_record";
const std::string WHISPER_REASON_KEY = "intell_whisper_switch_record";
namespace {
Uri AssembleUri(const std::string& key)
{
    Uri uri(SWITCH_URI_PROXY + "&key=" + key);
    return uri;
}
}

SwitchProvider::SwitchProvider()
{
}

SwitchProvider::~SwitchProvider()
{
    helper_ = nullptr;
}

bool SwitchProvider::Init()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("saManager is nullptr");
        return false;
    }
    auto remoteObj = saManager->GetSystemAbility(INTELL_VOICE_SERVICE_ID);
    if (remoteObj == nullptr) {
        INTELL_VOICE_LOG_ERROR("remoteObj is nullptr");
        return false;
    }

    helper_ = DataShare::DataShareHelper::Creator(remoteObj, SWITCH_URI_PROXY);
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper_ is nullptr");
        return false;
    }
    return true;
}

void SwitchProvider::RegisterObserver(const sptr<SwitchObserver> &observer, const std::string &key)
{
    auto uri = AssembleUri(key);
    helper_->RegisterObserver(uri, observer);
}

void SwitchProvider::UnregisterObserver(const sptr<SwitchObserver> &observer, const std::string &key)
{
    auto uri = AssembleUri(key);
    helper_->UnregisterObserver(uri, observer);
}

bool SwitchProvider::QuerySwitchStatus(const std::string &key)
{
    auto resultSet = QueryWithRetry(key);
    if (resultSet == nullptr) {
        INTELL_VOICE_LOG_WARN("resultSet is null, key is %{public}s", key.c_str());
        return false;
    }

    int32_t count;
    resultSet->GetRowCount(count);
    if (count == 0) {
        INTELL_VOICE_LOG_WARN("not found value, key is %{public}s", key.c_str());
        resultSet->Close();
        return false;
    }
    const int32_t INDEX = 0;
    resultSet->GoToRow(INDEX);
    std::string value;
    resultSet->GetString(INDEX, value);
    resultSet->Close();

    if (value == "0") {
        return false;
    } else if (value == "1") {
        return true;
    } else {
        return false;
    }
}

void SwitchProvider::QueryAndLogReason()
{
    if (helper_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("helper_ is nullptr");
        return;
    }
    std::vector<std::string> columns = {"VALUE"};
    const std::vector<std::pair<std::string, std::string>> queries = {
        {WAKEUP_REASON_KEY, "wakeup"},
        {WHISPER_REASON_KEY, "whisper"}
    };

    for (const auto& [key, logName] : queries) {
        DataShare::DataSharePredicates pred;
        pred.EqualTo("KEYWORD", key);
        auto uri = AssembleUri(key);
        auto resultSet = helper_->Query(uri, pred, columns);
        if (resultSet == nullptr) {
            INTELL_VOICE_LOG_WARN("Failed to query reason for %{public}s key", logName.c_str());
            continue;
        }
        
        int32_t count = 0;
        resultSet->GetRowCount(count);
        if (count == 0) {
            INTELL_VOICE_LOG_INFO("No reason record found for %{public}s key", logName.c_str());
        } else {
            const int32_t INDEX = 0;
            resultSet->GoToRow(INDEX);
            std::string reason;
            resultSet->GetString(INDEX, reason);
            INTELL_VOICE_LOG_WARN("Switch reason for %{public}s key: %{public}s",
                                 logName.c_str(), reason.c_str());
        }
        resultSet->Close();
    }
}

bool SwitchProvider::IsSwitchError(const std::string &key)
{
    auto resultSet = QueryWithRetry(key);
    if (resultSet == nullptr) {
        INTELL_VOICE_LOG_ERROR("resultSet is nullptr");
        return true;
    }

    int32_t count;
    resultSet->GetRowCount(count);
    if (count == 0) {
        INTELL_VOICE_LOG_ERROR("not found value, key is %{public}s", key.c_str());
        resultSet->Close();
        return true;
    }

    return false;
}

std::shared_ptr<DataShare::DataShareResultSet> SwitchProvider::QueryWithRetry(const std::string &key)
{
    std::vector<std::string> columns = {"VALUE"};
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo("KEYWORD", key);
    auto uri = AssembleUri(key);
    std::shared_ptr<DataShare::DatashareBusinessError> error = std::make_shared<DataShare::DatashareBusinessError>();
    auto resultSet = helper_->Query(uri, predicates, columns, error.get());
    if (resultSet != nullptr || error->GetCode() != DataShare::E_RESULTSET_BUSY) {
        return resultSet;
    }
    INTELL_VOICE_LOG_WARN("db query busy, retry...");
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("saManager is nullptr");
        return nullptr;
    }
    auto remoteObj = saManager->GetSystemAbility(INTELL_VOICE_SERVICE_ID);
    if (remoteObj == nullptr) {
        INTELL_VOICE_LOG_ERROR("remoteObj is nullptr");
        return nullptr;
    }
    auto retryHelper = DataShare::DataShareHelper::Creator(remoteObj, SWITCH_DATA_EXT_URI, "");
    if (retryHelper == nullptr) {
        INTELL_VOICE_LOG_ERROR("retryHelper is nullptr");
        return nullptr;
    }
    return retryHelper->Query(uri, predicates, columns);
}

bool SwitchProvider::CheckIfDataShareReady()
{
    auto saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        INTELL_VOICE_LOG_ERROR("saManager is nullptr");
        return false;
    }
    auto remoteObj = saManager->GetSystemAbility(INTELL_VOICE_SERVICE_ID);
    if (remoteObj == nullptr) {
        INTELL_VOICE_LOG_ERROR("remoteObj is nullptr");
        return false;
    }

    auto ret = DataShare::DataShareHelper::Create(remoteObj, SWITCH_URI_PROXY, SWITCH_DATA_EXT_URI);
    if (ret.first == OHOS::DataShare::E_OK) {
        INTELL_VOICE_LOG_INFO("create data share helper success");
        if (ret.second != nullptr) {
            ret.second->Release();
        }
        return true;
    }
    if (ret.first == OHOS::DataShare::E_DATA_SHARE_NOT_READY) {
        INTELL_VOICE_LOG_INFO("create data share helper failed");
        return false;
    }
    INTELL_VOICE_LOG_INFO("data share ret:%{public}d is unknown", ret.first);
    return true;
}
}  // namespace IntellVoice
}  // namespace OHOS