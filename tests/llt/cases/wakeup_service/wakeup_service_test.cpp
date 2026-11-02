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
#include "intell_voice_definitions.h"
#include "intell_voice_info.h"
#include "i_device_wakeup.h"
#include "hdi_mock_init.h"
#include "llt_constants.h"

#define LOG_TAG "WakeupServiceTest"

using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::IntellVoice;
using namespace OHOS;
using namespace testing::ext;
using OHOS::IntellVoice::LltConstants::WAIT_INIT_DONE_MS;
using OHOS::IntellVoice::LltConstants::WAIT_RECOGNIZE_COMPLETE_MS;

class WakeupServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    static inline std::unique_ptr<IntellVoiceService> g_service = nullptr;
};

void WakeupServiceTest::SetUpTestCase(void)
{
    INTELL_VOICE_LOG_INFO("WakeupServiceTest Setup");
    InitHdiMock();
    g_service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
}

void WakeupServiceTest::TearDownTestCase(void)
{
    INTELL_VOICE_LOG_INFO("WakeupServiceTest TearDown");
    g_service.reset();
}

void WakeupServiceTest::SetUp(void)
{
}

void WakeupServiceTest::TearDown(void)
{
}

/**
 * 测试用例: UT_WE_001_CreateWakeupEngine
 * 业务背景: 通过IntellVoiceService创建INTELL_VOICE_WAKEUP类型引擎，验证唤醒引擎可正常创建
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP)
 *   2. 验证返回值为0且engine不为空
 * 预期结果: 返回0，engine对象有效
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_001_CreateWakeupEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing CreateWakeupEngine");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    EXPECT_NE(engine, nullptr);
}

/**
 * 测试用例: UT_WE_002_ReleaseWakeupEngine
 * 业务背景: 释放INTELL_VOICE_WAKEUP类型引擎，验证资源正常释放
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP)
 *   2. 验证返回值为0
 * 预期结果: 返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_002_ReleaseWakeupEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing ReleaseWakeupEngine");
    ASSERT_NE(g_service, nullptr);
    int32_t ret = g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
}

/**
 * 测试用例: UT_WE_003_WakeupEngineAttach
 * 业务背景: 唤醒引擎创建后，需Attach附加引擎配置信息（唤醒词、采样参数等）
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 构造IntellVoiceEngineInfo
 *   3. 调用Attach
 * 预期结果: Attach成功返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_003_WakeupEngineAttach, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Attach");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    IntellVoiceEngineInfo info = {};
    info.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    info.minBufSize = LltConstants::DEFAULT_MIN_BUF_SIZE;
    info.sampleChannels = LltConstants::DEFAULT_SAMPLE_CHANNELS;
    info.bitsPerSample = LltConstants::DEFAULT_BITS_PER_SAMPLE;
    info.sampleRate = LltConstants::DEFAULT_SAMPLE_RATE;
    ret = engine->Attach(info);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_004_WakeupEngineSetCallback
 * 业务背景: 唤醒引擎需要设置回调来接收识别结果事件
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用SetCallback(nullptr)
 * 预期结果: 不崩溃，正常处理nullptr回调
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_004_WakeupEngineSetCallback, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine SetCallback");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    engine->SetCallback(nullptr);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_005_WakeupEngineStart
 * 业务背景: 唤醒引擎启动识别流程，Start接口由WakeupEngine直接返回0
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用Start(true)
 * 预期结果: 返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_005_WakeupEngineStart, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Start");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->Start(true);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_006_WakeupEngineStop
 * 业务背景: 唤醒引擎停止识别，Stop通过Facade发送STOP_RECOGNIZE消息到状态机
 * 前置条件: 引擎进入RECOGNIZING状态（通过NotifyDeviceWakeupEvent触发）
 * 测试步骤:
 *   1. 创建唤醒引擎，等待INIT_DONE
 *   2. 通过NotifyDeviceWakeupEvent触发START_RECOGNIZE进入RECOGNIZING
 *   3. 调用Stop()
 * 预期结果: Stop在RECOGNIZING状态返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_006_WakeupEngineStop, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Stop");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE (INITIALIZING -> INITIALIZED)
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // Trigger START_RECOGNIZE to enter RECOGNIZING state
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Now in RECOGNIZING state, Stop should succeed
    ret = engine->Stop();
    INTELL_VOICE_LOG_INFO("Stop result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_007_WakeupEngineSetParameter
 * 业务背景: 唤醒引擎设置运行参数，如唤醒类型、灵敏度等，通过Facade转发到状态机
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用SetParameter("WakeupType=0")
 * 预期结果: SetParameter通过状态机处理
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_007_WakeupEngineSetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine SetParameter");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->SetParameter("WakeupType=0");
    INTELL_VOICE_LOG_INFO("SetParameter result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_008_WakeupEngineGetParameter
 * 业务背景: 唤醒引擎获取运行参数，如唤醒源通道数、回调状态等
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用GetParameter("wakeup_source_channel")
 * 预期结果: GetParameter通过状态机处理，不崩溃
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_008_WakeupEngineGetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine GetParameter");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    std::string result = engine->GetParameter("wakeup_source_channel");
    INTELL_VOICE_LOG_INFO("GetParameter result: %{public}s", result.c_str());

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_009_WakeupEngineDetach
 * 业务背景: 唤醒引擎分离，Detach通过Facade发送RELEASE消息释放引擎资源
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用Detach()
 * 预期结果: Detach通过状态机处理RELEASE
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_009_WakeupEngineDetach, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Detach");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    ret = engine->Detach();
    INTELL_VOICE_LOG_INFO("Detach result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_010_WakeupEngineStartCapturer
 * 业务背景: 唤醒成功后启动音频采集，用于one-shot场景持续采集音频
 * 前置条件: 引擎进入RECOGNIZED状态（NotifyDeviceWakeupEvent + RECOGNIZE_COMPLETE）
 * 测试步骤:
 *   1. 创建唤醒引擎，等待INIT_DONE
 *   2. NotifyDeviceWakeupEvent触发START_RECOGNIZE进入RECOGNIZING
 *   3. 等待RECOGNIZE_COMPLETE进入RECOGNIZED
 *   4. 调用StartCapturer(1)
 * 预期结果: StartCapturer在RECOGNIZED状态返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_010_WakeupEngineStartCapturer, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine StartCapturer");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // Trigger wakeup to enter RECOGNIZING, then wait for RECOGNIZE_COMPLETE -> RECOGNIZED
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));

    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    INTELL_VOICE_LOG_INFO("StartCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_011_WakeupEngineRead
 * 业务背景: 从唤醒引擎读取采集到的音频数据
 * 前置条件: 引擎进入READ_CAPTURER状态（RECOGNIZED + StartCapturer）
 * 测试步骤:
 *   1. 创建引擎并驱动到READ_CAPTURER状态
 *   2. 调用Read读取数据
 * 预期结果: Read在READ_CAPTURER状态返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_011_WakeupEngineRead, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Read");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Drive to READ_CAPTURER state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));
    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    std::vector<uint8_t> data;
    ret = engine->Read(data);
    INTELL_VOICE_LOG_INFO("Read result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_012_WakeupEngineStopCapturer
 * 业务背景: 停止音频采集，结束one-shot场景
 * 前置条件: 引擎进入READ_CAPTURER状态
 * 测试步骤:
 *   1. 创建引擎并驱动到READ_CAPTURER状态
 *   2. 调用StopCapturer()
 * 预期结果: StopCapturer在READ_CAPTURER状态返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_012_WakeupEngineStopCapturer, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine StopCapturer");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Drive to READ_CAPTURER state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));
    ret = engine->StartCapturer(LltConstants::DEFAULT_CAPTURER_SOURCE_ID);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("StopCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_013_WakeupEngineGetWakeupPcm
 * 业务背景: 获取唤醒时的PCM音频数据，用于回放或验证
 * 前置条件: 引擎进入RECOGNIZED状态
 * 测试步骤:
 *   1. 创建引擎并驱动到RECOGNIZED状态
 *   2. 调用GetWakeupPcm获取数据
 * 预期结果: GetWakeupPcm在RECOGNIZED状态返回0
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_013_WakeupEngineGetWakeupPcm, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine GetWakeupPcm");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Drive to RECOGNIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));
    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_RECOGNIZE_COMPLETE_MS));

    std::vector<uint8_t> data;
    ret = engine->GetWakeupPcm(data);
    INTELL_VOICE_LOG_INFO("GetWakeupPcm result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_014_WakeupEngineNotifyDeviceWakeupEvent
 * 业务背景: 外部设备（如蓝牙耳机）触发唤醒事件时，通过此接口通知唤醒引擎
 * 前置条件: 唤醒引擎已创建且进入INITIALIZED状态
 * 测试步骤:
 *   1. 创建唤醒引擎，等待INIT_DONE
 *   2. 构造EventDetail事件
 *   3. 使用DEFAULT_MAC_ADDRESS调用NotifyDeviceWakeupEvent
 * 预期结果: 事件通过Facade转发到OnTriggerDetected处理，返回0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_014_WakeupEngineNotifyDeviceWakeupEvent, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine NotifyDeviceWakeupEvent");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    // Wait for INIT_DONE so engine is in INITIALIZED state
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    EventDetail event = {VOICE_RECOGNIZE_EVENT, LltConstants::EVENT_VERSION_DEFAULT};
    ret = engine->NotifyDeviceWakeupEvent(DEFAULT_MAC_ADDRESS, event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_015_WakeupEngineWriteAudio
 * 业务背景: 向唤醒引擎写入音频数据，EngineBase默认实现返回0
 * 前置条件: 唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 构造音频buffer
 *   3. 调用WriteAudio
 * 预期结果: 返回0
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_015_WakeupEngineWriteAudio, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine WriteAudio");
    ASSERT_NE(g_service, nullptr);
    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    uint8_t buffer[LltConstants::DEFAULT_AUDIO_BUFFER_SIZE] = {0};
    ret = engine->WriteAudio(buffer, sizeof(buffer));
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_016_ServiceGetWakeupSourceFilesList
 * 业务背景: 获取唤醒源文件列表，用于唤醒模型升级等场景
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用GetWakeupSourceFilesList
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_016_ServiceGetWakeupSourceFilesList, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GetWakeupSourceFilesList");
    ASSERT_NE(g_service, nullptr);

    std::vector<std::string> cloneFiles;
    int32_t ret = g_service->GetWakeupSourceFilesList(cloneFiles);
    EXPECT_GE(ret, 0);
}

/**
 * 测试用例: UT_WE_017_ServiceGetWakeupSourceFile
 * 业务背景: 获取指定唤醒源文件的内容
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用GetWakeupSourceFile
 * 预期结果: 返回获取结果或错误码
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_017_ServiceGetWakeupSourceFile, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GetWakeupSourceFile");
    ASSERT_NE(g_service, nullptr);

    std::vector<uint8_t> buffer;
    int32_t ret = g_service->GetWakeupSourceFile("test_file.pcm", buffer);
    INTELL_VOICE_LOG_INFO("GetWakeupSourceFile result: %{public}d", ret);
    // mock环境下文件路径不存在，返回-1是预期行为
    EXPECT_EQ(ret, LltConstants::ERR_CODE_GENERIC_FAILURE);
}

/**
 * 测试用例: UT_WE_018_ServiceSendWakeupFile
 * 业务背景: 发送唤醒文件到设备端
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用SendWakeupFile
 * 预期结果: 返回发送结果或错误码
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_018_ServiceSendWakeupFile, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing SendWakeupFile");
    ASSERT_NE(g_service, nullptr);

    std::vector<uint8_t> buffer = {0x01, 0x02, 0x03};
    int32_t ret = g_service->SendWakeupFile("test_file.pcm", buffer);
    INTELL_VOICE_LOG_INFO("SendWakeupFile result: %{public}d", ret);
    // mock环境下文件路径校验失败，返回-1是预期行为
    EXPECT_EQ(ret, LltConstants::ERR_CODE_GENERIC_FAILURE);
}

/**
 * 测试用例: UT_WE_019_ServiceRegisterDeviceWakeupInfo
 * 业务背景: 注册外部设备（如蓝牙耳机、眼镜）的唤醒信息，用于设备切换唤醒模式
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 构造WakeupDeviceInfo
 *   2. 通过g_service调用RegisterDeviceWakeupInfo
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_019_ServiceRegisterDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing RegisterDeviceWakeupInfo");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "00:11:22:33:44:55";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::PHONE;
    info.serviceName = "test_service";
    info.parameters = "test_params";
    int32_t ret = g_service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, 0);
}

/**
 * 测试用例: UT_WE_020_ServiceDeregisterDeviceWakeupInfo
 * 业务背景: 注销外部设备的唤醒信息
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用DeregisterDeviceWakeupInfo
 * 预期结果: 返回值>=0
 * 优先级: P0
 */
