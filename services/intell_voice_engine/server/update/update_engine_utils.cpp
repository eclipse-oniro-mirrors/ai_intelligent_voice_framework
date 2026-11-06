/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "update_engine_utils.h"

#include <fstream>
#include "securec.h"
#include "intell_voice_log.h"
#include "string_util.h"
#include "history_info_mgr.h"
#include "intell_voice_definitions.h"
#include "engine_host_manager.h"

#define LOG_TAG "UpdateEngineUtils"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
const std::string INTELL_VOICE_DEFAULT_RES_PATH = "/sys_prod/variant/region_comm/china/etc/intellvoice/wakeup/";
const std::string INTELL_VOICE_CONDICT_PATH_DSP = "/dsp/condict/";
static constexpr uint32_t WAKEUP_VERSION_SPLIT_NUM = 2;
static constexpr uint32_t WAKEUP_VERSION_NUMBERS = 3;

UpdateEngineUtils::UpdateEngineUtils()
{
}

UpdateEngineUtils::~UpdateEngineUtils()
{
}

void UpdateEngineUtils::GetWakeupVesion(std::string &versionNumber)
{
    std::string filePath = INTELL_VOICE_DEFAULT_RES_PATH + INTELL_VOICE_CONDICT_PATH_DSP + "version.txt";
    std::string wakeupVersion;
    std::ifstream file(filePath);

    INTELL_VOICE_LOG_INFO("enter");
    if (!file.good()) {
        INTELL_VOICE_LOG_ERROR("Open file(%{public}s) failed", filePath.c_str());
        return;
    }

    getline(file, wakeupVersion);

    if (wakeupVersion.empty()) {
        INTELL_VOICE_LOG_ERROR("wakeupVersion empty");
        return;
    }

    INTELL_VOICE_LOG_INFO("wakeupVersion: (%{public}s)", wakeupVersion.c_str());

    std::vector<std::string> tokens;
    StringUtil::Split(wakeupVersion, "v.", tokens);
    if (tokens.size() != WAKEUP_VERSION_SPLIT_NUM || tokens[1].empty()) {
        INTELL_VOICE_LOG_INFO("wakeupVersion split empty (%{public}zu)", tokens.size());
        return;
    }
    /* 5.2.1 -> 050201 */
    std::string version = tokens[1];
    tokens.resize(0);
    StringUtil::Split(version, ".", tokens);
    if (tokens.size() != WAKEUP_VERSION_NUMBERS) {
        INTELL_VOICE_LOG_INFO("vesion split empty");
        return;
    }

    for (size_t index = 0; index < tokens.size(); index++) {
        if (tokens[index].length() == 1) {
            tokens[index] = "0" + tokens[index];
        }
        versionNumber += tokens[index];
    }
    INTELL_VOICE_LOG_INFO("exit (%{public}s)", versionNumber.c_str());
}

void UpdateEngineUtils::SaveWakeupVesion()
{
    std::string versionNumber;
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();

    GetWakeupVesion(versionNumber);
    if (versionNumber.empty()) {
        return;
    }

    historyInfoMgr.SetStringKVPair(KEY_WAKEUP_VESRION, versionNumber);
}

bool UpdateEngineUtils::IsBackupSourceExist()
{
    std::vector<std::string> cloneFiles;
    int32_t ret = EngineHostManager::GetInstance().GetWakeupSourceFilesList(cloneFiles);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("GetWakeupSourceFilesList failed, ret: %{public}d", ret);
        return false;
    }

    bool found = false;
    for (const auto& file : cloneFiles) {
        if (file.find("enroll_phrase.txt") != std::string::npos) {
            found = true;
            break;
        }
    }
    return found;
}

bool UpdateEngineUtils::IsVersionUpdate()
{
    std::string versionNumberSave;
    std::string versionNumberCur;
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();

    versionNumberSave = historyInfoMgr.GetStringKVPair(KEY_WAKEUP_VESRION);
    if (versionNumberSave.empty()) {
        INTELL_VOICE_LOG_ERROR("versionNumberSave is null");
        return false;
    }

    GetWakeupVesion(versionNumberCur);
    if (versionNumberCur.empty()) {
        INTELL_VOICE_LOG_ERROR("versionNumberCur is null");
        return false;
    }

    int32_t historyVersion = 0;
    int32_t curVersion = 0;
    if ((!StringUtil::StringToInt(versionNumberSave, historyVersion)) ||
        (!StringUtil::StringToInt(versionNumberCur, curVersion))) {
        INTELL_VOICE_LOG_ERROR("failed to get version");
        return false;
    }

    if (!IsBackupSourceExist()) {
        INTELL_VOICE_LOG_ERROR("backup audio not found");
        return false;
    }

    if (curVersion > historyVersion) {
        INTELL_VOICE_LOG_INFO("version new %{public}d, cur %{public}d", curVersion, historyVersion);
        return true;
    }
    return false;
}
}
}