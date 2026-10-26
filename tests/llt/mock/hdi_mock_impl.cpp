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

/*
 * Non-inline mock implementations for HDI interfaces.
 * Thin LTO (-flto=thin) prevents inline definitions in headers from taking effect,
 * so all static Get()/CastFrom() methods and concrete mock classes must be in .cpp.
 *
 * Mock adapters fire INIT_DONE after Attach() and RECOGNIZE_COMPLETE after Start()
 * to drive the WakeupEngineImpl state machine through proper transitions:
 *   IDLE -> INITIALIZING -> INITIALIZED -> RECOGNIZING -> RECOGNIZED
 */

#include "idevmgr_hdi.h"
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <message_parcel.h>
#include "v1_0/iintell_voice_engine_manager.h"
#include "v1_0/iintell_voice_engine_adapter.h"
#include "v1_0/iintell_voice_engine_callback.h"
#include "v1_0/intell_voice_engine_types.h"
#include "v1_1/iintell_voice_engine_manager.h"
#include "v1_1/iintell_voice_data_opr_callback.h"
#include "v1_2/iintell_voice_engine_manager.h"
#include "v1_2/iintell_voice_engine_adapter.h"
#include "v1_2/intell_voice_engine_types.h"
#include "v1_3/iintell_voice_engine_manager.h"
#include "v1_3/iintell_voice_engine_adapter.h"
#include "v1_3/intell_voice_engine_types.h"
#include "engine_host_manager.h"

using namespace OHOS;
using namespace OHOS::HDI;

/* ==================== Device Manager ==================== */

namespace OHOS {
namespace HDI {
namespace DeviceManager {
namespace V1_0 {

class MockDeviceManager : public IDeviceManager {
public:
    int32_t LoadDevice(const std::string &serviceName) override
    {
        (void)serviceName;
        return HDF_SUCCESS;
    }
    int32_t UnloadDevice(const std::string &serviceName) override
    {
        (void)serviceName;
        return HDF_SUCCESS;
    }
};

sptr<IDeviceManager> IDeviceManager::Get()
{
    static sptr<IDeviceManager> instance = new MockDeviceManager();
    return instance;
}

} // V1_0
} // DeviceManager
} // HDI
} // OHOS

