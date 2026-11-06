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
#include "update_engine_controller.h"
#include <fstream>
#include "securec.h"
#include "intell_voice_log.h"

#include "update_adapter_listener.h"
#include "history_info_mgr.h"
#include "time_util.h"
#include "scope_guard.h"
#include "engine_callback_message.h"
#include "adapter_callback_service.h"
#include "string_util.h"
#include "intell_voice_definitions.h"

#define LOG_TAG "UpdateEngineController"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr int32_t MS_PER_S = 1000;

std::atomic<bool> UpdateEngineController::isUpdating_ = false;

UpdateEngineController::UpdateEngineController()
{
}

UpdateEngineController::~UpdateEngineController()
{
}

void UpdateEngineController::OnTimerEvent(TimerEvent &info)
{
    INTELL_VOICE_LOG_INFO("TimerEvent %{public}d", timerId_);
    EngineCallbackMessage::CallFunc(HANDLE_UPDATE_RETRY);
}

bool UpdateEngineController::UpdateRetryProc()
{
    std::lock_guard<std::mutex> lock(updateEngineMutex_);
    if (isForceReleased_) {
        INTELL_VOICE_LOG_WARN("force to stop, no need to retry");
        return true;
    }
    if (updateStrategy_ != nullptr && !updateStrategy_->UpdateRestrain()) {
        if (CreateUpdateEngine(updateStrategy_->param_)) {
            INTELL_VOICE_LOG_INFO("retry update, times %{public}d", retryTimes_);
            timerId_ = INVALID_ID;
            return true;
        }
        updateResult_ = UpdateState::UPDATE_STATE_COMPLETE_FAIL;
    } else {
        updateResult_ = UpdateState::UPDATE_STATE_COMPLETE_SUCCESS;
    }

    INTELL_VOICE_LOG_INFO("retry err, times %{public}d, result %{public}d", retryTimes_, updateResult_);
    ClearRetryState();
    ReleaseUpdateEngine();
    TimerMgr::Stop();
    return false;
}

int UpdateEngineController::UpdateArbitration(int priority)
{
    if (!GetUpdateState()) {
        return 0;
    }

    if (curPriority_ > priority) {
        INTELL_VOICE_LOG_INFO("cur priority is higher than new, can not create");
        return -1;
    }

    if (curPriority_ == priority) {
        INTELL_VOICE_LOG_INFO("same priority, retry update process");
        return 0;
    }

    INTELL_VOICE_LOG_INFO("cur priority is lower than before, release old engine");
    if (updateStrategy_ != nullptr) {
        updateStrategy_->OnUpdateCompleteCallback(UPDATE_STATE_CONCURRENT, true);
    }

    ReleaseUpdateEngine();
    ClearRetryState();
    TimerMgr::Stop();
    return 0;
}

int UpdateEngineController::CreateUpdateEngineUntilTime(std::shared_ptr<IUpdateStrategy> updateStrategy, bool reEnroll)
{
    std::lock_guard<std::mutex> lock(updateEngineMutex_);
    INTELL_VOICE_LOG_INFO("enter");
    if (updateStrategy == nullptr) {
        INTELL_VOICE_LOG_INFO("strategy is null");
        return -1;
    }

    if (updateStrategy->UpdateRestrain()) {
        INTELL_VOICE_LOG_INFO("update restrain, no need to update");
        return -1;
    }

    if (UpdateArbitration(updateStrategy->GetUpdatePriority()) != 0) {
        return -1;
    }

    if (!CreateUpdateEngine(updateStrategy->param_, reEnroll)) {
        INTELL_VOICE_LOG_ERROR("create update engine error");
        return -1;
    }

    retryTimesLimit_ = updateStrategy->GetRetryTimes();
    if (retryTimes_ < retryTimesLimit_) {
        TimerMgr::Start("UpdateThread", nullptr);
    }

    updateStrategy_ = updateStrategy;
    curPriority_ = updateStrategy->GetUpdatePriority();
    SetUpdateState(true);
    isForceReleased_ = false;

    INTELL_VOICE_LOG_INFO("create update engine success");
    return 0;
}

bool UpdateEngineController::StartUpdateTimer()
{
    if (timerId_ != INVALID_ID) {
        INTELL_VOICE_LOG_INFO("timer already start %{public}d", timerId_);
        return true;
    }

    timerId_ = SetTimer(0, delaySecond_ * MS_PER_S * US_PER_MS, 0, this);
    if (timerId_ == INVALID_ID) {
        INTELL_VOICE_LOG_ERROR("timerId %{public}d is invalid", timerId_);
        return false;
    }

    retryTimes_++;
    INTELL_VOICE_LOG_INFO("start update timer succeed");
    return true;
}

void UpdateEngineController::StopUpdateTimer()
{
    if (timerId_ == INVALID_ID) {
        INTELL_VOICE_LOG_INFO("timer already stop");
        return;
    }

    KillTimer(timerId_);
    INTELL_VOICE_LOG_INFO("stop update timer");
}

bool UpdateEngineController::IsNeedRetryUpdate()
{
    if (updateResult_ == UpdateState::UPDATE_STATE_COMPLETE_SUCCESS) {
        INTELL_VOICE_LOG_INFO("update success");
        return false;
    }

    if (retryTimes_ >= retryTimesLimit_) {
        INTELL_VOICE_LOG_INFO("retry times limit");
        return false;
    }

    if ((updateStrategy_ == nullptr) || (updateStrategy_->UpdateRestrain())) {
        INTELL_VOICE_LOG_INFO("no need to retry");
        return false;
    }

    return true;
}

void UpdateEngineController::ClearRetryState()
{
    retryTimes_ = 0;
    updateResult_ = UpdateState::UPDATE_STATE_DEFAULT;
    timerId_ = INVALID_ID;
    updateStrategy_ = nullptr;
    retryTimesLimit_ = 0;
    curPriority_ = 0;
    SetUpdateState(false);
}

void UpdateEngineController::UpdateCompleteProc(UpdateState result, const std::string &param, bool &isLast)
{
    std::lock_guard<std::mutex> lock(updateEngineMutex_);
    isLast = false;
    if (isForceReleased_) {
        HistoryInfoMgr::GetInstance().DeleteKey({ KEY_WAKEUP_VESRION });
        INTELL_VOICE_LOG_WARN("already stop");
        return;
    }
    if (updateStrategy_ == nullptr || param != updateStrategy_->param_) {
        INTELL_VOICE_LOG_WARN("param is not equal");
        return;
    }
    updateResult_ = result;
    StopUpdateTimer();

    if (IsNeedRetryUpdate() && StartUpdateTimer()) {
        INTELL_VOICE_LOG_INFO("retry to update");
    } else {
        if (updateStrategy_ != nullptr) {
            updateStrategy_->OnUpdateCompleteCallback(result != UpdateState::UPDATE_STATE_COMPLETE_SUCCESS, true);
        }
        ClearRetryState();
        isLast = true;
        TimerMgr::Stop();
    }
    ReleaseUpdateEngine();
}

void UpdateEngineController::ForceRelease()
{
    std::lock_guard<std::mutex> lock(updateEngineMutex_);
    if (!GetUpdateState()) {
        INTELL_VOICE_LOG_INFO("no need to stop update");
        return;
    }
    ClearRetryState();
    ReleaseUpdateEngine();
    TimerMgr::Stop();
    isForceReleased_ = true;
}
}
}