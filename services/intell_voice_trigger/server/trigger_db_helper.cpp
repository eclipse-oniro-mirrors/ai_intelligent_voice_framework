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

#include "trigger_db_helper.h"

#include <string>

#include "rdb_errno.h"
#include "rdb_helper.h"
#include "rdb_open_callback.h"
#include "intell_voice_log.h"
#include "intell_voice_definitions.h"

#define LOG_TAG "TriggerDbHelper"

using namespace OHOS::NativeRdb;

namespace OHOS {
namespace IntellVoiceTrigger {
enum {
    VERSION_ADD_MODEL_TYPE = 2,
};
static const std::string TABLE_NAME = "trigger";

class TriggerModelOpenCallback : public RdbOpenCallback {
public:
    int OnCreate(RdbStore &rdbStore) override;
    int OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion) override;
private:
    static void VersionAddModelType(RdbStore &store);
};

int TriggerModelOpenCallback::OnCreate(RdbStore &rdbStore)
{
    INTELL_VOICE_LOG_INFO("enter");
    const std::string CREATE_TABLE_Trigger = "CREATE TABLE IF NOT EXISTS " + TABLE_NAME +
        " (model_uuid INTEGER PRIMARY KEY, vendor_uuid INTEGER, data BLOB, model_version INTEGER)";

    int32_t result = rdbStore.ExecuteSql(CREATE_TABLE_Trigger);
    if (result != NativeRdb::E_OK) {
        INTELL_VOICE_LOG_ERROR("create table failed, ret:%{public}d", result);
        return result;
    }

    VersionAddModelType(rdbStore);
    return NativeRdb::E_OK;
}

int TriggerModelOpenCallback::OnUpgrade(RdbStore &rdbStore, int oldVersion, int newVersion)
{
    INTELL_VOICE_LOG_INFO("enter, oldVersion:%{public}d, newVersion:%{public}d", oldVersion, newVersion);
    if (oldVersion < VERSION_ADD_MODEL_TYPE) {
        VersionAddModelType(rdbStore);
    }
    return E_OK;
}

void TriggerModelOpenCallback::VersionAddModelType(RdbStore &store)
{
    const std::string alterModelType = "ALTER TABLE " + TABLE_NAME +
        " ADD COLUMN " + "model_type" + " INTEGER";
    int32_t result = store.ExecuteSql(alterModelType);
    if (result != NativeRdb::E_OK) {
        INTELL_VOICE_LOG_WARN("Upgrade rbd model type failed, ret:%{public}d", result);
    }
}

TriggerDbHelper::TriggerDbHelper()
{
    int errCode = E_OK;
    RdbStoreConfig config("/data/service/el1/public/database/intell_voice_service_manager/triggerModel.db");
    TriggerModelOpenCallback helper;
    store_ = RdbHelper::GetRdbStore(config, VERSION_ADD_MODEL_TYPE, helper, errCode);
    if (store_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("store is nullptr");
    }
}

TriggerDbHelper::~TriggerDbHelper()
{
    store_ = nullptr;
}

bool TriggerDbHelper::UpdateGenericTriggerModel(std::shared_ptr<GenericTriggerModel> model)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");

    if (store_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("store is nullptr");
        return false;
    }

    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("model is nullptr");
        return false;
    }

    model->Print();
    int64_t rowId = -1;
    ValuesBucket values;
    values.PutInt("model_uuid", model->GetUuid());
    values.PutInt("vendor_uuid", model->GetVendorUuid());
    values.PutBlob("data", model->GetData());
    values.PutInt("model_version", model->GetVersion());
    values.PutInt("model_type", model->GetType());
    int ret = store_->InsertWithConflictResolution(rowId, TABLE_NAME, values, ConflictResolution::ON_CONFLICT_REPLACE);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("update generic model failed");
        return false;
    }
    ret = store_->ExecuteSql("PRAGMA wal_checkpoint(TRUNCATE)");
    if (ret != E_OK) {
        INTELL_VOICE_LOG_WARN("reduce db_wal file size failed, ret:%{public}d", ret);
    }
    return true;
}

bool TriggerDbHelper::GetVendorUuid(std::shared_ptr<AbsSharedResultSet> &set, int32_t &vendorUuid) const
{
    int columnIndex;
    int ret = set->GetColumnIndex("vendor_uuid", columnIndex);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get model uuid column index, ret:%{public}d", ret);
        return false;
    }

    ret = set->GetInt(columnIndex, vendorUuid);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get vendor uuid, ret:%{public}d", ret);
        return false;
    }
    return true;
}