/* ==================== Engine Adapter V1_0 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_0 {

class MockEngineAdapter : public IIntellVoiceEngineAdapter {
public:
    MockEngineAdapter() : alive_(std::make_shared<std::atomic<bool>>(true)) {}
    int32_t SetCallback(const sptr<IIntellVoiceEngineCallback> &engineCallback) override
    {
        callback_ = engineCallback;
        return HDF_SUCCESS;
    }
    int32_t Attach(const IntellVoiceEngineAdapterInfo &info) override
    {
        (void)info;
        // Fire INIT_DONE asynchronously to transition WakeupEngineImpl: INITIALIZING -> INITIALIZED
        if (callback_ != nullptr) {
            auto cb = callback_;
            auto alive = alive_;
            std::thread([cb, alive]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (!alive->load()) {
                    return;
                }
                IntellVoiceEngineCallBackEvent event;
                event.msgId = INTELL_VOICE_ENGINE_MSG_INIT_DONE;
                event.result = INTELL_VOICE_ENGINE_OK;
                cb->OnIntellVoiceHdiEvent(event);
            }).detach();
        }
        return HDF_SUCCESS;
    }
    int32_t Detach() override
    {
        alive_->store(false);
        callback_ = nullptr;
        return HDF_SUCCESS;
    }
    int32_t SetParameter(const std::string &keyValueList) override
    {
        auto pos = keyValueList.find('=');
        if (pos != std::string::npos) {
            params_[keyValueList.substr(0, pos)] = keyValueList.substr(pos + 1);
        }
        return HDF_SUCCESS;
    }
    int32_t GetParameter(const std::string &keyList, std::string &valueList) override
    {
        auto it = params_.find(keyList);
        valueList = (it != params_.end()) ? it->second : "";
        return HDF_SUCCESS;
    }
    int32_t Start(const StartInfo &info) override
    {
        (void)info;
        // Fire HEADSET_RECOGNIZE_COMPLETE asynchronously for HeadsetWakeupEngineImpl
        // (V1_0 adapter is only used by HeadsetWakeupEngineImpl which expects msgId=6)
        if (callback_ != nullptr) {
            auto cb = callback_;
            auto alive = alive_;
            std::thread([cb, alive]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                if (!alive->load()) {
                    return;
                }
                IntellVoiceEngineCallBackEvent event;
                // V1_2::INTELL_VOICE_ENGINE_MSG_HEADSET_RECOGNIZE_COMPLETE
                event.msgId = static_cast<IntellVoiceEngineMessageType>(6);
                event.result = INTELL_VOICE_ENGINE_OK;
                cb->OnIntellVoiceHdiEvent(event);
            }).detach();
        }
        return HDF_SUCCESS;
    }
    int32_t Stop() override
    {
        return HDF_SUCCESS;
    }
    int32_t WriteAudio(const std::vector<uint8_t> &buffer) override
    {
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t Read(ContentType type, sptr<Ashmem> &buffer) override
    {
        (void)type;
        (void)buffer;
        return HDF_SUCCESS;
    }
private:
    sptr<IIntellVoiceEngineCallback> callback_;
    std::map<std::string, std::string> params_;
    std::shared_ptr<std::atomic<bool>> alive_;
};

} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Adapter V1_2 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_2 {

class MockEngineAdapterV2 : public IIntellVoiceEngineAdapter {
public:
    MockEngineAdapterV2() : alive_(std::make_shared<std::atomic<bool>>(true)) {}
    int32_t SetCallback(const sptr<V1_0::IIntellVoiceEngineCallback> &engineCallback) override
    {
        callback_ = engineCallback;
        return HDF_SUCCESS;
    }
    int32_t Attach(const V1_0::IntellVoiceEngineAdapterInfo &info) override
    {
        (void)info;
        // Fire INIT_DONE asynchronously for engine impls that use V1_2 adapter
        if (callback_ != nullptr) {
            auto cb = callback_;
            auto alive = alive_;
            std::thread([cb, alive]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                if (!alive->load()) {
                    return;
                }
                V1_0::IntellVoiceEngineCallBackEvent event;
                event.msgId = V1_0::INTELL_VOICE_ENGINE_MSG_INIT_DONE;
                event.result = V1_0::INTELL_VOICE_ENGINE_OK;
                cb->OnIntellVoiceHdiEvent(event);
            }).detach();
        }
        return HDF_SUCCESS;
    }
    int32_t Detach() override
    {
        alive_->store(false);
        callback_ = nullptr;
        return HDF_SUCCESS;
    }
    int32_t SetParameter(const std::string &keyValueList) override
    {
        auto pos = keyValueList.find('=');
        if (pos != std::string::npos) {
            params_[keyValueList.substr(0, pos)] = keyValueList.substr(pos + 1);
        }
        return HDF_SUCCESS;
    }
    int32_t GetParameter(const std::string &keyList, std::string &valueList) override
    {
        auto it = params_.find(keyList);
        valueList = (it != params_.end()) ? it->second : "";
        return HDF_SUCCESS;
    }
    int32_t Start(const V1_0::StartInfo &info) override
    {
        (void)info;
        // Fire RECOGNIZE_COMPLETE asynchronously for headset/glasses engine impls
        if (callback_ != nullptr) {
            auto cb = callback_;
            auto alive = alive_;
            std::thread([cb, alive]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                if (!alive->load()) {
                    return;
                }
                V1_0::IntellVoiceEngineCallBackEvent event;
                event.msgId = V1_0::INTELL_VOICE_ENGINE_MSG_RECOGNIZE_COMPLETE;
                event.result = V1_0::INTELL_VOICE_ENGINE_OK;
                cb->OnIntellVoiceHdiEvent(event);
            }).detach();
        }
        return HDF_SUCCESS;
    }
    int32_t Stop() override
    {
        return HDF_SUCCESS;
    }
    int32_t WriteAudio(const std::vector<uint8_t> &buffer) override
    {
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t Read(V1_0::ContentType type, sptr<Ashmem> &buffer) override
    {
        (void)type;
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t GetWakeupPcm(std::vector<uint8_t> &data) override
    {
        (void)data;
        return HDF_SUCCESS;
    }
    int32_t Evaluate(const std::string &word, EvaluationResultInfo &info) override
    {
        (void)word;
        (void)info;
        return HDF_SUCCESS;
    }
private:
    sptr<V1_0::IIntellVoiceEngineCallback> callback_;
    std::map<std::string, std::string> params_;
    std::shared_ptr<std::atomic<bool>> alive_;
};

sptr<IIntellVoiceEngineAdapter> IIntellVoiceEngineAdapter::CastFrom(
    const sptr<V1_0::IIntellVoiceEngineAdapter> &parent)
{
    (void)parent;
    return nullptr;
}

} // V1_2
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Adapter V1_3 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_3 {

sptr<IIntellVoiceEngineAdapter> IIntellVoiceEngineAdapter::CastFrom(
    const sptr<V1_2::IIntellVoiceEngineAdapter> &parent)
{
    (void)parent;
    return nullptr;
}

} // V1_3
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Manager V1_0 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_0 {

class MockEngineManager : public IIntellVoiceEngineManager {
public:
    int32_t GetVersion(uint32_t &majorVer, uint32_t &minorVer) override
    {
        majorVer = 1;
        minorVer = 3;
        return HDF_SUCCESS;
    }
    int32_t GetAdapterDescriptors(std::vector<IntellVoiceEngineAdapterDescriptor> &descs) override
    {
        (void)descs;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter(const IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new MockEngineAdapter();
        return HDF_SUCCESS;
    }
    int32_t ReleaseAdapter(const IntellVoiceEngineAdapterDescriptor &descriptor) override
    {
        (void)descriptor;
        return HDF_SUCCESS;
    }
};

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(bool isStub)
{
    (void)isStub;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManager();
    return instance;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(const std::string &serviceName, bool isStub)
{
    (void)serviceName;
    (void)isStub;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManager();
    return instance;
}

} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Manager V1_1 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_1 {

class MockEngineManagerV1_1 : public IIntellVoiceEngineManager {
public:
    int32_t GetAdapterDescriptors(std::vector<V1_0::IntellVoiceEngineAdapterDescriptor> &descs) override
    {
        (void)descs;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<V1_0::IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new V1_0::MockEngineAdapter();
        return HDF_SUCCESS;
    }
    int32_t ReleaseAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor) override
    {
        (void)descriptor;
        return HDF_SUCCESS;
    }
    int32_t SetDataOprCallback(const sptr<IIntellVoiceDataOprCallback> &dataOprCallback) override
    {
        (void)dataOprCallback;
        return HDF_SUCCESS;
    }
};

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(bool isStub)
{
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(const std::string &serviceName, bool isStub)
{
    (void)serviceName;
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::CastFrom(
    const sptr<V1_0::IIntellVoiceEngineManager> &parent)
{
    (void)parent;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManagerV1_1();
    return instance;
}

} // V1_1
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Manager V1_2 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_2 {

class MockEngineManagerV1_2 : public IIntellVoiceEngineManager {
public:
    int32_t GetAdapterDescriptors(std::vector<V1_0::IntellVoiceEngineAdapterDescriptor> &descs) override
    {
        (void)descs;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<V1_0::IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new V1_0::MockEngineAdapter();
        return HDF_SUCCESS;
    }
    int32_t ReleaseAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor) override
    {
        (void)descriptor;
        return HDF_SUCCESS;
    }
    int32_t SetDataOprCallback(const sptr<V1_1::IIntellVoiceDataOprCallback> &dataOprCallback) override
    {
        (void)dataOprCallback;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter_V_2(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new MockEngineAdapterV2();
        return HDF_SUCCESS;
    }
    int32_t GetUploadFiles(int32_t numMax, std::vector<UploadHdiFile> &files) override
    {
        (void)numMax;
        UploadHdiFile file;
        file.type = UploadHdiFileType::FILE_TYPE_WAKEUP;
        file.filesDescription = "mock_wakeup_file";
        files.push_back(file);
        return HDF_SUCCESS;
    }
    int32_t GetCloneFilesList(std::vector<std::string> &cloneFiles) override
    {
        (void)cloneFiles;
        return HDF_SUCCESS;
    }
    int32_t GetCloneFile(const std::string &filePath, std::vector<uint8_t> &buffer) override
    {
        (void)filePath;
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t SendCloneFile(const std::string &filePath, const std::vector<uint8_t> &buffer) override
    {
        (void)filePath;
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t ClearUserWakeupData(const std::string &wakeupPhrase) override
    {
        (void)wakeupPhrase;
        return HDF_SUCCESS;
    }
};

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(bool isStub)
{
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(const std::string &serviceName, bool isStub)
{
    (void)serviceName;
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::CastFrom(
    const sptr<V1_1::IIntellVoiceEngineManager> &parent)
{
    (void)parent;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManagerV1_2();
    return instance;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::CastFrom(
    const sptr<V1_0::IIntellVoiceEngineManager> &parent)
{
    (void)parent;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManagerV1_2();
    return instance;
}

} // V1_2
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Engine Manager V1_3 ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_3 {

class MockEngineManagerV1_3 : public IIntellVoiceEngineManager {
public:
    int32_t GetAdapterDescriptors(std::vector<V1_0::IntellVoiceEngineAdapterDescriptor> &descs) override
    {
        (void)descs;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<V1_0::IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new V1_0::MockEngineAdapter();
        return HDF_SUCCESS;
    }
    int32_t ReleaseAdapter(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor) override
    {
        (void)descriptor;
        return HDF_SUCCESS;
    }
    int32_t SetDataOprCallback(const sptr<V1_1::IIntellVoiceDataOprCallback> &dataOprCallback) override
    {
        (void)dataOprCallback;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter_V_2(const V1_0::IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<V1_2::IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new V1_2::MockEngineAdapterV2();
        return HDF_SUCCESS;
    }
    int32_t GetUploadFiles(int32_t numMax, std::vector<V1_2::UploadHdiFile> &files) override
    {
        (void)numMax;
        V1_2::UploadHdiFile file;
        file.type = V1_2::UploadHdiFileType::FILE_TYPE_WAKEUP;
        file.filesDescription = "mock_wakeup_file";
        files.push_back(file);
        return HDF_SUCCESS;
    }
    int32_t GetCloneFilesList(std::vector<std::string> &cloneFiles) override
    {
        (void)cloneFiles;
        return HDF_SUCCESS;
    }
    int32_t GetCloneFile(const std::string &filePath, std::vector<uint8_t> &buffer) override
    {
        (void)filePath;
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t SendCloneFile(const std::string &filePath, const std::vector<uint8_t> &buffer) override
    {
        (void)filePath;
        (void)buffer;
        return HDF_SUCCESS;
    }
    int32_t ClearUserWakeupData(const std::string &wakeupPhrase) override
    {
        (void)wakeupPhrase;
        return HDF_SUCCESS;
    }
    int32_t CreateAdapter_V_3(const IntellVoiceEngineAdapterDescriptor &descriptor,
        sptr<V1_2::IIntellVoiceEngineAdapter> &adapter) override
    {
        (void)descriptor;
        adapter = new V1_2::MockEngineAdapterV2();
        return HDF_SUCCESS;
    }
};

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(bool isStub)
{
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::Get(const std::string &serviceName, bool isStub)
{
    (void)serviceName;
    (void)isStub;
    return nullptr;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::CastFrom(
    const sptr<V1_2::IIntellVoiceEngineManager> &parent)
{
    (void)parent;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManagerV1_3();
    return instance;
}

sptr<IIntellVoiceEngineManager> IIntellVoiceEngineManager::CastFrom(
    const sptr<V1_0::IIntellVoiceEngineManager> &parent)
{
    (void)parent;
    static sptr<IIntellVoiceEngineManager> instance = new MockEngineManagerV1_3();
    return instance;
}

} // V1_3
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== Marshalling stubs for v1_0 types ==================== */

