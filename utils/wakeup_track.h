/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef WAKEUP_TRACK_H
#define WAKEUP_TRACK_H

#define IOCTL_AUDIO_WAKEUP_TRACK_START_CHECK_CMD     _IO('A', 0x01)
#define IOCTL_AUDIO_WAKEUP_TRACK_REPORT_CMD          _IO('A', 0x02)
#define IOCTL_AUDIO_WAKEUP_SWITCH_REPORT_CMD         _IO('A', 0x03)
namespace OHOS {
namespace IntellVoiceUtils {
enum StageCode {
    WAKEUP_STAGE_SA_WAKEUP_DETECTED = 0x0000,
    WAKEUP_STAGE_SA_WHISPER_DETECTED,
    WAKEUP_STAGE_SA_HANDLE_DETECTED,
    WAKEUP_STAGE_SA_RECOGNIZING,
    WAKEUP_STAGE_SA_RECOGNIZED,
    WAKEUP_STAGE_SA_RECOGNIZED_COMPLETE,
    WAKEUP_STAGE_SA_ONESHOT_START,
    WAKEUP_STAGE_SA_ONESHOT_PROCESSE,
    WAKEUP_STAGE_SA_ONESHOT_END,
    WAKEUP_STAGE_SA_BOTTOM = 0x0050
};

enum WakeupResult {
    WAKEUP_RESULT_DEFAULT,
    WAKEUP_RESULT_SUCC,
    WAKEUP_RESULT_FAIL
};

enum WakeupErrorCode {
    WAKEUP_ERROR_CODE_HAL = 0x0000,
    SA_INPUT_ERROR,
    SA_HOST_ERROR,
    SA_TIMEOUT_ERROR,
    SA_AUDIO_SOURCE_ERROR,
    SA_CALLBACK_ERROR,
    WAKEUP_ERROR_CODE_SA_BOTTOM = 0x0500,
    WAKEUP_ERROR_CODE_HAL_BOTTOM = 0x1000,
    WAKEUP_ERROR_CODE_END = 0x3000
};

enum SwitchState {
    WAKEUP_SWITCH_STATE_CLOSE,
    WAKEUP_SWITCH_STATE_OPEN,
};

enum SwitchStage {
    WAKEUP_SWITCH_STAGE_DEFAULT,
    WAKEUP_SWITCH_STAGE_SA,
    WAKEUP_SWITCH_STAGE_HAL
};

enum SwitchReason {
    WAKEUP_SWITCH_REASON_INIT,
    WAKEUP_SWITCH_REASON_POWER_SAVE_MODE,
    WAKEUP_SWITCH_REASON_USER_SETTING,
    WAKEUP_SWITCH_REASON_SWITCH_DEVICE,
    WAKEUP_SWITCH_REASON_AUDIO_SCENE_CHANGE,
    WAKEUP_SWITCH_REASON_DEFAULT
};

struct WakeupTrackInfo {
    enum StageCode stage;
    enum WakeupResult result;
    enum WakeupErrorCode err_code;
};
 
struct WakeupSwitchInfo {
    enum SwitchStage stage;
	enum SwitchState state;
    enum SwitchReason reason;
};

class WakeupTrack {
public:
    static bool reportWakeupStart(StageCode stage, WakeupResult result, WakeupErrorCode errCode);
    static bool reportWakeupTrack(StageCode stage, WakeupResult result, WakeupErrorCode errCode);
    static bool reportWakeupStatus(SwitchStage stage, SwitchState state, SwitchReason reason);
private:
    static bool sHasFailed;
};
}
}
#endif