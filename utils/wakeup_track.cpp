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
#include "wakeup_track.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "intell_voice_log.h"

#define LOG_TAG "WakeupTrack"

constexpr const char* DEVICE_PATH = "/dev/audio_wakeup_track";

namespace OHOS {
namespace IntellVoiceUtils {
bool WakeupTrack::sHasFailed = false;
bool WakeupTrack::reportWakeupStart(StageCode stage, WakeupResult result, WakeupErrorCode errCode)
{
    if (sHasFailed) {
        INTELL_VOICE_LOG_WARN("skip\n");
        return false;
    }

    int fd = open(DEVICE_PATH, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        INTELL_VOICE_LOG_ERROR("open audio_wakeup_track failed\n");
        sHasFailed = true;
        return false;
    }
    
    WakeupTrackInfo info = {stage, result, errCode};
    int ret = ioctl(fd, IOCTL_AUDIO_WAKEUP_TRACK_START_CHECK_CMD, &info);
    close(fd);
    
    if (ret < 0) {
        INTELL_VOICE_LOG_ERROR("ioctl failed\n");
        return false;
    }
    
    return true;
}

bool WakeupTrack::reportWakeupTrack(StageCode stage, WakeupResult result, WakeupErrorCode errCode)
{
    if (sHasFailed) {
        return false;
    }

    int fd = open(DEVICE_PATH, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        INTELL_VOICE_LOG_ERROR("open audio_wakeup_track failed\n");
        sHasFailed = true;
        return false;
    }
    
    WakeupTrackInfo info = {stage, result, errCode};
    int ret = ioctl(fd, IOCTL_AUDIO_WAKEUP_TRACK_REPORT_CMD, &info);
    close(fd);
    
    if (ret < 0) {
        INTELL_VOICE_LOG_ERROR("ioctl failed\n");
        return false;
    }
    
    return true;
}

bool WakeupTrack::reportWakeupStatus(SwitchStage stage, SwitchState state, SwitchReason reason)
{
    if (sHasFailed) {
        INTELL_VOICE_LOG_WARN("skip\n");
        return false;
    }
    static SwitchStage lastStage = SwitchStage::WAKEUP_SWITCH_STAGE_DEFAULT;
    static SwitchState lastState = SwitchState::WAKEUP_SWITCH_STATE_CLOSE;
    static SwitchReason lastReason = SwitchReason::WAKEUP_SWITCH_REASON_DEFAULT;
    static bool hasLastReport = false;
    
    if (hasLastReport && lastStage == stage && lastState == state && lastReason == reason) {
        return true;
    }

    int fd = open(DEVICE_PATH, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        INTELL_VOICE_LOG_ERROR("open audio_wakeup_track failed\n");
        sHasFailed = true;
        return false;
    }
    
    WakeupSwitchInfo info = {stage, state, reason};
    int ret = ioctl(fd, IOCTL_AUDIO_WAKEUP_SWITCH_REPORT_CMD, &info);
    close(fd);
    
    if (ret < 0) {
        INTELL_VOICE_LOG_ERROR("ioctl failed\n");
        return false;
    }
    lastStage = stage;
    lastState = state;
    lastReason = reason;
    hasLastReport = true;
    return true;
}
}
}