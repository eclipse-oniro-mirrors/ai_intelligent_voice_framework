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

#include "intell_voice_service.h"
#include "i_intell_voice_engine.h"
#include "intell_voice_log.h"
#include "intell_voice_info.h"
#include "i_device_wakeup.h"
#include "hdi_mock_init.h"

#define LOG_TAG "EnrollServiceTest"

using namespace OHOS::IntellVoiceEngine;
using namespace OHOS::IntellVoice;
using namespace OHOS;
using namespace testing::ext;

class EnrollServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    static inline std::unique_ptr<IntellVoiceService> g_service = nullptr;
};

void EnrollServiceTest::SetUpTestCase(void)
{
    INTELL_VOICE_LOG_INFO("EnrollServiceTest Setup");
    InitHdiMock();
    g_service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
}

void EnrollServiceTest::TearDownTestCase(void)
{
    INTELL_VOICE_LOG_INFO("EnrollServiceTest TearDown");
    g_service.reset();
}

void EnrollServiceTest::SetUp(void)
{
}

void EnrollServiceTest::TearDown(void)
{
}

/**
 * 测试用例: UT_EE_001_CreateEnrollEngine
 * 业务背景: 通过IntellVoiceService创建INTELL_VOICE_ENROLL类型引擎，验证注册引擎可正常创建
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用CreateIntellVoiceEngine(INTELL_VOICE_ENROLL)
 *   2. 验证返回值为0且engine不为空
 * 预期结果: 返回0，engine对象有效
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_001_CreateEnrollEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing CreateEnrollEngine");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(engine, nullptr);
}

/**
 * 测试用例: UT_EE_002_ReleaseEnrollEngine
 * 业务背景: 释放INTELL_VOICE_ENROLL类型引擎，验证资源正常释放
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 通过g_service调用ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL)
 *   2. 验证返回值为0
 * 预期结果: 返回0
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_002_ReleaseEnrollEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing ReleaseEnrollEngine");
    ASSERT_NE(g_service, nullptr);

    int32_t ret = g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
    EXPECT_EQ(ret, 0);
}

/**
 * 测试用例: UT_EE_003_EnrollEngineSetCallback
 * 业务背景: 注册引擎设置回调接收注册结果事件
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用SetCallback(nullptr)
 * 预期结果: 不崩溃
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_003_EnrollEngineSetCallback, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine SetCallback");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    engine->SetCallback(nullptr);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_004_EnrollEngineAttach
 * 业务背景: 注册引擎Attach附加引擎配置信息，用于注册流程初始化
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 构造IntellVoiceEngineInfo
 *   3. 调用Attach
 * 预期结果: Attach成功返回0
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_004_EnrollEngineAttach, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Attach");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    IntellVoiceEngineInfo info = {};
    info.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    info.minBufSize = 640;
    info.sampleChannels = 1;
    info.bitsPerSample = 16;
    info.sampleRate = 16000;
    ret = engine->Attach(info);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_005_EnrollEngineStart
 * 业务背景: 注册引擎启动注册流程
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用Start(true)
 * 预期结果: 返回0
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_005_EnrollEngineStart, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Start");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->Start(true);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_006_EnrollEngineStop
 * 业务背景: 注册引擎停止注册流程
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用Stop()
 * 预期结果: 不崩溃
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_006_EnrollEngineStop, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Stop");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->Stop();
    INTELL_VOICE_LOG_INFO("Stop result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_007_EnrollEngineWriteAudio
 * 业务背景: 向注册引擎写入音频数据用于声纹注册
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 构造音频buffer
 *   3. 调用WriteAudio
 * 预期结果: EngineBase默认实现返回0
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_007_EnrollEngineWriteAudio, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine WriteAudio");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    uint8_t buffer[640] = {0};
    ret = engine->WriteAudio(buffer, sizeof(buffer));
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_008_EnrollEngineSetParameter
 * 业务背景: 注册引擎设置运行参数
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用SetParameter("WakeupType=0")
 * 预期结果: SetParameter通过状态机处理
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_008_EnrollEngineSetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine SetParameter");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->SetParameter("WakeupType=0");
    INTELL_VOICE_LOG_INFO("SetParameter result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_009_EnrollEngineGetParameter
 * 业务背景: 注册引擎获取运行参数
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用GetParameter("wakeup_source_channel")
 * 预期结果: GetParameter通过状态机处理，不崩溃
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_009_EnrollEngineGetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine GetParameter");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    std::string result = engine->GetParameter("wakeup_source_channel");
    INTELL_VOICE_LOG_INFO("GetParameter result: %{public}s", result.c_str());

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_010_EnrollEngineDetach
 * 业务背景: 注册引擎分离，释放引擎资源
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用Detach()
 * 预期结果: Detach通过状态机处理RELEASE
 * 优先级: P0
 */
