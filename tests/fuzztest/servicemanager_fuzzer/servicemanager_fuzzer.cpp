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
#include <cstddef>
#include <cstdint>

#include "intell_voice_service_manager.h"
#include "intell_voice_manager.h"
#include "intell_voice_log.h"
#include "engine_base.h"
#include "i_intell_voice_engine.h"
#include "i_intell_voice_engine_callback.h"
#include "enroll_intell_voice_engine.h"
#include "wakeup_intell_voice_engine.h"
#include "base_macros.h"
#include "engine_host_manager.h"
#include "intell_voice_util.h"
#include "wakeup_engine_impl.h"
#include "headset_wakeup_engine_impl.h"

#define LOG_TAG "ServiceManagerFuzzer"

const int32_t LIMITSIZE = 4;

using namespace std;
using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceTests {
void IntellVoiceServiceManagerFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    IntellVoiceManager::GetInstance();
    INTELL_VOICE_LOG_ERROR("enter");
    const auto &manager = IntellVoiceServiceManager::GetInstance();
    IntellVoiceEngineType type = *reinterpret_cast<const IntellVoiceEngineType *>(data);
    manager->HandleCreateEngine(type);
    manager->HandleReleaseEngine(type);
    manager->SetEnrollResult(type, data);
    manager->GetEnrollResult(type);
    const sptr<IRemoteObject> object;
    manager->HandleSilenceUpdate();
    manager->HandleCloneUpdate(std::to_string(size), object);
    manager->HandleSwitchOn(data, size, size);
    manager->HandleSwitchOff(data, size);
    manager->HandleCloseWakeupSource();
    manager->HandleUnloadIntellVoiceService(data);
    manager->HandleOnIdle();
    manager->HandleServiceStop();
    manager->HandleHeadsetHostDie();
    manager->ProcBreathModel();
    manager->CreateSwitchProvider();
    manager->ReleaseSwitchProvider();
    manager->StartDetection(size);
    manager->StopDetection(size);
    manager->QuerySwitchStatus(std::to_string(size));
    manager->DeregisterProxyDeathRecipient(type);
    manager->GetParameter(std::to_string(size));
    std::vector<std::string> cloneFiles;
    manager->GetWakeupSourceFilesList(cloneFiles);
    std::vector<uint8_t> buffer;
    manager->GetWakeupSourceFile(std::to_string(size), buffer);
    manager->SendWakeupFile(std::to_string(size), buffer);
}

void EnrollEngineFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    INTELL_VOICE_LOG_ERROR("enter");
    EngineHostManager::GetInstance().Init();
    auto enrollEngine = IntellVoiceServiceManager::GetInstance()->HandleCreateEngine(INTELL_VOICE_ENROLL);
    if (enrollEngine == nullptr) {
        INTELL_VOICE_LOG_ERROR("enrollEngine is nullptr");
        return;
    }
    IntellVoiceEngineInfo info = {
        .wakeupPhrase = std::to_string(size),
        .isPcmFromExternal = data,
        .minBufSize = size,
        .sampleChannels = size,
        .bitsPerSample = size,
        .sampleRate = size,
    };
    enrollEngine->Attach(info);
    enrollEngine->Start(data);
    enrollEngine->SetParameter(std::to_string(size));
    sptr<IRemoteObject> object;
    enrollEngine->SetCallback(object);
    enrollEngine->GetParameter(std::to_string(size));
    enrollEngine->WriteAudio(data, size);
    EvaluationResultInfo infos;
    enrollEngine->Evaluate(std::to_string(size), infos);
    enrollEngine->Stop();
    enrollEngine->Detach();
    IntellVoiceServiceManager::GetInstance()->HandleReleaseEngine(INTELL_VOICE_ENROLL);
}

void WakeupEngineFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    INTELL_VOICE_LOG_ERROR("enter");
    EngineHostManager::GetInstance().Init();
    auto wakeEngine = IntellVoiceServiceManager::GetInstance()->HandleCreateEngine(INTELL_VOICE_WAKEUP);
    if (wakeEngine == nullptr) {
        INTELL_VOICE_LOG_ERROR("wakeEngine is nullptr");
        return;
    }
    wakeEngine->SetParameter("CommitEnrollment=true");
    sptr<IRemoteObject> object;
    wakeEngine->SetCallback(object);
    IntellVoiceEngineInfo info = {};
    wakeEngine->Attach(info);
    wakeEngine->Start(data);
    wakeEngine->SetParameter(std::to_string(size));
    wakeEngine->GetParameter(std::to_string(size));
    std::vector<uint8_t> pcmData;
    wakeEngine->GetWakeupPcm(pcmData);
    wakeEngine->StartCapturer(size);
    wakeEngine->Read(pcmData);
    wakeEngine->StopCapturer();
    wakeEngine->NotifyHeadsetWakeEvent();
    wakeEngine->NotifyHeadsetHostEvent(*reinterpret_cast<const HeadsetHostEventType *>(data));
    wakeEngine->Stop();
    wakeEngine->Detach();

    auto wakeupSourceProcess = std::make_shared<WakeupSourceProcess>();
    wakeupSourceProcess->Init(size);
    std::vector<std::vector<uint8_t>> audioData;
    wakeupSourceProcess->Write(audioData);
    std::vector<uint8_t> capturerData;
    wakeupSourceProcess->Read(capturerData, size);
    wakeupSourceProcess->Release();
    IntellVoiceServiceManager::GetInstance()->HandleReleaseEngine(INTELL_VOICE_WAKEUP);
}

void UpdataEngineFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    EngineHostManager::GetInstance().Init();
    INTELL_VOICE_LOG_ERROR("enter");
    auto updateEngine = IntellVoiceServiceManager::GetInstance()->HandleCreateEngine(INTELL_VOICE_UPDATE);
    if (updateEngine == nullptr) {
        INTELL_VOICE_LOG_ERROR("updateEngine is nullptr");
        return;
    }
}

void ServiceUtilsFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    INTELL_VOICE_LOG_ERROR("enter");
    IntellVoiceUtil::VerifySystemPermission(std::to_string(size));
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint32_t size1 = static_cast<uint32_t>(size);
    IntellVoiceUtil::ReadFile(std::to_string(size), buffer, size1);
    IntellVoiceUtil::GetHdiVersionId(size, size);
    std::vector<std::vector<uint8_t>> audioData;
    HistoryInfoMgr &historyInfoMgr = HistoryInfoMgr::GetInstance();
    historyInfoMgr.SetEnrollEngineUid(size);
    historyInfoMgr.GetEnrollEngineUid();
    historyInfoMgr.SetWakeupEngineBundleName(std::to_string(size));
    historyInfoMgr.GetWakeupEngineBundleName();
    historyInfoMgr.SetWakeupEngineAbilityName(std::to_string(size));
    historyInfoMgr.GetWakeupEngineAbilityName();
    historyInfoMgr.SetWakeupVesion(std::to_string(size));
    historyInfoMgr.GetWakeupVesion();
    historyInfoMgr.SetLanguage(std::to_string(size));
    historyInfoMgr.GetLanguage();
    historyInfoMgr.SetArea(std::to_string(size));
    historyInfoMgr.GetArea();
    historyInfoMgr.SetWakeupPhrase(std::to_string(size));
    historyInfoMgr.GetWakeupPhrase();
    historyInfoMgr.SetWakeupDspFeature(std::to_string(size));
    historyInfoMgr.GetWakeupDspFeature();
}

void HdiAdapterFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }
    INTELL_VOICE_LOG_ERROR("enter");
    EngineHostManager::GetInstance().Init();
    EngineHostManager::GetInstance().RegisterEngineHDIDeathRecipient();
    EngineHostManager::GetInstance().DeregisterEngineHDIDeathRecipient();
    EngineHostManager::GetInstance().SetDataOprCallback();
    IntellVoiceEngineAdapterDescriptor desc;
    EngineHostManager::GetInstance().CreateEngineAdapter(desc);
    EngineHostManager::GetInstance().ReleaseEngineAdapter(desc);
    std::vector<UploadHdiFile> files;
    EngineHostManager::GetInstance().GetUploadFiles(size, files);
    std::vector<std::string> cloneFiles;
    EngineHostManager::GetInstance().GetWakeupSourceFilesList(cloneFiles);
    std::vector<uint8_t> buffer;
    EngineHostManager::GetInstance().GetWakeupSourceFile(std::to_string(size), buffer);
    EngineHostManager::GetInstance().SendWakeupFile(std::to_string(size), buffer);
    EngineHostManager::GetInstance().GetEngineHostProxy1_0();
    EngineHostManager::GetInstance().GetEngineHostProxy1_1();
    EngineHostManager::GetInstance().GetEngineHostProxy1_2();
}
} // namespace.OHOS
} // namespace.IntellVoiceTests

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::IntellVoiceTests::IntellVoiceServiceManagerFuzzTest(data, size);
    OHOS::IntellVoiceTests::EnrollEngineFuzzTest(data, size);
    OHOS::IntellVoiceTests::WakeupEngineFuzzTest(data, size);
    OHOS::IntellVoiceTests::UpdataEngineFuzzTest(data, size);
    OHOS::IntellVoiceTests::ServiceUtilsFuzzTest(data, size);
    OHOS::IntellVoiceTests::HdiAdapterFuzzTest(data, size);
    INTELL_VOICE_LOG_ERROR("LLVMFuzzerTestOneInput end");
    return 0;
}
