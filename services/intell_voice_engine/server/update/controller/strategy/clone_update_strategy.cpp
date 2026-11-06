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
#include "clone_update_strategy.h"
#include <fstream>
#include "securec.h"
#include "intell_voice_log.h"
#include "scope_guard.h"
#include "update_engine_utils.h"
#include "history_info_mgr.h"
#include "json/json.h"
#include "intell_voice_definitions.h"
#include "engine_offline_audio_processor.h"

#define LOG_TAG "CloneUpdateStrategy"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static const std::string PCM_PREFIX = "/data/service/el0/intelligent_voice/wakeup/enrollAudioBackup/";

CloneUpdateStrategy::CloneUpdateStrategy(const std::string &param,
    sptr<IIntelligentVoiceUpdateCallback> updateCallback): IUpdateStrategy(param), updateCallback_(updateCallback)
{
}

CloneUpdateStrategy::~CloneUpdateStrategy()
{
}

bool CloneUpdateStrategy::UpdateRestrain()
{
    std::string versionNumberSave;
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();

    versionNumberSave = historyInfoMgr.GetStringKVPair(KEY_WAKEUP_VESRION);
    if (!versionNumberSave.empty()) {
        INTELL_VOICE_LOG_ERROR("saved version number is not null");
        return true;
    }
    return false;
}

UpdatePriority CloneUpdateStrategy::GetUpdatePriority()
{
    return CLONE_UPDATE_PRIORITY;
}

int CloneUpdateStrategy::GetRetryTimes()
{
    return 0;
}

std::string CloneUpdateStrategy::GetBundleOrAbilityName(const std::string &key)
{
    std::istringstream jsonStrm(param_);
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;

    reader["collectComments"] = false;
    if (!parseFromStream(reader, jsonStrm, &root, &errs)) {
        INTELL_VOICE_LOG_ERROR("input str is not json");
        return "";
    }

    if ((!root.isMember(key)) || (!root[key].isString())) {
        INTELL_VOICE_LOG_ERROR("invalid key");
        return "";
    }

    return root[key].asString();
}

void CloneUpdateStrategy::SetBundleAndAbilityName()
{
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();

    std::string bundleName = GetBundleOrAbilityName("bundle_name");
    if (!bundleName.empty()) {
        INTELL_VOICE_LOG_INFO("set bundle");
        historyInfoMgr.SetStringKVPair(KEY_WAKEUP_ENGINE_BUNDLE_NAME, bundleName);
    }

    std::string abilityName = GetBundleOrAbilityName("ability_name");
    if (!abilityName.empty()) {
        INTELL_VOICE_LOG_INFO("set ability");
        historyInfoMgr.SetStringKVPair(KEY_WAKEUP_ENGINE_ABILITY_NAME, abilityName);
    }
}

int CloneUpdateStrategy::OnUpdateCompleteCallback(const int result, bool isLast)
{
    if (updateCallback_ == nullptr) {
        return 0;
    }
    INTELL_VOICE_LOG_INFO("enter");
    if (result == UPDATE_STATE_COMPLETE_SUCCESS) {
        SetBundleAndAbilityName();
        bool tse_support = IntellVoiceUtil::GetSupportAbility(ABILITY_TSE);
        if (tse_support) {
            // Create processor instance
            INTELL_VOICE_LOG_INFO("GetSupportAbility_tse_support: true, audio_asr_tse_reg_enter");
            auto processor = std::make_shared<IntelligentVoice::Engine::IntellVoiceEngine::OfflineAudioProcessor>();
            INTELL_VOICE_LOG_INFO("audio_asr_clone_enter");

            std::string fPath1 = PCM_PREFIX + "enroll_1.pcm";
            std::string fPath2 = PCM_PREFIX + "enroll_2.pcm";
            std::string fPath3 = PCM_PREFIX + "enroll_3.pcm";
            std::string fPath4 = PCM_PREFIX + "enroll_phrase.txt";
            std::thread t([processor, fPath1, fPath2, fPath3, fPath4]() {
                processor->ProcessOfflineAudioWhile(fPath1, fPath2, fPath3, fPath4);
            });
            t.join();
        } else {
            INTELL_VOICE_LOG_INFO("GetSupportAbility_tse_support: false");
        }
    }
    updateCallback_->OnUpdateComplete(result);

    return  0;
}
}
}
