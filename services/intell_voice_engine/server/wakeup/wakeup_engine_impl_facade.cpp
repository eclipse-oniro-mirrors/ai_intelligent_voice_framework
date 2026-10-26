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
#include "wakeup_engine_impl_facade.h"
#include "intell_voice_log.h"
#include "headset_wakeup_engine_impl.h"
#include "glasses_wakeup_engine_impl.h"
#include "wakeup_engine_impl.h"
#include "engine_device_manager.h"
#include "engine_callback_message.h"
#include "intell_voice_util.h"
#include "intell_voice_definitions.h"
 
#undef LOG_TAG
#define LOG_TAG "WakeupEngineImplFacade"
 
using namespace OHOS::IntellVoiceUtils;
using namespace std;
 
namespace OHOS {
namespace IntellVoiceEngine {
 
WakeupEngineImplFacade::WakeupEngineImplFacade()
{
}
 
WakeupEngineImplFacade::~WakeupEngineImplFacade()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (deviceChangeCbId_ != 0) {
        EngineDeviceManager::GetInstance().UnregisterDeviceChangeCb(deviceChangeCbId_);
        deviceChangeCbId_ = 0;
    }
    {
        std::lock_guard<std::mutex> lock(deviceMutex_);
        ReleaseEngineImpl();
    }
}
 
bool WakeupEngineImplFacade::Init()
{
    INTELL_VOICE_LOG_INFO("enter");
    std::lock_guard<std::mutex> lock(deviceMutex_);
    stateListener_ = std::make_shared<StateInfoListener>();
    if (stateListener_ == nullptr) {
        INTELL_VOICE_LOG_INFO("create state listener failed");
        return false;
    }
    stateListener_->SetCallback([this](const State &stateInfo, const ProcessType &type) {
        INTELL_VOICE_LOG_INFO("enter");
        OnStateChange(stateInfo, type);
    });
    if (InitDefaultImpl() != 0) {
        return false;
    }
    EngineDeviceManager &deviceManager = EngineDeviceManager::GetInstance();
    if (deviceChangeCbId_ != 0) {
        INTELL_VOICE_LOG_WARN("deviceChangeCb already registered, id:%{public}lu, unregister first",
            static_cast<unsigned long>(deviceChangeCbId_));
        deviceManager.UnregisterDeviceChangeCb(deviceChangeCbId_);
        deviceChangeCbId_ = 0;
    }
    auto weakThis = GetWeakRef();
    deviceChangeCbId_ = deviceManager.RegisterDeviceChangeCb(
        [weakThis, this](const std::shared_ptr<WakeupDeviceDetail> &info) {
            sptr<RefBase> sp = weakThis.promote();
            if (sp == nullptr) {
                INTELL_VOICE_LOG_WARN("facade already destroyed, skip callback");
                return;
            }
            UpdateWakeupEngineImpl(info);
        });
    if (deviceChangeCbId_ == 0) {
        INTELL_VOICE_LOG_ERROR("register device change callback failed");
        return false;
    }
    wakeupDevice_ = deviceManager.GetWakeupDevice();
    if (CreateEngineImpl() != 0) {
        return false;
    }
    return true;
}

void WakeupEngineImplFacade::SaveCallback(sptr<IIntelligentVoiceEngineCallback> callback)
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    INTELL_VOICE_LOG_INFO("enter");
    callback_ = callback;
    SetListenerMsg listenerMsg(callback);
    StateMsg msg(SET_LISTENER, &listenerMsg, sizeof(SetListenerMsg));
    defaultImpl_->Handle(msg);
    if (wakeupImpl_ != nullptr) {
        wakeupImpl_->Handle(msg);
    }
}
 
int32_t WakeupEngineImplFacade::InitDefaultImpl()
{
    INTELL_VOICE_LOG_INFO("enter");
    defaultImpl_ = EngineImplFactory<BaseWakeupEngineImpl, WakeupEngineImpl>::CreateInstance(stateListener_);
    if (defaultImpl_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to allocate engine impl");
        return -1;
    }
    StateMsg msg(INIT);
    if (defaultImpl_->Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("init wakeup engine impl failed");
        defaultImpl_ = nullptr;
        return -1;
    }
    wispherImpl_ = defaultImpl_;
    return 0;
}
 
int32_t WakeupEngineImplFacade::Handle(const StateMsg &msg)
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    switch (msg.msgId) {
        // control cmd - current impl and phone impl
        case SET_LISTENER:
        case RELEASE:
        case RELEASE_ADAPTER:
        case RESET_ADAPTER:
            defaultImpl_->Handle(msg);
            if (wakeupImpl_ != nullptr && wakeupImpl_ != defaultImpl_) {
                return wakeupImpl_->Handle(msg);
            }
            break;
        // wakeup cmd - current impl
        case START_RECOGNIZE:
        case STOP_RECOGNIZE:
        case START_CAPTURER:
        case READ:
        case STOP_CAPTURER: {
                auto impl = GetCurrentImpl();
                if (impl != nullptr) {
                    return impl->Handle(msg);
                }
                break;
            }
        // other function cmd - phone impl
        default:
            return defaultImpl_->Handle(msg);
    }
    INTELL_VOICE_LOG_WARN("engineImpl null");
    return 0;
}
 
std::shared_ptr<BaseWakeupEngineImpl> WakeupEngineImplFacade::GetCurrentImpl()
{
    // NOTE: this function acquires stateMutex_, Caller must hold deviceMutex_ to avoid deadlock
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (curProcess_ == ProcessType::PROCESS_WISPHER_PHONE) {
        return wispherImpl_;
    } else {
        return wakeupImpl_;
    }
}
 
