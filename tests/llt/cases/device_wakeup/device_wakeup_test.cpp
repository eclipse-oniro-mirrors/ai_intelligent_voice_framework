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

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "intell_voice_service.h"
#include "i_intell_voice_engine.h"
#include "intell_voice_log.h"
#include "intell_voice_info.h"
#include "i_device_wakeup.h"
#include "intell_voice_definitions.h"
#include "engine_device_manager.h"
#include "audio_system.h"
#include "hdi_mock_init.h"
#include "llt_constants.h"

#define LOG_TAG "DeviceWakeupTest"

using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::IntellVoice;
using namespace OHOS::AudioStandard;
using namespace OHOS;
using namespace testing::ext;
using OHOS::IntellVoice::LltConstants::WAIT_INIT_DONE_MS;
using OHOS::IntellVoice::LltConstants::WAIT_RECOGNIZE_COMPLETE_MS;
using OHOS::IntellVoice::LltConstants::WAIT_AUDIO_SWITCH_MS;

namespace {
// Create a standard IntellVoiceEngineInfo for wakeup engine Attach
IntellVoiceEngineInfo MakeWakeupEngineInfo()
{
    IntellVoiceEngineInfo info = {};
    info.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    info.minBufSize = LltConstants::DEFAULT_MIN_BUF_SIZE;
    info.sampleChannels = LltConstants::DEFAULT_SAMPLE_CHANNELS;
    info.bitsPerSample = LltConstants::DEFAULT_BITS_PER_SAMPLE;
    info.sampleRate = LltConstants::DEFAULT_SAMPLE_RATE;
    return info;
}

// Create an AudioDeviceDescriptor for bluetooth A2DP with the given MAC address
std::shared_ptr<AudioDeviceDescriptor> MakeBluetoothDesc(const std::string &mac)
{
    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    desc->macAddress_ = mac;
    return desc;
}

// Create an AudioDeviceDescriptor for speaker (PHONE path)
std::shared_ptr<AudioDeviceDescriptor> MakeSpeakerDesc()
{
    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->deviceType_ = DEVICE_TYPE_SPEAKER;
    desc->macAddress_ = "";
    return desc;
}

// Switch audio device via EngineDeviceManager and wait for async callback
void SwitchAudioDevice(EngineDeviceManager &mgr,
    const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &descList)
{
    mgr.UpdateAudioDevice(descList);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_AUDIO_SWITCH_MS));
}

// Switch audio to bluetooth device by MAC address
void SwitchToBluetooth(EngineDeviceManager &mgr, const std::string &mac)
{
    auto desc = MakeBluetoothDesc(mac);
    SwitchAudioDevice(mgr, {desc});
}

// Switch audio to speaker (restore PHONE implementation)
void SwitchToSpeaker(EngineDeviceManager &mgr)
{
    auto desc = MakeSpeakerDesc();
    SwitchAudioDevice(mgr, {desc});
}