HWTEST_F(EnrollServiceTest, UT_EE_010_EnrollEngineDetach, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Detach");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->Detach();
    INTELL_VOICE_LOG_INFO("Detach result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_011_EnrollEngineEvaluate
 * 业务背景: 评估注册词的语音质量分数，用于注册结果校验
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用Evaluate传入评估词
 * 预期结果: Evaluate通过V1_2 mock adapter处理，返回0
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_011_EnrollEngineEvaluate, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Evaluate");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    EvaluationResult result = {};
    ret = engine->Evaluate("\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA", result);
    INTELL_VOICE_LOG_INFO("Evaluate result: %{public}d, score: %{public}d, code: %{public}d",
        ret, result.score, result.resultCode);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_012_EnrollEngineNotifyDeviceWakeupEvent
 * 业务背景: 注册引擎接收设备唤醒事件通知
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 构造EventDetail事件（蓝牙HFP识别事件）
 *   3. 调用NotifyDeviceWakeupEvent
 * 预期结果: 事件通过EngineBase的NotifyDeviceWakeupEvent处理
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_012_EnrollEngineNotifyDeviceWakeupEvent, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine NotifyDeviceWakeupEvent");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    EventDetail event = {BLUETOOTH_HFP_RECOGNIZE_EVENT, 100};
    ret = engine->NotifyDeviceWakeupEvent("00:11:22:33:44:66", event);
    INTELL_VOICE_LOG_INFO("NotifyDeviceWakeupEvent result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_013_EnrollEngineStartCapturer
 * 业务背景: 注册引擎启动音频采集，用于注册过程中的音频数据获取
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用StartCapturer(1)
 * 预期结果: StartCapturer通过EngineBase处理
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_013_EnrollEngineStartCapturer, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine StartCapturer");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->StartCapturer(1);
    INTELL_VOICE_LOG_INFO("StartCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_014_EnrollEngineRead
 * 业务背景: 从注册引擎读取采集到的音频数据
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用Read读取数据
 * 预期结果: Read通过EngineBase处理
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_014_EnrollEngineRead, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine Read");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    std::vector<uint8_t> data;
    ret = engine->Read(data);
    INTELL_VOICE_LOG_INFO("Read result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_015_EnrollEngineStopCapturer
 * 业务背景: 注册引擎停止音频采集
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用StopCapturer()
 * 预期结果: StopCapturer通过EngineBase处理
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_015_EnrollEngineStopCapturer, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine StopCapturer");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    ret = engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("StopCapturer result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_016_EnrollEngineGetWakeupPcm
 * 业务背景: 注册引擎获取唤醒PCM数据
 * 前置条件: 注册引擎已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. 调用GetWakeupPcm
 * 预期结果: GetWakeupPcm通过EngineBase处理
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_016_EnrollEngineGetWakeupPcm, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine GetWakeupPcm");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    std::vector<uint8_t> data;
    ret = engine->GetWakeupPcm(data);
    INTELL_VOICE_LOG_INFO("GetWakeupPcm result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    // Release engine to avoid affecting subsequent tests
    g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
}

/**
 * 测试用例: UT_EE_017_EnrollEngineLifecycle
 * 业务背景: 验证注册引擎从创建到释放的完整生命周期
 * 前置条件: IntellVoiceService全局对象已创建
 * 测试步骤:
 *   1. 创建注册引擎
 *   2. SetCallback
 *   3. Attach
 *   4. Start
 *   5. WriteAudio
 *   6. Evaluate
 *   7. Stop
 *   8. Detach
 *   9. ReleaseIntellVoiceEngine
 * 预期结果: 完整流程各步骤返回0
 * 优先级: P1
 */
HWTEST_F(EnrollServiceTest, UT_EE_017_EnrollEngineLifecycle, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing EnrollEngine lifecycle");
    ASSERT_NE(g_service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = g_service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(engine, nullptr);

    engine->SetCallback(nullptr);

    IntellVoiceEngineInfo info = {};
    info.wakeupPhrase = "\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA";
    info.minBufSize = 640;
    info.sampleChannels = 1;
    info.bitsPerSample = 16;
    info.sampleRate = 16000;
    ret = engine->Attach(info);
    INTELL_VOICE_LOG_INFO("Attach result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    ret = engine->Start(true);
    INTELL_VOICE_LOG_INFO("Start result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    uint8_t buffer[640] = {0};
    ret = engine->WriteAudio(buffer, sizeof(buffer));
    INTELL_VOICE_LOG_INFO("WriteAudio result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    EvaluationResult evalResult = {};
    ret = engine->Evaluate("\xE5\xB0\x8F\xE8\x89\xBA\xE5\xB0\x8F\xE8\x89\xBA", evalResult);
    INTELL_VOICE_LOG_INFO("Evaluate result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    ret = engine->Stop();
    INTELL_VOICE_LOG_INFO("Stop result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    ret = engine->Detach();
    INTELL_VOICE_LOG_INFO("Detach result: %{public}d", ret);
    EXPECT_EQ(ret, 0);

    ret = g_service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
    EXPECT_EQ(ret, 0);
}