bool TriggerDbHelper::GetBlob(std::shared_ptr<AbsSharedResultSet> &set, std::vector<uint8_t> &data) const
{
    int columnIndex;
    int ret = set->GetColumnIndex("data", columnIndex);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get data column index, ret:%{public}d", ret);
        return false;
    }

    ret = set->GetBlob(columnIndex, data);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get data, ret:%{public}d", ret);
        return false;
    }
    return true;
}

bool TriggerDbHelper::GetModelVersion(std::shared_ptr<AbsSharedResultSet> &set, int32_t &version) const
{
    int columnIndex;
    int ret = set->GetColumnIndex("model_version", columnIndex);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get model version column index, ret:%{public}d", ret);
        return false;
    }

    ret = set->GetInt(columnIndex, version);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get model version, ret:%{public}d", ret);
        return false;
    }
    return true;
}

bool TriggerDbHelper::GetModelType(std::shared_ptr<AbsSharedResultSet> &set, int32_t &type) const
{
    int columnIndex;
    int ret = set->GetColumnIndex("model_type", columnIndex);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get model type column index, ret:%{public}d", ret);
        return false;
    }

    ret = set->GetInt(columnIndex, type);
    if (ret != E_OK) {
        INTELL_VOICE_LOG_ERROR("failed to get model type, ret:%{public}d", ret);
        return false;
    }
    INTELL_VOICE_LOG_INFO("model type:%{public}d", type);
    return true;
}

std::shared_ptr<GenericTriggerModel> TriggerDbHelper::GetGenericTriggerModel(const int32_t modelUuid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter, model uuid:%{public}d", modelUuid);
    if (store_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("store is nullptr");
        return nullptr;
    }

    std::shared_ptr<AbsSharedResultSet> set = store_->QuerySql(
        "SELECT * FROM trigger WHERE model_uuid = ?", std::vector<std::string> {std::to_string(modelUuid)});
    if (set == nullptr) {
        INTELL_VOICE_LOG_ERROR("set is nullptr");
        return nullptr;
    }

    set->GoToFirstRow();

    int32_t vendorUuid;
    if (!GetVendorUuid(set, vendorUuid)) {
        INTELL_VOICE_LOG_ERROR("failed to get vendor uuid");
        return nullptr;
    }

    std::vector<uint8_t> data;
    if (!GetBlob(set, data)) {
        INTELL_VOICE_LOG_ERROR("failed to get data");
        return nullptr;
    }

    int32_t modelVersion;
    if (!GetModelVersion(set, modelVersion)) {
        INTELL_VOICE_LOG_ERROR("failed to get model version");
        return nullptr;
    }

    int32_t type;
    if (modelVersion >= static_cast<int32_t>(TriggerModel::TriggerModelVersion::MODLE_VERSION_2)) {
        if (!GetModelType(set, type)) {
            INTELL_VOICE_LOG_ERROR("failed to get model type");
            return nullptr;
        }
    } else {
        type = (modelUuid == OHOS::IntellVoiceEngine::VOICE_WAKEUP_MODEL_UUID ?
            TriggerModelType::VOICE_WAKEUP_TYPE : TriggerModelType::PROXIMAL_WAKEUP_TYPE);
    }

    std::shared_ptr<GenericTriggerModel> model = std::make_shared<GenericTriggerModel>(modelUuid, modelVersion,
        static_cast<TriggerModelType>(type));
    if (model == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to alloc model");
        return nullptr;
    }
    model->SetData(data);
    return model;
}

void TriggerDbHelper::DeleteGenericTriggerModel(const int32_t modelUuid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    INTELL_VOICE_LOG_INFO("enter");
    if (store_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("store is nullptr");
        return;
    }
    int deletedRows;
    store_->Delete(deletedRows, "trigger", "model_uuid = ?", std::vector<std::string> {std::to_string(modelUuid)});
    int result = store_->ExecuteSql("PRAGMA wal_checkpoint(TRUNCATE)");
    if (result != E_OK) {
        INTELL_VOICE_LOG_WARN("reduce db_wal file size failed, ret:%{public}d", result);
    }
}
}  // namespace IntellVoiceTrigger
}  // namespace OHOS
