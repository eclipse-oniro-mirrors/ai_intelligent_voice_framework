/*
 * Copyright (c) 2023 Huawei Device Co., Ltd. 2023-2023.
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
#ifndef CLONE_UPDATE_STRATEGY_H
#define CLONE_UPDATE_STRATEGY_H

#include <thread>
#include "update_strategy.h"
#include "i_intell_voice_update_callback.h"

namespace OHOS {
namespace IntellVoiceEngine {
using namespace OHOS::IntellVoice;
class CloneUpdateStrategy final: public IUpdateStrategy {
public:
    ~CloneUpdateStrategy() override;
    CloneUpdateStrategy(const std::string &param, sptr<IIntelligentVoiceUpdateCallback> updateCallback);
    bool UpdateRestrain() override;
    UpdatePriority GetUpdatePriority() override;
    int GetRetryTimes() override;
    int OnUpdateCompleteCallback(const int result, bool isLast) override;

private:
    void SetBundleAndAbilityName();
    std::string GetBundleOrAbilityName(const std::string &key);
private:
    sptr<IIntelligentVoiceUpdateCallback> updateCallback_ = nullptr;
};
}
}
#endif
