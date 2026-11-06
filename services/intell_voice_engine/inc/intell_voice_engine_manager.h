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
#ifndef ENGINE_MANAGER_H
#define ENGINE_MANAGER_H
#include <mutex>
#include <map>
#include <atomic>
#include "engine_base.h"
#include "intell_voice_engine_arbitration.h"
#include "intell_voice_death_recipient.h"
#include "update_engine_controller.h"
#include "task_executor.h"
#include "intell_voice_definitions.h"
#include "i_intell_voice_service.h"
#include "i_device_wakeup.h"

namespace OHOS {
namespace IntellVoiceEngine {
class IntellVoiceEngineManager : private IntellVoiceEngineArbitration,
    private UpdateEngineController {
public:
    IntellVoiceEngineManager();
    ~IntellVoiceEngineManager();
    static bool GetScreenOff();
    void SetScreenOff(bool value);
    static void SetEnrollResult(IntellVoiceEngineType type, bool result)
    {
        if ((type < INTELL_VOICE_ENROLL) || (type >= ENGINE_TYPE_BUT)) {
            return;
        }

        g_enrollResult[type].store(result);
    }
    static bool GetEnrollResult(IntellVoiceEngineType type)
    {
        if ((type < INTELL_VOICE_ENROLL) || (type >= ENGINE_TYPE_BUT)) {
            return false;
        }

        return g_enrollResult[type].load();
    }

    bool AnyEngineExist(const std::vector<IntellVoiceEngineType> &types);
    void OnAudioPolicyServiceChange(bool isAdded);
    bool RegisterProxyDeathRecipient(IntellVoiceEngineType type, const sptr<IRemoteObject> &object);
    bool DeregisterProxyDeathRecipient(IntellVoiceEngineType type);
    bool RegisterDeviceDeathRecipient(const std::string &serviceName, const sptr<IRemoteObject> &object);
    bool DeregisterDeviceDeathRecipient(const std::string &serviceName);

    int32_t GetUploadFiles(int numMax, std::vector<UploadFilesFromHdi> &files);
    int32_t SetParameter(const std::string &key, const std::string &value);
    std::string GetParameter(const std::string &key);
    int32_t GetWakeupSourceFilesList(std::vector<std::string> &cloneFiles);
    int32_t GetWakeupSourceFile(const std::string &filePath, std::vector<uint8_t> &buffer);
    int32_t SendWakeupFile(const std::string &filePath, const std::vector<uint8_t> &buffer);
    std::string GetDspSensibility(const std::string &sensibility, const std::string &dspFeature,
    const std::string &configPath);

    void ClearUserDataInner();
    void DeviceServiceDie(const std::string &serviceName);
    bool IsNeedUpdateComplete(int32_t result, const std::string &param);
    bool IsNeedUpdateRetry();
    void EngineOnDetected(int32_t uuid);
    void ClearWakeupEngineCb();
    bool CreateOrResetWakeupEngine();
    bool IsEngineExist(IntellVoiceEngineType type);
    sptr<IIntellVoiceEngine> CreateEngine(IntellVoiceEngineType type,
        const std::string &param = "", bool reEnroll = false);
    int32_t ReleaseEngineInner(IntellVoiceEngineType type);
    int32_t SilenceUpdate();
    int32_t WhisperVprUpdate(bool reEnroll = false);
    int32_t CloneUpdate(const std::string &wakeupInfo, const sptr<IRemoteObject> &object);
    int32_t ImplicitEnroll(const std::string &wakeupInfo, const sptr<IRemoteObject> &object);
    int32_t ServiceStopProc();
    int32_t RegisterWakeupDevice(const std::string &macAddress, const WakeupDeviceInfo &info);
    int32_t DeregisterWakeupDevice(const std::string &macAddress);
    void SetDspSensibility(const std::string &sensibility);
    void OnServiceStart();
    void OnServiceStop();
    void SubscribeSwingEvent(const std::string &event);
    void UnSubscribeSwingEvent(const std::string &event);

private:
    sptr<IIntellVoiceEngine> CreateEngineInner(IntellVoiceEngineType type,
        const std::string &param = "", bool reEnroll = false);
    void ReleaseUpdateEngine() override;
    bool CreateUpdateEngine(const std::string &param, bool reEnroll = false) override;
    void LoadIntellVoiceHost();
    void UnloadIntellVoiceHost();
    int32_t GetUploadFilesFromVendor(int numMax, std::vector<UploadFilesFromHdi> &files);
    int32_t GetUploadFilesFromSystem(int numMax, std::vector<UploadFilesFromHdi> &files);
    int32_t ClearImplicitEnrollData(const std::string &key, const std::string &value);

private:
    static std::mutex instanceMutex_;
    static std::shared_ptr<IntellVoiceEngineManager> instance_;
    static std::atomic<bool> g_enrollResult[ENGINE_TYPE_BUT];
    static std::atomic<bool> screenoff_;
    std::mutex deathMutex_;
    std::map<IntellVoiceEngineType, sptr<EngineBase>> engines_;
    std::map<IntellVoiceEngineType, sptr<IntellVoiceUtils::IntellVoiceDeathRecipient>> proxyDeathRecipient_;
    std::map<IntellVoiceEngineType, sptr<IRemoteObject>> deathRecipientObj_;
    std::mutex serviceDeathMutex_;
    std::mutex engineOpMutex_ ;
    std::map<std::string, sptr<IntellVoiceUtils::IntellVoiceDeathRecipient>> serviceDeathRecipient_;
    std::map<std::string, sptr<IRemoteObject>> serviceDeathRecipientObj_;
};
}  // namespace IntellVoice
}  // namespace OHOS
#endif
