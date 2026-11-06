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
#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <iostream>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <string>
#include <functional>
#include "timer_mgr.h"

namespace OHOS {
namespace IntellVoiceUtils {
constexpr int NO_PROCESS_RET = -2;

struct StateMsg {
public:
    int32_t msgId;
    void *inMsg = nullptr;
    int32_t inMsgLen;
    void *outMsg = nullptr;

public:
    explicit StateMsg(int32_t id, void *in = nullptr, int32_t inLen = 0, void *out = nullptr)
        : msgId(id), inMsg(in), inMsgLen(inLen), outMsg(out) {};
};

struct State {
    State() = default;
    explicit State(int s) : state(s) {};

    bool operator==(const State &right) const
    {
        return state == right.state;
    };

    bool operator!=(const State &right) const
    {
        return !(*this == right);
    };

    bool operator<(const State &right) const
    {
        return state < right.state;
    };

    std::string ToStr() const
    {
        return std::to_string(state);
    };

    int32_t state = -1;
};

enum ProcessType {
    PROCESS_IDLE = 0,
    PROCESS_WAKEUP_PHONE = 1,
    PROCESS_WAKEUP_HEADSET = 2,
    PROCESS_WAKEUP_GLASSES = 3,
    PROCESS_WISPHER_PHONE = 4,
};

using StateChangeCallback = std::function<void(const State &stateInfo, const ProcessType &type)>;

class StateInfoListener {
public:
    explicit StateInfoListener() = default;
    void SetCallback(StateChangeCallback callback)
    {
        callback_ = callback;
    }
    void OnStateChange(State state, ProcessType type)
    {
        if (callback_ != nullptr) {
            callback_(state, type);
        }
    }

private:
    StateChangeCallback callback_;
};

struct StateInfo {
    virtual ~StateInfo(){};
    TimerCfg cfg;
    int timerId = INVALID_ID;
};

using HandleMsg = std::function<int(const StateMsg &msg, State &nextState)>;
struct ModuleStates;

struct StateActions : public StateInfo {
    ~StateActions() override {};
    StateActions() = default;
    bool operator==(StateActions& right) const
    {
        return actions.size() == right.actions.size();
    }

    StateActions& Add(int msgid, HandleMsg handler)
    {
        actions[msgid] = handler;
        return *this;
    };

    StateActions& Del(int msgid)
    {
        actions.erase(msgid);
        return *this;
    };

    StateActions& WaitUntil(int type, HandleMsg handler, int64_t delayUs, int cookie = 0)
    {
        cfg.type = type;
        cfg.delayUs = delayUs;
        cfg.cookie = cookie;

        Add(type, handler);

        return *this;
    };

    int Handle(const StateMsg &msg, State &nextState);

private:
    std::map<int32_t, HandleMsg> actions;
};

struct StateGroup {
public:
    virtual ~StateGroup()
    {
        ClearAction();
    }

    void AddAction(StateActions *action)
    {
        mActions.push_back(action);
    }

    void ClearAction()
    {
        mActions.clear();
    }

    StateGroup& Add(int msgid, HandleMsg handler)
    {
        for (auto each : mActions) {
            each->Add(msgid, handler);
        }
        return *this;
    }

    StateGroup& Del(int msgid)
    {
        for (auto each : mActions) {
            each->Del(msgid);
        }
        return *this;
    }
private:
    std::vector<StateActions*> mActions;
};

struct ModuleStates : public ITimerObserver, private TimerMgr, private StateGroup {
    explicit ModuleStates(const State &defaultState = State(0), const std::string &name = "",
        const std::string &threadName = "StateThread");
    ~ModuleStates() override;

    StateActions& ForState(const State &s);
    StateActions& ForState(int simpleState);
    StateGroup& FromState(int simpleStateStart, int simpleStateEnd);

    int HandleMsg(const StateMsg &msg);
    void ResetTimerDelay();
    bool IsStatesInitSucc() const;
    State CurrState() const;

protected:
    void ToState(std::map<State, StateActions*>::iterator &nextIt);
    void OnTimerEvent(TimerEvent &info) override;
    void SetStateListener(std::shared_ptr<StateInfoListener> stateListener)
    {
        stateListener_ = stateListener;
    }
    void SetProcessType(ProcessType processType)
    {
        processType_ = processType;
    }

protected:
    std::map<State, StateActions*>::iterator currState_;

private:
    std::mutex msgHandleMutex_;
    bool isInitSucc_ = false;
    std::map<State, StateActions*> states_;
    std::string name_;
    ProcessType processType_;
    std::shared_ptr<StateInfoListener> stateListener_ = nullptr;
};

#define ADDR(func) ([this](const StateMsg &msg, State &nextState)->int { return this->func(msg, nextState); })
#define ACT(msgid, func) Add(msgid, ADDR(func))
}
}

#endif /* STATE_MANAGER_H */
