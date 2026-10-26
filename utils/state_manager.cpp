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
#include "state_manager.h"
#include "intell_voice_log.h"

#undef LOG_TAG
#define LOG_TAG "StateManger"

namespace OHOS {
namespace IntellVoiceUtils {
StateActions g_nullAction;
const State NULLSTATE(-1);

int StateActions::Handle(const StateMsg &msg, State &nextState)
{
    if (actions.find(msg.msgId) == actions.end()) {
        return NO_PROCESS_RET;
    }
    return (actions[msg.msgId])(msg, nextState);
}

ModuleStates::ModuleStates(const State &defaultState, const std::string &name, const std::string &threadName)
    : isInitSucc_(true), name_(name)
{
    states_[defaultState] = nullptr;
    currState_ = states_.begin();
    Start(threadName);
}

ModuleStates::~ModuleStates()
{
    for (auto each : states_) {
        if (each.second != nullptr) {
            delete each.second;
        }
    }

    states_.clear();
}

StateActions& ModuleStates::ForState(const State &s)
{
    auto it = states_.find(s);
    if (it == states_.end() || it->second == nullptr) {
        StateActions* action = new (std::nothrow) StateActions();
        if (action == nullptr) {
            currState_ = states_.end();
            isInitSucc_ = false;
            return g_nullAction;
        }
        states_[s] = action;
    }

    return *(states_[s]);
}

StateActions& ModuleStates::ForState(int simpleState)
{
    return ForState(State(simpleState));
}

StateGroup& ModuleStates::FromState(int simpleStateStart, int simpleStateEnd)
{
    for (int i = simpleStateStart; i <= simpleStateEnd; i++) {
        StateActions& action = ForState(i);
        AddAction(&action);
    }

    return *this;
}

void ModuleStates::ToState(std::map<State, StateActions*>::iterator &nextIt)
{
    // exit current state
    StateActions* action = currState_->second;
    if (action->timerId != INVALID_ID) {
        KillTimer(action->timerId);
    }

    INTELL_VOICE_LOG_INFO("%{public}s state from %{public}s to %{public}s", name_.c_str(),
        (currState_->first).ToStr().c_str(), (nextIt->first).ToStr().c_str());

    currState_ = nextIt;

    // enter next state
    action = nextIt->second;
    if (action->cfg.delayUs != 0) {
        action->timerId = SetTimer(action->cfg.type, action->cfg.delayUs, action->cfg.cookie, this);
    }
    if (stateListener_ != nullptr) {
        stateListener_->OnStateChange(nextIt->first, processType_);
    }
}

void ModuleStates::OnTimerEvent(TimerEvent &info)
{
    StateMsg msg(info.type, &info, sizeof(info));
    HandleMsg(msg);
}

int ModuleStates::HandleMsg(const StateMsg &msg)
{
    std::unique_lock<std::mutex> lock(msgHandleMutex_);

    if ((currState_ == states_.end()) || (currState_->second == nullptr)) {
        INTELL_VOICE_LOG_ERROR("%{public}s invalid current state", name_.c_str());
        return -1;
    }

    State nextState = currState_->first;
    int ret = currState_->second->Handle(msg, nextState);
    if (ret != 0) {
        if (ret != NO_PROCESS_RET) {
            INTELL_VOICE_LOG_ERROR("%{public}s handle msg %{public}d fail", name_.c_str(), msg.msgId);
        } else {
            INTELL_VOICE_LOG_INFO("%{public}s state %{public}d no need to handle msg %{public}d", name_.c_str(),
                currState_->first.state, msg.msgId);
        }
        return ret;
    }

    if (currState_->first == nextState) {
        return 0;
    }

    auto nextIt = states_.find(nextState);
    if (nextIt == states_.end()) {
        INTELL_VOICE_LOG_ERROR("%{public}s invalid next state %{public}s", name_.c_str(), nextState.ToStr().c_str());
        return 0;
    }

    ToState(nextIt);

    return 0;
}

void ModuleStates::ResetTimerDelay()
{
    if (currState_ == states_.end()) {
        INTELL_VOICE_LOG_ERROR("invalid current state");
        return;
    }
    StateActions *action = currState_->second;
    if (action == nullptr) {
        INTELL_VOICE_LOG_ERROR("action of state:%{public}d is nullptr", currState_->first.state);
        return;
    }
    if ((action->timerId == INVALID_ID) || (action->cfg.delayUs == 0)) {
        INTELL_VOICE_LOG_INFO("no valid timer to reset");
        return;
    }
    action->timerId = ResetTimer(action->timerId, action->cfg.type, action->cfg.delayUs, action->cfg.cookie, this);
}

State ModuleStates::CurrState() const
{
    if (currState_ == states_.end()) {
        return NULLSTATE;
    }

    return currState_->first;
}

bool ModuleStates::IsStatesInitSucc() const
{
    return isInitSucc_;
}
}
}