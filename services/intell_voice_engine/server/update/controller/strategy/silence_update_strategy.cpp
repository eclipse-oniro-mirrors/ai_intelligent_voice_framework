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
#include "silence_update_strategy.h"
#include <fstream>
#include "securec.h"
#include "intell_voice_log.h"
#include "scope_guard.h"
#include "update_engine_utils.h"
#include "intell_voice_util.h"
#include "engine_offline_audio_processor.h"

#define LOG_TAG "SilenceUpdateStrategy"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int32_t SILENCE_UPDATE_RETRY_TIMES = 5;
static const std::string PCM_PREFIX = "/data/service/el0/intelligent_voice/wakeup/enrollAudioBackup/";

SilenceUpdateStrategy::SilenceUpdateStrategy(const std::string &param): IUpdateStrategy(param)
{
}

SilenceUpdateStrategy::~SilenceUpdateStrategy()
{
}

bool SilenceUpdateStrategy::UpdateRestrain()
{
    return !UpdateEngineUtils::IsVersionUpdate();
}

UpdatePriority SilenceUpdateStrategy::GetUpdatePriority()
{
    return SILENCE_UPDATE_PRIORITY;
}

int SilenceUpdateStrategy::GetRetryTimes()
{
    return SILENCE_UPDATE_RETRY_TIMES;
}

int SilenceUpdateStrategy::OnUpdateCompleteCallback(const int result, bool isLast)
{
    if (!isLast || result == 0) {
        bool tse_support = IntellVoiceUtil::GetSupportAbility(ABILITY_TSE);
        if (tse_support) {
            // Create processor instance
            auto processor = std::make_shared<IntelligentVoice::Engine::IntellVoiceEngine::OfflineAudioProcessor>();
            INTELL_VOICE_LOG_INFO("GetSupportAbility_tse_support: true, audio_asr_tse_reg_enter");
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
        return 0;
    }

    if (result == UPDATE_STATE_CONCURRENT) {
        INTELL_VOICE_LOG_INFO("update engine conrrent");
        return 0;
    }
    INTELL_VOICE_LOG_INFO("notify silence update fail");
    IntellVoiceUtil::StartAbility("update_event");
    return  0;
}
}
}