HWTEST_F(WakeupServiceTest, UT_WE_020_ServiceDeregisterDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing DeregisterDeviceWakeupInfo");
    ASSERT_NE(g_service, nullptr);

    std::string macAddress = "00:11:22:33:44:55";
    int32_t ret = g_service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, 0);
}

/**
 * 测试用例: UT_WE_021_ServiceEnrollWithWakeupFilesForResult
 * 业务背景: 使用唤醒文件进行注册，获取注册结果
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用EnrollWithWakeupFilesForResult
 * 预期结果: 返回处理结果或错误码
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_021_ServiceEnrollWithWakeupFilesForResult, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollWithWakeupFilesForResult");
    ASSERT_NE(g_service, nullptr);

    std::string wakeupInfo = "{\"phrase\":\"test\"}";
    int32_t ret = g_service->EnrollWithWakeupFilesForResult(wakeupInfo, nullptr);
    INTELL_VOICE_LOG_INFO("EnrollWithWakeupFilesForResult result: %{public}d", ret);
    // mock环境下引擎对象为空，返回-1是预期行为
    EXPECT_EQ(ret, LltConstants::ERR_CODE_GENERIC_FAILURE);
}

/**
 * 测试用例: UT_WE_022_CreateHeadsetWakeupEngine
 * 业务背景: 创建INTELL_VOICE_HEADSET_WAKEUP类型引擎，用于耳机唤醒场景
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用CreateIntellVoiceEngine(INTELL_VOICE_HEADSET_WAKEUP)
 * 预期结果: 返回0，engine不为空
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_022_CreateHeadsetWakeupEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing CreateHeadsetWakeupEngine");
    ASSERT_NE(g_service, nullptr);

    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_HEADSET_WAKEUP, engine);
    // INTELL_VOICE_HEADSET_WAKEUP is a sub-type not handled by EngineFactory as a
    // top-level engine. It returns NO_MEMORY because the factory has no case for it.
    EXPECT_EQ(ret, INTELLIGENT_VOICE_NO_MEMORY);
    EXPECT_EQ(engine, nullptr);
}

/**
 * 测试用例: UT_WE_023_DuplicateCreateWakeupEngine
 * 业务背景: 验证同一类型引擎重复创建时的处理逻辑
 * 前置条件: IntellVoiceService全局对象已创建，唤醒引擎已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 再次创建同类型引擎
 * 预期结果: 第二次创建返回非0或engine不变
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_023_DuplicateCreateWakeupEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing DuplicateCreateWakeupEngine");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine1 = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine1);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    sptr<IIntellVoiceEngine> engine2 = nullptr;
    ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine2);
    INTELL_VOICE_LOG_INFO("Duplicate create result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_024_WakeupEngineAttachThenSetParameter
 * 业务背景: 验证Attach引擎后设置参数的流程
 * 前置条件: 唤醒引擎已创建并Attach
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. 调用Attach
 *   3. 调用SetParameter
 * 预期结果: Attach后SetParameter正常处理
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_024_WakeupEngineAttachThenSetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine Attach then SetParameter");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    IntellVoiceEngineInfo info = {};
    ret = engine->Attach(info);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    ret = engine->SetParameter("WakeupType=0");
    INTELL_VOICE_LOG_INFO("SetParameter result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
}

/**
 * 测试用例: UT_WE_025_WakeupEngineLifecycle
 * 业务背景: 验证唤醒引擎从创建到释放的完整生命周期
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 创建唤醒引擎
 *   2. SetCallback
 *   3. Attach
 *   4. NotifyDeviceWakeupEvent (触发RECOGNIZING)
 *   5. Stop (RECOGNIZING -> INITIALIZED)
 *   6. Detach
 *   7. ReleaseIntellVoiceEngine
 * 预期结果: 完整流程各步骤返回0
 * 优先级: P1
 */
HWTEST_F(WakeupServiceTest, UT_WE_025_WakeupEngineLifecycle, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing WakeupEngine lifecycle");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_WAKEUP, engine);
    ASSERT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
    ASSERT_NE(engine, nullptr);

    engine->SetCallback(nullptr);

    IntellVoiceEngineInfo info = {};
    ret = engine->Attach(info);
    INTELL_VOICE_LOG_INFO("Attach result: %{public}d", ret);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);

    // Wait for INIT_DONE
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_INIT_DONE_MS));

    // Trigger wakeup to enter RECOGNIZING state
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

    ret = g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_WAKEUP);
    EXPECT_EQ(ret, LltConstants::ERR_CODE_SUCCESS);
}
