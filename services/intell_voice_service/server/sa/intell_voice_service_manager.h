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
#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H
#include <mutex>
#include <map>
#include <atomic>
#include "i_device_wakeup.h"
#include "switch_observer.h"
#include "switch_provider.h"
#include "task_executor.h"
#include "intell_voice_definitions.h"
#include "intell_voice_engine_registrar.h"
#include "intell_voice_trigger_registrar.h"
#include "trigger_base_type.h"
#include "service_manager_type.h"
namespace OHOS {
namespace IntellVoiceEngine {

template<typename T, typename E>
class IntellVoiceServiceManager : private OHOS::IntellVoiceUtils::TaskExecutor, private T, private E,
    private IntellVoiceTriggerRegistrar, private IntellVoiceEngineRegistrar {
public:
    ~IntellVoiceServiceManager() override;
    static IntellVoiceServiceManager &GetInstance()
    {
        static IntellVoiceServiceManager instance;
        return instance;
    }

    void HandleUnloadIntellVoiceService(bool isAsync);
    bool HandleOnIdle();
    void HandlePowerSaveModeChange();
    sptr<IIntellVoiceEngine> HandleCreateEngine(IntellVoiceEngineType type);
    int32_t HandleReleaseEngine(IntellVoiceEngineType type) override;
    void HandleSilenceUpdate();
    void HandleWhisperVprUpdate();
    int32_t HandleCloneUpdate(const std::string &wakeupInfo, const sptr<IRemoteObject> &object);
    void HandleServiceStop();

    void HandleSwitchOn(bool isAsync, int32_t uuid, bool needUpdateAdapter);
    void HandleSwitchOff(bool isAsync, int32_t uuid);

    bool RegisterProxyDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object);
    bool DeregisterProxyDeathRecipient(IntellVoiceEngineType type);
    bool RegisterDeviceDeathRecipient(const std::string &serviceName, const sptr<IRemoteObject> &object);
    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files);
    int32_t EngineSetParameter(const std::string &keyValueList);
    std::string EngineGetParameter(const std::string &key);
    int32_t GetWakeupSourceFilesList(std::vector<std::string>& cloneFiles);
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer);
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer);
    void SetScreenOff(bool value);

    void ProcBreathModel();
    void ProcSingleLevelModel();
    void CreateSwitchProvider();
    void ReleaseSwitchProvider();
    bool StartDetection(int32_t uuid);
    void StopDetection(int32_t uuid);
    bool QuerySwitchStatus(const std::string &key) override;
    int32_t ClearUserData();
    int32_t RegisterWakeupDevice(const std::string &macAddress, const WakeupDeviceInfo &info);
    int32_t DeregisterWakeupDevice(const std::string &macAddress);
    void OnServiceStart(std::map<int32_t, std::function<void(bool)>> &saChangeFuncMap);
    void OnServiceStop();
    void OnServiceActive();

    using TaskExecutor::AddAsyncTask;
    using TaskExecutor::AddSyncTask;
    using TaskExecutor::StopThread;

private:
    void HandleCloseWakeupSource(bool isNeedStop = false) override;
    void HandleClearWakeupEngineCb() override;
    void HandleDeviceServiceDie(const std::string &serviceName) override;
    void HandleUpdateComplete(int32_t result, const std::string &param) override;
    void HandleUpdateRetry() override;
    int32_t ReleaseEngine(IntellVoiceEngineType type) override;
    int32_t ReleaseImplicitEnrollEngine() override;
    std::string TriggerGetParameter(const std::string &key) override;
    int32_t TriggerSetParameter(const std::string &key, const std::string &value) override;
    void TriggerMgrUpdateModel(std::vector<uint8_t> buffer, int32_t uuid,
        IntellVoiceTrigger::TriggerModelType type) override;

