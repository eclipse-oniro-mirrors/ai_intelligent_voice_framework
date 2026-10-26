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
#ifndef SWITCH_PROVIDER_H
#define SWITCH_PROVIDER_H

#include "switch_observer.h"
#include "datashare_helper.h"
#include "intell_voice_generic_factory.h"

namespace OHOS {
namespace IntellVoiceEngine {
class SwitchProvider {
public:
    bool Init();
    void RegisterObserver(const sptr<SwitchObserver> &observer, const std::string &key);
    void UnregisterObserver(const sptr<SwitchObserver> &observer, const std::string &key);
    bool QuerySwitchStatus(const std::string &key);
    void QueryAndLogReason();
    bool IsSwitchError(const std::string &key);
    static bool CheckIfDataShareReady();

private:
    SwitchProvider();
    ~SwitchProvider();
    std::shared_ptr<DataShare::DataShareResultSet> QueryWithRetry(const std::string &key);

    std::shared_ptr<DataShare::DataShareHelper> helper_ = nullptr;
    friend class IntellVoiceUtils::UniquePtrFactory<SwitchProvider>;
};
}  // namespace IntellVoice
}  // namespace OHOS
#endif