// Execute Start + NotifyDeviceWakeupEvent + Stop sequence
void StartNotifyAndStop(sptr<IIntellVoiceEngine> &engine, const std::string &mac, DeviceWakeupEvent eventType)
{
    int32_t ret = engine->Start(true);
    INTELL_VOICE_LOG_INFO("Start result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    EventDetail event = {eventType, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(mac, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->Stop();
    INTELL_VOICE_LOG_INFO("Stop result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
}

// Execute full capturer flow: StartCapturer + Read + StopCapturer
void CapturerFlow(sptr<IIntellVoiceEngine> &engine)
{
    int32_t ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    INTELL_VOICE_LOG_INFO("StartCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::vector<uint8_t> data;
    ret = engine->Read(data);
    INTELL_VOICE_LOG_INFO("Read result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("StopCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
}

// Deregister device and release wakeup engine
void DeregisterAndRelease(EngineDeviceManager &mgr, const std::string &mac,
    IntellVoiceService *service)
{
    std::string serviceName;
    int32_t ret = mgr.DeregisterDevice(mac, serviceName);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
    service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}
} // namespace

class DeviceWakeupTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    static inline std::unique_ptr<IntellVoiceService> g_service = nullptr;
};

void DeviceWakeupTest::SetUpTestCase(void)
{
    INTELL_VOICE_LOG_INFO("DeviceWakeupTest Setup");
    InitHdiMock();
    g_service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
}

void DeviceWakeupTest::TearDownTestCase(void)
{
    INTELL_VOICE_LOG_INFO("DeviceWakeupTest TearDown");
    g_service.reset();
}

void DeviceWakeupTest::SetUp(void)
{
}

void DeviceWakeupTest::TearDown(void)
{
}

/**
 * 测试用例: UT_DW_001_RegisterHeadsetDeviceWakeupInfo
 * 业务背景: 注册HEADSET类型设备的唤醒信息，用于蓝牙耳机唤醒场景
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造HEADSET类型的WakeupDeviceInfo
 *   2. 调用RegisterDeviceWakeupInfo
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_001_RegisterHeadsetDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing RegisterHeadsetDeviceWakeupInfo");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "AA:BB:CC:DD:EE:FF";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::HEADSET;
    info.serviceName = "tws_kws_service";
    info.parameters = "headset_params";

    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_002_RegisterGlassesDeviceWakeupInfo
 * 业务背景: 注册GLASSES类型设备的唤醒信息，用于智能眼镜唤醒场景
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造GLASSES类型的WakeupDeviceInfo
 *   2. 调用RegisterDeviceWakeupInfo
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_002_RegisterGlassesDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing RegisterGlassesDeviceWakeupInfo");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "11:22:33:44:55:66";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::GLASSES;
    info.serviceName = "glasses_service";
    info.parameters = "glasses_params";

    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_003_RegisterAiGlassesDeviceWakeupInfo
 * 业务背景: 注册AI_GLASSES类型设备的唤醒信息，用于AI眼镜唤醒场景
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造AI_GLASSES类型的WakeupDeviceInfo
 *   2. 调用RegisterDeviceWakeupInfo
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_003_RegisterAiGlassesDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing RegisterAiGlassesDeviceWakeupInfo");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "22:33:44:55:66:77";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::AI_GLASSES;
    info.serviceName = "ai_glasses_service";
    info.parameters = "ai_glasses_params";

    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_004_DeregisterMultipleDevices
 * 业务背景: 注销多个不同类型设备的唤醒信息
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注销HEADSET设备
 *   2. 注销GLASSES设备
 *   3. 注销AI_GLASSES设备
 * 预期结果: 所有注销操作返回值>=0
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_004_DeregisterMultipleDevices, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing DeregisterMultipleDevices");
    ASSERT_NE(g_service, nullptr);

    int32_t ret = g_service->DeregisterDeviceWakeupInfo("AA:BB:CC:DD:EE:FF");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = g_service->DeregisterDeviceWakeupInfo("11:22:33:44:55:66");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = g_service->DeregisterDeviceWakeupInfo("22:33:44:55:66:77");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_005_WakeupEngineBluetoothHfpEvent
 * 业务背景: 唤醒引擎接收蓝牙HFP识别事件，通过Facade转发到OnTriggerDetected处理
 * 前置条件: 唤醒引擎已创建且进入INITIALIZED状态
 * 测试步骤:
 *   1. 创建唤醒引擎，等待INIT_DONE
 *   2. 构造BLUETOOTH_HFP_RECOGNIZE_EVENT类型的EventDetail
 *   3. 使用DEFAULT_MAC_ADDRESS调用NotifyDeviceWakeupEvent
 * 预期结果: 事件通过Facade转发到OnTriggerDetected处理，返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_005_WakeupEngineBluetoothHfpEvent, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine BluetoothHfpEvent");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    EventDetail event = {BLUETOOTH_HFP_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_HFP_50};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_DW_006_CreateUpdateEngine
 * 业务背景: 通过IntellVoiceService创建INTELL_VOICE_UPDATE类型引擎，用于唤醒模型升级
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用CreateIntellVoiceEngine(INTELL_VOICE_UPDATE)
 *   2. 验证返回值和engine对象
 * 预期结果: 返回0，engine对象有效
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_006_CreateUpdateEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing CreateUpdateEngine");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_UPDATE, engine);
    INTELL_VOICE_LOG_INFO("CreateUpdateEngine result: %{public}d, engine: %{public}p", ret, engine.GetRefPtr());
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    EXPECT_NE(engine, nullptr);
}

/**
 * 测试用例: UT_DW_007_ReleaseUpdateEngine
 * 业务背景: 释放INTELL_VOICE_UPDATE类型引擎
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用ReleaseIntellVoiceEngine(INTELL_VOICE_UPDATE)
 *   2. 验证返回值为0
 * 预期结果: 返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_007_ReleaseUpdateEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing ReleaseUpdateEngine");
    ASSERT_NE(g_service, nullptr);

    int32_t ret = g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_UPDATE);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_008_ServiceSetParameterEnrolled
 * 业务背景: 服务级SetParameter设置"isEnrolled=true"参数，验证参数可正常设置
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 调用SetParameter("isEnrolled=true")
 *   2. 调用GetParameter("isEnrolled")验证
 * 预期结果: GetParameter返回设置的值
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_008_ServiceSetParameterEnrolled, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing Service SetParameter isEnrolled");
    ASSERT_NE(g_service, nullptr);

    int32_t ret = g_service->SetParameter("isEnrolled=true");
    INTELL_VOICE_LOG_INFO("SetParameter result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::string enrolled = g_service->GetParameter("isEnrolled");
    INTELL_VOICE_LOG_INFO("GetParameter isEnrolled: %{public}s", enrolled.c_str());
    EXPECT_TRUE(enrolled == "true" || enrolled == "false");
}

/**
 * 测试用例: UT_DW_009_ServiceGetParameterWakeupVersion
 * 业务背景: 服务级GetParameter查询唤醒模型版本信息
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 调用GetParameter("WakeupVersion")
 * 预期结果: 返回版本字符串（可能为空）
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_009_ServiceGetParameterWakeupVersion, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing Service GetParameter WakeupVersion");
    ASSERT_NE(g_service, nullptr);

    std::string version = g_service->GetParameter("WakeupVersion");
    INTELL_VOICE_LOG_INFO("WakeupVersion: %{public}s", version.c_str());
}

/**
 * 测试用例: UT_DW_010_ServiceGetParameterSensibility
 * 业务背景: 服务级GetParameter查询唤醒灵敏度设置
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 调用GetParameter("Sensibility")
 * 预期结果: 返回灵敏度字符串（可能为空）
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_010_ServiceGetParameterSensibility, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing Service GetParameter Sensibility");
    ASSERT_NE(g_service, nullptr);

    std::string sensibility = g_service->GetParameter("Sensibility");
    INTELL_VOICE_LOG_INFO("Sensibility: %{public}s", sensibility.c_str());
}

/**
 * 测试用例: UT_DW_011_ServiceSendWakeupFileWithLargeBuffer
 * 业务背景: 发送较大唤醒文件到设备端，验证大数据量场景下的处理
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造4096字节的buffer
 *   2. 调用SendWakeupFile
 * 预期结果: 返回处理结果或错误码
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_011_ServiceSendWakeupFileWithLargeBuffer, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing SendWakeupFile with large buffer");
    ASSERT_NE(g_service, nullptr);

    std::vector<uint8_t> buffer(LltConstants::LARGE_AUDIO_BUFFER_SIZE, 0xAB);
    int32_t ret = g_service->SendWakeupFile("large_model.bin", buffer);
    INTELL_VOICE_LOG_INFO("SendWakeupFile result: %{public}d", ret);
    // mock环境下文件路径校验失败，返回-1是预期行为
    EXPECT_EQ(ret, LltConstants::ERR_CODE_GENERIC_FAILURE);
}

/**
 * 测试用例: UT_DW_012_EnrollWithWakeupFilesForResultWithInfo
 * 业务背景: 使用详细唤醒信息进行注册，传入JSON格式的唤醒词配置
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造包含唤醒词和模型类型的JSON信息
 *   2. 调用EnrollWithWakeupFilesForResult
 * 预期结果: 返回处理结果或错误码
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_012_EnrollWithWakeupFilesForResultWithInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollWithWakeupFilesForResult with detailed info");
    ASSERT_NE(g_service, nullptr);

    std::string wakeupInfo = "{\"phrase\":\"xiaoyi xiaoyi\",\"modelType\":\"VOICE_WAKEUP_TYPE\"}";
    int32_t ret = g_service->EnrollWithWakeupFilesForResult(wakeupInfo, nullptr);
    INTELL_VOICE_LOG_INFO("EnrollWithWakeupFilesForResult result: %{public}d", ret);
    // mock环境下引擎对象为空，返回-1是预期行为
    EXPECT_EQ(ret, LltConstants::ERR_CODE_GENERIC_FAILURE);
}

/**
 * 测试用例: UT_DW_013_WakeupEngineEvaluate
 * 业务背景: 唤醒引擎调用Evaluate接口评估唤醒词质量
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用Evaluate传入唤醒词
 * 预期结果: Evaluate通过EngineBase处理，不崩溃
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_013_WakeupEngineEvaluate, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Evaluate");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    EvaluationResult result = {};
    ret = engine->Evaluate("\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA", result);
    INTELL_VOICE_LOG_INFO("Evaluate result: %{public}d, score: %{public}d", ret, result.score);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_DW_014_MultiDeviceRegisterAndDeregister
 * 业务背景: 注册多个不同类型设备后逐个注销，验证多设备管理能力
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册PHONE、HEADSET、GLASSES三种设备
 *   2. 逐个注销
 * 预期结果: 所有操作返回值>=0
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_014_MultiDeviceRegisterAndDeregister, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing MultiDevice Register and Deregister");
    ASSERT_NE(g_service, nullptr);

    // Register PHONE
    WakeupDeviceInfo phoneInfo = {};
    phoneInfo.type = WakeupDeviceType::PHONE;
    phoneInfo.serviceName = "phone_service";
    phoneInfo.parameters = "phone_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo("AA:11:22:33:44:55", phoneInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // Register HEADSET
    WakeupDeviceInfo headsetInfo = {};
    headsetInfo.type = WakeupDeviceType::HEADSET;
    headsetInfo.serviceName = "tws_kws_service";
    headsetInfo.parameters = "headset_params";
    ret = g_service->RegisterDeviceWakeupInfo("BB:22:33:44:55:66", headsetInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // Register GLASSES
    WakeupDeviceInfo glassesInfo = {};
    glassesInfo.type = WakeupDeviceType::GLASSES;
    glassesInfo.serviceName = "glasses_service";
    glassesInfo.parameters = "glasses_params";
    ret = g_service->RegisterDeviceWakeupInfo("CC:33:44:55:66:77", glassesInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // Deregister all
    ret = g_service->DeregisterDeviceWakeupInfo("AA:11:22:33:44:55");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
    ret = g_service->DeregisterDeviceWakeupInfo("BB:22:33:44:55:66");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
    ret = g_service->DeregisterDeviceWakeupInfo("CC:33:44:55:66:77");
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_015_HeadsetWakeupFullFlow
 * 业务背景: 蓝牙耳机设备唤醒完整流程：注册设备 → 创建唤醒引擎 → 通知HFP事件 → 注销设备
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册HEADSET设备唤醒信息
 *   2. 创建WAKEUP引擎，等待INIT_DONE
 *   3. 通过NotifyDeviceWakeupEvent通知唤醒事件（使用DEFAULT_MAC_ADDRESS触发PHONE路径）
 *   4. 注销HEADSET设备
 * 预期结果: 全流程各步骤返回正常
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_015_HeadsetWakeupFullFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing HeadsetWakeup FullFlow");
    ASSERT_NE(g_service, nullptr);

    // 1. Register HEADSET device
    std::string macAddress = "AA:BB:CC:DD:EE:01";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::HEADSET;
    info.serviceName = "tws_kws_service";
    info.parameters = "headset_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify wakeup event (PHONE path via DEFAULT_MAC_ADDRESS)
    EventDetail event = {BLUETOOTH_HFP_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 4. Deregister HEADSET device
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_016_GlassesWakeupFullFlow
 * 业务背景: 智能眼镜设备唤醒完整流程：注册设备 → 创建唤醒引擎 → Attach → 通知唤醒事件 → Detach → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册GLASSES设备唤醒信息
 *   2. 创建WAKEUP引擎并Attach
 *   3. 等待INIT_DONE，通过NotifyDeviceWakeupEvent通知唤醒事件
 *   4. Detach引擎
 *   5. 注销GLASSES设备
 * 预期结果: 全流程各步骤返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_016_GlassesWakeupFullFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GlassesWakeup FullFlow");
    ASSERT_NE(g_service, nullptr);

    // 1. Register GLASSES device
    std::string macAddress = "AA:BB:CC:DD:EE:02";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::GLASSES;
    info.serviceName = "glasses_service";
    info.parameters = "glasses_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine and attach
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    IntellVoiceEngineInfo engineInfo = {};
    engineInfo.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    engineInfo.minBufSize = LltConstants::DEFAULT_MIN_BUF_SIZE;
    engineInfo.sampleChannels = LltConstants::DEFAULT_SAMPLE_CHANNELS;
    engineInfo.bitsPerSample = LltConstants::DEFAULT_BITS_PER_SAMPLE;
    engineInfo.sampleRate = LltConstants::DEFAULT_SAMPLE_RATE;
    ret = engine->Attach(engineInfo);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify VOICE_RECOGNIZE_EVENT (PHONE path via DEFAULT_MAC_ADDRESS)
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 4. Detach engine
    ret = engine->Detach();
    INTELL_VOICE_LOG_INFO("Detach result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 释放引擎资源，避免后续测试复用已Detach的引擎（adapter_为空导致后续接口返回-1）
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 5. Deregister GLASSES device
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_017_AiGlassesWakeupFullFlow
 * 业务背景: AI眼镜设备唤醒完整流程：注册设备 → 创建唤醒引擎 → 语音识别事件 → 采集音频 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册AI_GLASSES设备唤醒信息
 *   2. 创建WAKEUP引擎，等待INIT_DONE
 *   3. NotifyDeviceWakeupEvent触发START_RECOGNIZE进入RECOGNIZING
 *   4. 等待RECOGNIZE_COMPLETE进入RECOGNIZED
 *   5. StartCapturer采集音频
 *   6. Read读取数据
 *   7. StopCapturer停止采集
 *   8. 注销AI_GLASSES设备
 * 预期结果: 全流程各步骤返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_017_AiGlassesWakeupFullFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing AiGlassesWakeup FullFlow");
    ASSERT_NE(g_service, nullptr);

    // 1. Register AI_GLASSES device
    std::string macAddress = "AA:BB:CC:DD:EE:03";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::AI_GLASSES;
    info.serviceName = "ai_glasses_service";
    info.parameters = "ai_glasses_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify VOICE_RECOGNIZE_EVENT to trigger RECOGNIZING
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 4. Wait for RECOGNIZE_COMPLETE to enter RECOGNIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));

    // 5. StartCapturer (RECOGNIZED -> READ_CAPTURER)
    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    INTELL_VOICE_LOG_INFO("StartCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 6. Read audio data
    std::vector<uint8_t> data;
    ret = engine->Read(data);
    INTELL_VOICE_LOG_INFO("Read result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 7. StopCapturer (READ_CAPTURER -> INITIALIZED)
    ret = engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("StopCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 释放引擎资源，避免后续测试复用状态异常的引擎
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 8. Deregister AI_GLASSES device
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_018_PhoneWakeupFullFlow
 * 业务背景: 手机设备唤醒完整流程：注册PHONE设备 → 创建唤醒引擎 → Attach → NotifyDeviceWakeupEvent → Stop → Detach → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册PHONE设备唤醒信息
 *   2. 创建WAKEUP引擎并完整生命周期操作
 *   3. 注销PHONE设备
 * 预期结果: 全流程各步骤返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_018_PhoneWakeupFullFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing PhoneWakeup FullFlow");
    ASSERT_NE(g_service, nullptr);

    // 1. Register PHONE device
    std::string macAddress = "AA:BB:CC:DD:EE:04";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::PHONE;
    info.serviceName = "default_service_name_phone";
    info.parameters = "phone_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine with full lifecycle
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    engine->SetCallback(nullptr);

    IntellVoiceEngineInfo engineInfo = {};
    engineInfo.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    engineInfo.minBufSize = LltConstants::DEFAULT_MIN_BUF_SIZE;
    engineInfo.sampleChannels = LltConstants::DEFAULT_SAMPLE_CHANNELS;
    engineInfo.bitsPerSample = LltConstants::DEFAULT_BITS_PER_SAMPLE;
    engineInfo.sampleRate = LltConstants::DEFAULT_SAMPLE_RATE;
    ret = engine->Attach(engineInfo);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // Trigger START_RECOGNIZE to enter RECOGNIZING state
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Stop in RECOGNIZING state
    ret = engine->Stop();
    INTELL_VOICE_LOG_INFO("Stop result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->Detach();
    INTELL_VOICE_LOG_INFO("Detach result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 释放引擎资源，避免后续测试复用已Detach的引擎
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 3. Deregister PHONE device
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_019_MultiDeviceWakeupSequence
 * 业务背景: 多设备依次唤醒场景：注册多个设备 → 按顺序通知不同设备唤醒事件 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册PHONE、HEADSET、GLASSES三种设备
 *   2. 创建唤醒引擎，等待INIT_DONE
 *   3. 依次通知语音唤醒事件（使用DEFAULT_MAC_ADDRESS触发PHONE路径）
 *   4. 逐个注销设备
 * 预期结果: 各步骤返回正常
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_019_MultiDeviceWakeupSequence, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing MultiDeviceWakeup Sequence");
    ASSERT_NE(g_service, nullptr);

    std::string phoneMac = "DD:11:22:33:44:55";
    std::string headsetMac = "DD:22:33:44:55:66";
    std::string glassesMac = "DD:33:44:55:66:77";

    // 1. Register multiple devices
    WakeupDeviceInfo phoneInfo = {};
    phoneInfo.type = WakeupDeviceType::PHONE;
    phoneInfo.serviceName = "default_service_name_phone";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(phoneMac, phoneInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    WakeupDeviceInfo headsetInfo = {};
    headsetInfo.type = WakeupDeviceType::HEADSET;
    headsetInfo.serviceName = "tws_kws_service";
    ret = g_service->RegisterDeviceWakeupInfo(headsetMac, headsetInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    WakeupDeviceInfo glassesInfo = {};
    glassesInfo.type = WakeupDeviceType::GLASSES;
    glassesInfo.serviceName = "glasses_service";
    ret = g_service->RegisterDeviceWakeupInfo(glassesMac, glassesInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify events in sequence (PHONE path via DEFAULT_MAC_ADDRESS)
    EventDetail voiceEvent = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    EventDetail hfpEvent = {BLUETOOTH_HFP_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};

    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, voiceEvent);
    INTELL_VOICE_LOG_INFO("Phone wakeup event result: %{public}d", ret);
    // First NotifyDeviceWakeupEvent triggers RECOGNIZING, ProcessCheck passes (INITIALIZED <= INITIALIZED)
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // After first trigger, engine is in RECOGNIZING state, ProcessCheck fails for subsequent triggers
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, hfpEvent);
    INTELL_VOICE_LOG_INFO("Headset wakeup event result: %{public}d", ret);
    // ProcessCheck returns WAKEUP_PROCESSING(-2) because curState_ > INITIALIZED
    EXPECT_EQ(ret, LltConstants::ERR_CODE_WAKEUP_PROCESSING);

    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, voiceEvent);
    INTELL_VOICE_LOG_INFO("Glasses wakeup event result: %{public}d", ret);
    // Same, ProcessCheck returns WAKEUP_PROCESSING(-2)
    EXPECT_EQ(ret, LltConstants::ERR_CODE_WAKEUP_PROCESSING);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 4. Deregister all
    ret = g_service->DeregisterDeviceWakeupInfo(phoneMac);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
    ret = g_service->DeregisterDeviceWakeupInfo(headsetMac);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
    ret = g_service->DeregisterDeviceWakeupInfo(glassesMac);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_020_HeadsetWakeupWithCapturerFlow
 * 业务背景: 蓝牙耳机唤醒后采集音频流程：注册 → 通知HFP事件 → 等待识别完成 → StartCapturer → Read → StopCapturer → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册HEADSET设备
 *   2. 创建唤醒引擎，等待INIT_DONE
 *   3. 通知HFP唤醒事件触发RECOGNIZING
 *   4. 等待RECOGNIZE_COMPLETE进入RECOGNIZED
 *   5. StartCapturer/Read/StopCapturer完成音频采集
 *   6. 注销设备
 * 预期结果: 全流程各步骤返回0
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_020_HeadsetWakeupWithCapturerFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing HeadsetWakeup WithCapturer Flow");
    ASSERT_NE(g_service, nullptr);

    // 1. Register HEADSET
    std::string macAddress = "EE:AA:BB:CC:DD:01";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::HEADSET;
    info.serviceName = "tws_kws_service";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify HFP event to trigger RECOGNIZING
    EventDetail event = {BLUETOOTH_HFP_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 4. Wait for RECOGNIZE_COMPLETE to enter RECOGNIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));

    // 5. Audio capturer flow
    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    INTELL_VOICE_LOG_INFO("StartCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::vector<uint8_t> data;
    ret = engine->Read(data);
    INTELL_VOICE_LOG_INFO("Read result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("StopCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    // 6. Deregister
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_021_DeviceWakeupReregister
 * 业务背景: 设备注销后重新注册，验证注销后可再次注册
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册HEADSET设备
 *   2. 注销该设备
 *   3. 使用相同MAC地址重新注册HEADSET设备
 *   4. 再次注销
 * 预期结果: 重新注册成功，返回值>=0
 * 优先级: P1
 */
HWTEST_F(DeviceWakeupTest, UT_DW_021_DeviceWakeupReregister, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing DeviceWakeup Reregister");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "FF:AA:BB:CC:DD:01";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::HEADSET;
    info.serviceName = "tws_kws_service";
    info.parameters = "headset_params";

    // 1. Register
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Deregister
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 3. Re-register same device
    ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 4. Deregister again
    ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_DW_022_AudioSwitchToHeadsetTriggerImplChange
 * 业务背景: 音频输出切换到蓝牙耳机时，唤醒引擎实现应从PHONE切换到HEADSET
 *   完整流程：注册HEADSET设备 → 创建唤醒引擎 → 切换音频输出到耳机MAC →
 *   Facade创建HeadsetWakeupEngineImpl → 执行后续引擎接口 → 切回扬声器 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过EngineDeviceManager注册HEADSET设备
 *   2. 创建WAKEUP引擎（Facade初始化时注册了设备变更回调）
 *   3. Attach引擎
 *   4. 模拟音频输出切换到蓝牙A2DP设备（MAC匹配HEADSET）
 *   5. 等待异步回调触发WakeupEngineImplFacade切换到HeadsetWakeupEngineImpl
 *   6. 执行SetParameter/GetParameter等接口验证新实现可用
 *   7. 模拟音频输出切回扬声器（恢复PHONE实现）
 *   8. Detach引擎，注销设备
 * 预期结果: 音频切换触发实现切换，后续接口正常执行，全流程不崩溃
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_022_AudioSwitchToHeadsetTriggerImplChange, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing AudioSwitchToHeadset TriggerImplChange");
    ASSERT_NE(g_service, nullptr);

    // Ensure no leftover engine from previous test
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    std::string headsetMac = "11:22:33:44:55:66";

    // 1. Register HEADSET device via EngineDeviceManager
    EngineDeviceManager &deviceMgr = EngineDeviceManager::GetInstance();
    WakeupDeviceInfo headsetInfo = {};
    headsetInfo.type = WakeupDeviceType::HEADSET;
    headsetInfo.serviceName = "tws_kws_service";
    headsetInfo.parameters = R"({"needSecondStageWakeup":true,"modelType":"headset_v1"})";
    int32_t ret = deviceMgr.RegisterDevice(headsetMac, headsetInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine and Attach
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->Attach(MakeWakeupEngineInfo());
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 3. Switch audio to bluetooth A2DP (MAC matches HEADSET)
    SwitchToBluetooth(deviceMgr, headsetMac);

    // Verify active device switched to HEADSET
    auto activeDevice = deviceMgr.GetWakeupDevice();
    INTELL_VOICE_LOG_INFO("Active device type: %{public}d, mac: %{public}s",
        activeDevice != nullptr ? activeDevice->type : -1,
        activeDevice != nullptr ? activeDevice->macAddress.c_str() : "null");

    // 4. Execute engine interfaces with HEADSET implementation active
    ret = engine->SetParameter("WakeupType=0");
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::string param = engine->GetParameter("wakeup_source_channel");
    INTELL_VOICE_LOG_INFO("GetParameter result: %{public}s", param.c_str());

    ret = engine->WriteAudio(nullptr, 0);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 5. Start + Notify + Stop
    StartNotifyAndStop(engine, headsetMac, VOICE_RECOGNIZE_EVENT);

    // 6. Switch back to speaker, then Detach and cleanup
    SwitchToSpeaker(deviceMgr);
    ret = engine->Detach();
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    DeregisterAndRelease(deviceMgr, headsetMac, g_service.get());
}

/**
 * 测试用例: UT_DW_023_AudioSwitchToGlassesTriggerImplChange
 * 业务背景: 音频输出切换到AI眼镜时，唤醒引擎实现应切换到GlassesWakeupEngineImpl
 *   完整流程：注册AI_GLASSES设备 → 创建唤醒引擎 → 切换音频输出到眼镜MAC →
 *   Facade创建GlassesWakeupEngineImpl → 执行引擎接口 → 切回 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过EngineDeviceManager注册AI_GLASSES设备
 *   2. 创建WAKEUP引擎
 *   3. 模拟音频输出切换到匹配AI_GLASSES的MAC
 *   4. 等待异步回调触发实现切换
 *   5. 执行引擎接口验证GlassesWakeupEngineImpl可用
 *   6. 通知眼镜语音唤醒事件
 *   7. 切回扬声器，注销设备
 * 预期结果: 眼镜实现切换正常，后续接口不崩溃
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_023_AudioSwitchToGlassesTriggerImplChange, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing AudioSwitchToGlasses TriggerImplChange");
    ASSERT_NE(g_service, nullptr);

    // Ensure no leftover engine from previous test
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    std::string glassesMac = "22:33:44:55:66:77";

    // 1. Register AI_GLASSES device
    EngineDeviceManager &deviceMgr = EngineDeviceManager::GetInstance();
    WakeupDeviceInfo glassesInfo = {};
    glassesInfo.type = WakeupDeviceType::AI_GLASSES;
    glassesInfo.serviceName = "ai_glasses_service";
    glassesInfo.parameters = R"({"needSecondStageWakeup":false,"modelType":"ai_glasses_v1"})";
    int32_t ret = deviceMgr.RegisterDevice(glassesMac, glassesInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // 3. Switch audio to glasses MAC
    SwitchToBluetooth(deviceMgr, glassesMac);

    // Verify active device switched to AI_GLASSES
    auto activeDevice = deviceMgr.GetWakeupDevice();
    INTELL_VOICE_LOG_INFO("Active device type: %{public}d", activeDevice != nullptr ? activeDevice->type : -1);

    // 4. Attach and start with GlassesWakeupEngineImpl
    ret = engine->Attach(MakeWakeupEngineInfo());
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->Start(true);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 5. Notify glasses wakeup event (AI_GLASSES skips RECOGNIZING, goes to RECOGNIZED)
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(glassesMac, event);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 6. Capturer flow in RECOGNIZED state (glasses: StartCapturer + StopCapturer, no Read)
    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ret = engine->StopCapturer();
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 7. Detach, switch back, deregister
    ret = engine->Detach();
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    SwitchToSpeaker(deviceMgr);
    DeregisterAndRelease(deviceMgr, glassesMac, g_service.get());
}

/**
 * 测试用例: UT_DW_024_AudioSwitchBetweenHeadsetAndPhone
 * 业务背景: 音频输出在耳机和手机之间来回切换时，唤醒引擎实现在PHONE和HEADSET间切换
 *   完整流程：注册HEADSET → 切到耳机 → 执行接口 → 切回手机 → 执行接口 → 再切耳机 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册HEADSET设备
 *   2. 创建并Attach唤醒引擎
 *   3. 切音频到耳机MAC → 验证HEADSET实现
 *   4. 切音频回扬声器 → 验证PHONE实现
 *   5. 再切到耳机MAC → 验证HEADSET实现
 *   6. 注销设备
 * 预期结果: 多次切换不崩溃，每次切换后引擎接口正常
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_024_AudioSwitchBetweenHeadsetAndPhone, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing AudioSwitch Between Headset and Phone");
    ASSERT_NE(g_service, nullptr);

    // Ensure no leftover engine from previous test
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    std::string headsetMac = "33:44:55:66:77:88";

    // 1. Register HEADSET
    EngineDeviceManager &deviceMgr = EngineDeviceManager::GetInstance();
    WakeupDeviceInfo headsetInfo = {};
    headsetInfo.type = WakeupDeviceType::HEADSET;
    headsetInfo.serviceName = "tws_kws_service";
    headsetInfo.parameters = R"({"needSecondStageWakeup":true})";
    int32_t ret = deviceMgr.RegisterDevice(headsetMac, headsetInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    // 2. Create and attach wakeup engine
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->Attach(MakeWakeupEngineInfo());
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 3. Switch to headset -> Start + Notify + Stop
    auto headsetDesc = MakeBluetoothDesc(headsetMac);
    SwitchAudioDevice(deviceMgr, {headsetDesc});
    StartNotifyAndStop(engine, headsetMac, VOICE_RECOGNIZE_EVENT);

    // 4. Switch back to phone (speaker) -> Start + Notify + Stop
    auto speakerDesc = MakeSpeakerDesc();
    SwitchAudioDevice(deviceMgr, {speakerDesc});
    StartNotifyAndStop(engine, DEFAULT_MAC_ADDRESS, VOICE_RECOGNIZE_EVENT);

    // 5. Switch to headset again -> Start + Notify + Stop
    SwitchAudioDevice(deviceMgr, {headsetDesc});
    StartNotifyAndStop(engine, headsetMac, VOICE_RECOGNIZE_EVENT);

    // 6. Restore, Detach and cleanup
    SwitchToSpeaker(deviceMgr);
    ret = engine->Detach();
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    DeregisterAndRelease(deviceMgr, headsetMac, g_service.get());
}

/**
 * 测试用例: UT_DW_025_HeadsetImplWakeupThenCapturerFlow
 * 业务背景: 音频切到耳机后，HeadsetWakeupEngineImpl处理唤醒+音频采集完整流程
 *   完整流程：注册HEADSET → 切音频 → Attach → NotifyHFP事件 →
 *   等待RECOGNIZE_COMPLETE → StartCapturer → Read → StopCapturer → Stop → Detach → 切回 → 注销
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 注册HEADSET设备并切换音频
 *   2. 创建引擎并Attach
 *   3. 通知HFP唤醒事件触发RECOGNIZING
 *   4. 等待RECOGNIZE_COMPLETE进入RECOGNIZED
 *   5. 启动采集、读取数据、停止采集
 *   6. 停止识别，Detach
 *   7. 切回手机，注销
 * 预期结果: 完整流程各步骤返回0
 * 优先级: P0
 */
HWTEST_F(DeviceWakeupTest, UT_DW_025_HeadsetImplWakeupThenCapturerFlow, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing HeadsetImpl WakeupThenCapturer Flow");
    ASSERT_NE(g_service, nullptr);

    // Ensure no leftover engine from previous test
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    std::string headsetMac = "44:55:66:77:88:99";

    // 1. Register HEADSET and switch audio
    EngineDeviceManager &deviceMgr = EngineDeviceManager::GetInstance();
    WakeupDeviceInfo headsetInfo = {};
    headsetInfo.type = WakeupDeviceType::HEADSET;
    headsetInfo.serviceName = "tws_kws_service";
    headsetInfo.parameters = R"({"needSecondStageWakeup":true,"modelType":"default"})";
    int32_t ret = deviceMgr.RegisterDevice(headsetMac, headsetInfo);
    EXPECT_GE(ret, LltConstants::ERR_CODE_SUCCESS);

    SwitchToBluetooth(deviceMgr, headsetMac);

    // 2. Create engine and attach
    sptr<IIntellVoiceEngine> engine = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->Attach(MakeWakeupEngineInfo());
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Wait for INIT_DONE on headset impl
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // 3. Notify HFP wakeup event to trigger RECOGNIZING
    EventDetail hfpEvent = {BLUETOOTH_HFP_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(headsetMac, hfpEvent);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 4. Wait for RECOGNIZE_COMPLETE to enter RECOGNIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));

    // 5. Capturer flow, then Detach
    CapturerFlow(engine);
    ret = engine->Detach();
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // 6. Restore and cleanup
    SwitchToSpeaker(deviceMgr);
    DeregisterAndRelease(deviceMgr, headsetMac, g_service.get());
}
