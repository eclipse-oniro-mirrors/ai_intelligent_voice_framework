/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef I_DEVICE_WAKEUP_H
#define I_DEVICE_WAKEUP_H
 
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
 
namespace OHOS {
namespace IntellVoiceEngine {
 
enum WakeupDeviceType {
    PHONE = 0,
    HEADSET = 1,
    GLASSES = 2,
    AI_GLASSES = 3,
    DEVICE_MAX = 4,
};
 
struct WakeupDeviceInfo {
    WakeupDeviceType type;
    /* SA serviceName */
    std::string serviceName;
    std::string parameters;
};
 
struct WakeupDeviceDetail {
    WakeupDeviceType type;
    /* SA serviceName */
    std::string serviceName;
    std::string macAddress;
    bool needSecondStageWakeup = false;
    std::string modelType = "default";
    std::string parameters;
 
public:
    explicit WakeupDeviceDetail (const WakeupDeviceInfo &deviceInfo, const std::string &mac) : type(deviceInfo.type),
        serviceName(deviceInfo.serviceName), macAddress(mac), parameters(deviceInfo.parameters) {};
};

enum DeviceMessageType : int32_t {
    MSG_NONE = 0,
    MSG_INIT_DONE = 1,
    MSG_ENROLL_COMPLETE = 2,
    MSG_COMMIT_ENROLL_COMPLETE = 3,
    MSG_RECOGNIZE_COMPLETE = 4,
};
 
enum DeviceErrors : int32_t {
    OK = 0,
    ERROR_OFFSET = -100,
    INVALID_PARAMS = -101,
    INIT_FAILED = -102,
    ENROLL_FAILED = -103,
    COMMIT_ENROLL_FAILED = -104,
    WAKEUP_FAILED = -105,
};
 
struct DeviceCallBackEvent {
    DeviceMessageType msgId;
    DeviceErrors result;
    std::string info;
};
 
using DeviceCallBack = std::function<void(DeviceCallBackEvent)>;

class IDeviceWakeup {
public:
    virtual int32_t ReadStream(std::vector<uint8_t> &audioStream, bool &hasAwakeWord) = 0;
    virtual int32_t StopReadingStream() = 0;
    virtual int32_t GetAwakeState() = 0;
    virtual int32_t SetCallback(DeviceCallBack callback) = 0;
};
}
}
#endif