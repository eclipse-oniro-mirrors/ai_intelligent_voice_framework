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
#ifndef UPDATE_STATE_H
#define UPDATE_STATE_H

namespace OHOS {
namespace IntellVoiceEngine {
#define UPDATE_DELAY_TIME_SECONDS  30

enum UpdateState {
    UPDATE_STATE_DEFAULT = -1,
    UPDATE_STATE_COMPLETE_SUCCESS,
    UPDATE_STATE_COMPLETE_FAIL,
    UPDATE_STATE_CONCURRENT,
    UPDATE_STATE_BUTT,
};

enum UpdatePriority {
    UPDATE_PRIORITY_DEFAULT = 0,
    CLOUD_UPDATE_PRIORITY = 1,
    CLONE_UPDATE_PRIORITY,
    SILENCE_UPDATE_PRIORITY,
    WHISPER_UPDATE_PRIORITY,
    UPDATE_PRIORITY_BUTT,
};
}
}
#endif
