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
#include "intell_voice_sensibility.h"

#include <fstream>
#include <algorithm>

#include "history_info_mgr.h"
#include "intell_voice_log.h"
#include "string_util.h"
#include "intell_voice_definitions.h"

#define LOG_TAG "IntellVoiceSensibility"

using namespace std;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int32_t MIN_SENSIBILITY_VALUE = 1;
static constexpr int32_t MAX_SENSIBILITY_VALUE = 3;
static constexpr int32_t THRESH_OFFST = 3;
static constexpr uint32_t HAL_FEATURE_KWS_NODES_900 = 0x2;
static const std::string WAKEUP_CONF_DEFAULPHRASE = "DefaultPhrase";

std::string IntellVoiceSensibility::GetDspSensibility(const std::string &sensibility, const std::string &dspFeature,
    const std::string &configPath)
{
    int32_t value = 0;
    if (!StringUtil::StringToInt(sensibility, value)) {
        INTELL_VOICE_LOG_ERROR("failed to get sensibility");
        return "";
    }
    if ((value < MIN_SENSIBILITY_VALUE) || (value > MAX_SENSIBILITY_VALUE)) {
        INTELL_VOICE_LOG_WARN("invalid sensibility:%{public}d", value);
        return "";
    }
    std::string wakeupPhrase = HistoryInfoMgr::GetInstance().GetStringKVPair(KEY_WAKEUP_PHRASE);
    if (wakeupPhrase.empty()) {
        INTELL_VOICE_LOG_WARN("wakeup phrase is empty");
        return "";
    }
    int32_t index = (IsSupportNodes800(dspFeature) ? (value + THRESH_OFFST - 1) : (value - 1));
    return ParseWakeupConfigDspSensibility(wakeupPhrase, static_cast<uint32_t>(index), configPath);
}

std::string IntellVoiceSensibility::ParseWakeupConfigDspSensibility(const std::string &wakeupPhrase, uint32_t index,
    const std::string &configPath)
{
    std::ifstream jsonStrm(configPath);
    if (!jsonStrm.is_open()) {
        INTELL_VOICE_LOG_ERROR("open file fail!");
        return "";
    }
    Json::Value wakeupJson;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;
    std::string errs;
    if (!parseFromStream(reader, jsonStrm, &wakeupJson, &errs)) {
        INTELL_VOICE_LOG_ERROR("parseFromStream json fail!");
        return "";
    }

    if (IsDefaltPhrase(wakeupJson, wakeupPhrase)) {
        return ParseDefaultDspSensibility(wakeupJson, wakeupPhrase, index);
    }

    return ParseUserDspSensibility(wakeupJson, index);
}

std::string IntellVoiceSensibility::ParseDefaultDspSensibility(const Json::Value &wakeupJson,
    const std::string &wakeupPhrase, uint32_t index)
{
    if ((wakeupJson.isMember(WAKEUP_CONF_DEFAULPHRASE)) &&
        (wakeupJson[WAKEUP_CONF_DEFAULPHRASE].isMember(wakeupPhrase))) {
        Json::Value sensibilityArray = wakeupJson[WAKEUP_CONF_DEFAULPHRASE][wakeupPhrase];
        if ((sensibilityArray.isArray()) && (sensibilityArray.size() > index) && (sensibilityArray[index].isInt())) {
            return std::to_string(sensibilityArray[index].asInt());
        }
        INTELL_VOICE_LOG_WARN("index:%{public}u, default sensibility is invalid", index);
        return "";
    }
    if ((!wakeupJson.isMember("SensibilityParams")) ||
        (!wakeupJson["SensibilityParams"].isMember("DspSentenceThresholds"))) {
        INTELL_VOICE_LOG_WARN("no sensiblity param");
        return "";
    }

    Json::Value sensibilityParam = wakeupJson["SensibilityParams"]["DspSentenceThresholds"];
    if ((sensibilityParam.isMember("Default")) && (sensibilityParam["Default"].isArray()) &&
        (sensibilityParam["Default"].size() > index) && sensibilityParam["Default"][index].isInt()) {
        return std::to_string(sensibilityParam["Default"][index].asInt());
    }

    INTELL_VOICE_LOG_WARN("index:%{public}u, default sensibility param is invalid", index);
    return "";
}

std::string IntellVoiceSensibility::ParseUserDspSensibility(const Json::Value &wakeupJson, uint32_t index)
{
    if ((!wakeupJson.isMember("SensibilityParams")) ||
        (!wakeupJson["SensibilityParams"].isMember("DspSentenceThresholds"))) {
        INTELL_VOICE_LOG_WARN("no sensiblity param");
        return "";
    }

    Json::Value sensibilityParam = wakeupJson["SensibilityParams"]["DspSentenceThresholds"];
    if ((sensibilityParam.isMember("UserDefined")) && (sensibilityParam["UserDefined"].isArray()) &&
        (sensibilityParam["UserDefined"].size() > index) && sensibilityParam["UserDefined"][index].isInt()) {
        return std::to_string(sensibilityParam["Default"][index].asInt());
    }

    INTELL_VOICE_LOG_WARN("index:%{public}u, default sensibility param is invalid", index);
    return "";
}

bool IntellVoiceSensibility::IsDefaltPhrase(const Json::Value &wakeupJson, const std::string &wakeupPhrase)
{
    const std::vector<std::string> defaultPhrases = {"你好小E", "你好小易", "你好华为", "小艺小艺", "你好悠悠"};
    if (wakeupJson.isMember(WAKEUP_CONF_DEFAULPHRASE)) {
        if (wakeupJson[WAKEUP_CONF_DEFAULPHRASE].isMember(wakeupPhrase)) {
            return true;
        }
    } else {
        auto it = std::find(defaultPhrases.begin(), defaultPhrases.end(), wakeupPhrase);
        if (it != defaultPhrases.end()) {
            return true;
        }
    }

    return false;
}

bool IntellVoiceSensibility::IsSupportNodes800(const std::string &dspFeature)
{
    if (dspFeature.empty()) {
        return false;
    }

    int32_t feature = 0;
    if (!StringUtil::StringToInt(dspFeature, feature)) {
        INTELL_VOICE_LOG_ERROR("failed to get dsp feature");
        return false;
    }

    if ((feature & HAL_FEATURE_KWS_NODES_900) == HAL_FEATURE_KWS_NODES_900) {
        return true;
    }

    return false;
}
}
}