int32_t WakeupEngineImplFacade::OnTriggerDetected(int32_t uuid, const std::string &macAddress,
    const EventDetail &event)
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    if (!ProcessCheck()) {
        if (macAddress == DEFAULT_MAC_ADDRESS) {
            EngineCallbackMessage::CallFunc(HANDLE_CLOSE_WAKEUP_SOURCE, true);
        }
        return WAKEUP_PROCESSING;
    }
    
    if (uuid == PROXIMAL_WAKEUP_MODEL_UUID) {
        return OnWispherDetected(uuid, event);
    } else {
        return OnWakeupDetected(uuid, macAddress, event);
    }
}

bool WakeupEngineImplFacade::ProcessCheck()
{
    if (curState_ > EngineState::INITIALIZED) {
        INTELL_VOICE_LOG_WARN("processing: %{public}d, state: %{public}d", curProcess_, curState_);
        return false;
    }
    return true;
}

int32_t WakeupEngineImplFacade::OnWispherDetected(int32_t uuid, const EventDetail &event)
{
    if (wispherImpl_ != nullptr) {
        wispherImpl_->OnTriggerDetected(uuid, wakeupDevice_, event);
    }
    return SUCCESS;
}

int32_t WakeupEngineImplFacade::OnWakeupDetected(int32_t uuid, const std::string &macAddress, const EventDetail &event)
{
    if (macAddress != wakeupDevice_->macAddress) {
        INTELL_VOICE_LOG_WARN("start failed, device conflict type %{public}d", wakeupDevice_->type);
        if (macAddress == DEFAULT_MAC_ADDRESS) {
            EngineCallbackMessage::CallFunc(HANDLE_CLOSE_WAKEUP_SOURCE, true);
        }
        return DEVICE_ERROR;
    }
    if (wakeupImpl_ != nullptr) {
        wakeupImpl_->OnTriggerDetected(uuid, wakeupDevice_, event);
    }
    return SUCCESS;
}
 
int32_t WakeupEngineImplFacade::UpdateWakeupEngineImpl(const std::shared_ptr<WakeupDeviceDetail> &info)
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    INTELL_VOICE_LOG_INFO("wakeup device changed, type: %{public}d", info->type);
    ReleaseEngineImpl();
    wakeupDevice_ = info;
    CreateEngineImpl();
    INTELL_VOICE_LOG_INFO("success");
    return SUCCESS;
}

int32_t WakeupEngineImplFacade::ReleaseEngineImpl()
{
    if (wakeupImpl_ == nullptr || wakeupImpl_ == defaultImpl_) {
        INTELL_VOICE_LOG_INFO("no need to release");
        return SUCCESS;
    }
    wakeupImpl_->Destroy();
    wakeupImpl_ = nullptr;
 
    return SUCCESS;
}
 
int32_t WakeupEngineImplFacade::CreateEngineImpl()
{
    if (wakeupDevice_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("wakeup device is null");
        return DEVICE_ERROR;
    }
    WakeupDeviceType deviceType = wakeupDevice_->type;
    switch (deviceType) {
        case WakeupDeviceType::PHONE:
            wakeupImpl_ = defaultImpl_;
            break;
        case WakeupDeviceType::HEADSET:
            wakeupImpl_
                = EngineImplFactory<BaseWakeupEngineImpl, HeadsetWakeupEngineImpl>::CreateInstance(stateListener_);
            InitImpl();
            break;
        case WakeupDeviceType::AI_GLASSES:
            wakeupImpl_
                = EngineImplFactory<BaseWakeupEngineImpl, GlassesWakeupEngineImpl>::CreateInstance(stateListener_);
            InitImpl();
            break;
        default:
            return DEVICE_ERROR;
    }
    if (callback_ != nullptr && wakeupImpl_ != nullptr) {
        SetListenerMsg listenerMsg(callback_);
        StateMsg msgCb(SET_LISTENER, &listenerMsg, sizeof(SetListenerMsg));
        wakeupImpl_->Handle(msgCb);
    }
    INTELL_VOICE_LOG_INFO("create impl success");
    return SUCCESS;
}
 
void WakeupEngineImplFacade::InitImpl()
{
    if (wakeupImpl_ == nullptr) {
        return;
    }
    StateMsg msg(INIT, &wakeupDevice_->modelType, sizeof(wakeupDevice_->modelType));
    if (wakeupImpl_->Handle(msg) != 0) {
        INTELL_VOICE_LOG_ERROR("init wakeup engine impl failed, use default phone");
        wakeupImpl_ = defaultImpl_;
    }
}

void WakeupEngineImplFacade::OnStateChange(const State &stateInfo, const ProcessType &type)
{
    int32_t nextState = stateInfo.state;
    INTELL_VOICE_LOG_INFO("process type: %{public}d, nextState: %{public}d", type, nextState);
    if (type == PROCESS_IDLE) {
        INTELL_VOICE_LOG_ERROR("process type error -idle");
        return;
    }
    std::unique_lock<std::mutex> lock(stateMutex_);
    // process enter
    if (curState_ <= EngineState::INITIALIZED && nextState > EngineState::INITIALIZED) {
        INTELL_VOICE_LOG_INFO("process enter");
        curState_ = static_cast<EngineState>(nextState);
        curProcess_ = type;
        return;
    }

    // in process
    if (curState_ > EngineState::INITIALIZED && curProcess_ == type) {
        if (nextState <= EngineState::INITIALIZED) {
            // process exit
            curState_ = EngineState::IDLE;
            curProcess_ = PROCESS_IDLE;
            INTELL_VOICE_LOG_INFO("process exit");
            return;
        } else {
            curState_ = static_cast<EngineState>(nextState);
        }
    }
}
}
}