private:
    IntellVoiceServiceManager();
    static bool IsSwitchKeyValid(const std::string &key)
    {
        if ((key == WAKEUP_KEY) || (key == WHISPER_KEY) ||
            (key == SHORTWORD_KEY) || (key == DUALMIC_KEY)) {
            return true;
        }
        return false;
    }

    bool QuerySwitchByUuid(int32_t uuid)
    {
        if (uuid == VOICE_WAKEUP_MODEL_UUID) {
            return QuerySwitchStatus(WAKEUP_KEY);
        }

        if (uuid == PROXIMAL_WAKEUP_MODEL_UUID) {
            return QuerySwitchStatus(WHISPER_KEY);
        }

        return false;
    }
    void OnSwitchChange(const std::string &switchKey);
    void RegisterObserver(const std::string &switchKey);
    bool IsNeedToUnloadService();
    int32_t UnloadIntellVoiceService();
    void NoiftySwitchOnToPowerChange();
    void OnDetected(int32_t uuid);
    void OnSingleLevelDetected();
    int32_t SwitchOnProc(int32_t uuid, bool needUpdateAdapter);
    int32_t SwitchOffProc(int32_t uuid);
    void CreateAndStartServiceObject(int32_t uuid, bool needResetAdapter);
    void ReleaseServiceObject(int32_t uuid);
    bool AddStartDetectionTask(int32_t uuid);
    void DelStartDetectionTask(int32_t uuid);
    bool IsSwitchError(const std::string &key);
    void StopWakeupSource();
    void ResetSingleLevelWakeup(const std::string &value);
    bool HasReceviedRecordStartMsg();
    void NoiftyRecordStartInfoChange();
    void HandleRecordStartInfoChange();
    void OnTriggerConnectServiceStart() override;
    void SetShortWordStatus();
    void SetDualMicStatus();
    void ScheduleDelayedUnload();
    void DelayedUnloadService();
    void HandleSensibility(const std::string &key, const std::string &value);
    void HandleWakeupScene(const std::string &key, const std::string &value);
    void HandleWakeupBundleName(const std::string &key, const std::string &value);
    void HandleWakeupAbilityName(const std::string &key, const std::string &value);
    void HandleCountryInfo(const std::string &key, const std::string &value);
    void HandleRecordStart(const std::string &key, const std::string &value);
    void HandleTriggerParam(const std::string &key, const std::string &value);
    void HandleConcurrentWhiteList(const std::string &key, const std::string &value);
    int32_t HandleDataClear(const std::string &key, const std::string &value);
    int32_t HandleEngineRelease(const std::string &key, const std::string &value);
    void HandleConcurrentBlackList(const std::string &key, const std::string &value);
    void InitParameterHandlerMap();

private:
    std::map<std::string, std::function<void(const std::string&, const std::string&)>> voidHandlerMap_;
    std::map<std::string, std::function<int32_t(const std::string&, const std::string&)>> handlerMap_;
    bool notifyPowerModeChange_ = false;
    std::mutex powerModeChangeMutex_;
    std::condition_variable powerModeChangeCv_;
    std::mutex switchMutex_;
    std::map<const std::string, sptr<SwitchObserver>> switchObserver_;
    IntellVoiceUtils::UniqueProductType<SwitchProvider> switchProvider_ =
        IntellVoiceUtils::UniqueProductType<SwitchProvider> {nullptr, nullptr};
    std::map<int32_t, bool> isStarted_;
    int32_t recordStart_ = -1;
    bool notifyRecordStartInfoChange_ = false;
    std::mutex recordStartInfoChangeMutex_;
    std::condition_variable recordStartInfoChangeCv_;
#ifdef USE_FFRT
    std::shared_ptr<ffrt::queue> taskQueue_ = nullptr;
    std::map<int32_t, ffrt::task_handle> taskHandle_;
#endif
    std::mutex delayUnloadMutex_;
    bool delayUnloadWaiting_ = false;
    std::atomic<bool> serviceUnloading_ = false;
    std::chrono::steady_clock::time_point lastCallTime_;
};
}  // namespace IntellVoice
}  // namespace OHOS
#endif