namespace OHOS {
namespace HDI {
namespace IntelligentVoice {
namespace Engine {
namespace V1_0 {

bool IntellVoiceEngineAdapterDescriptorBlockMarshalling(OHOS::MessageParcel &data,
    const IntellVoiceEngineAdapterDescriptor &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool IntellVoiceEngineAdapterDescriptorBlockUnmarshalling(OHOS::MessageParcel &data,
    IntellVoiceEngineAdapterDescriptor &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool IntellVoiceEngineAdapterInfoBlockMarshalling(OHOS::MessageParcel &data,
    const IntellVoiceEngineAdapterInfo &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool IntellVoiceEngineAdapterInfoBlockUnmarshalling(OHOS::MessageParcel &data,
    IntellVoiceEngineAdapterInfo &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool StartInfoBlockMarshalling(OHOS::MessageParcel &data, const StartInfo &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool StartInfoBlockUnmarshalling(OHOS::MessageParcel &data, StartInfo &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool IntellVoiceEngineCallBackEventBlockMarshalling(OHOS::MessageParcel &data,
    const IntellVoiceEngineCallBackEvent &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

bool IntellVoiceEngineCallBackEventBlockUnmarshalling(OHOS::MessageParcel &data,
    IntellVoiceEngineCallBackEvent &dataBlock)
{
    (void)data;
    (void)dataBlock;
    return true;
}

} // V1_0
} // Engine
} // IntelligentVoice
} // HDI
} // OHOS

/* ==================== HDI Mock Initialization ==================== */

void InitHdiMock()
{
    OHOS::IntellVoiceEngine::EngineHostManager::GetInstance().Init();
    OHOS::IntellVoiceEngine::EngineHostManager::GetInstance().SetDataOprCallback();
}
