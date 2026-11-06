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
#include "wakeup_engine_test.h"

#include "intell_voice_log.h"
#include "wakeup_intell_voice_engine.h"

#define LOG_TAG "TestWakeupEngine"

using namespace OHOS;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceEngine;
using OHOS::IntellVoiceEngine::EvaluationResult;

namespace OHOS {
namespace IntellVoiceFuzzTest {

constexpr int INTELL_WAKEUP_TEST_RANDOM_NUM = 12;
static WakeupIntellVoiceEngine *g_engine = new WakeupIntellVoiceEngine({true, "小艺小艺"});

class TestWakeupEngineEventCallback : public IIntellVoiceEngineEventCallback {
private:
    void OnEvent(const IntellVoiceEngineCallBackEvent &param) override;
};

void TestWakeupEngineEventCallback::OnEvent(const IntellVoiceEngineCallBackEvent &param)
{}

static void TestSetSensibility(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetSensibility start");
    int32_t ret = g_engine->SetSensibility(static_cast<int32_t>(*data));
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetSensibility end, result:%{public}d", ret);
}

static void TestSetWakeupHapInfo(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetWakeupHapInfo start");
    WakeupHapInfo config = {"com.huawei.hmos.aibase", "WakeUpExtAbility"};
    int32_t ret = g_engine->SetWakeupHapInfo(config);
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetWakeupHapInfo end, result:%{public}d", ret);
}

static void TestSetParameter(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetParameter start");
    std::string key = "key";
    std::string value = "123456";
    int32_t ret = g_engine->SetParameter(key, value);
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetParameter end, result:%{public}d", ret);
}

static void TestGetParameter(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test GetParameter start");
    std::string key = "key";
    std::string value = g_engine->GetParameter(key);
    INTELL_VOICE_LOG_INFO("WakeupEngine test GetParameter end, result:%{public}s", value.c_str());
}

static void TestRelease(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test Release start");
    int32_t ret = g_engine->Release();
    INTELL_VOICE_LOG_INFO("WakeupEngine test Release end, result:%{public}d", ret);
}

static void TestSetCallback(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetCallback start");
    std::shared_ptr<TestWakeupEngineEventCallback> cb = std::make_shared<TestWakeupEngineEventCallback>();
    int32_t ret = g_engine->SetCallback(cb);
    INTELL_VOICE_LOG_INFO("WakeupEngine test SetCallback end, result:%{public}d", ret);
}

static void TestStartCapturer(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test StartCapturer start");
    sptr<IRemoteObject> object;
    int32_t ret = g_engine->StartCapturer(static_cast<int32_t>(*data));
    INTELL_VOICE_LOG_INFO("WakeupEngine test StartCapturer end, result:%{public}d", ret);
}

static void TestRead(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test Read start");
    std::vector<uint8_t> buffer;
    int32_t ret = g_engine->Read(buffer);
    INTELL_VOICE_LOG_INFO("WakeupEngine test Read end, result:%{public}d", ret);
}

static void TestStopCapturer(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test StopCapturer start");
    int32_t ret = g_engine->StopCapturer();
    INTELL_VOICE_LOG_INFO("WakeupEngine test StopCapturer end, result:%{public}d", ret);
}

static void TestGetWakeupPcm(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("WakeupEngine test GetWakeupPcm start");
    std::vector<uint8_t> buffer;
    int32_t ret = g_engine->GetWakeupPcm(buffer);
    INTELL_VOICE_LOG_INFO("WakeupEngine test GetWakeupPcm end, result:%{public}d", ret);
}

namespace {
void (*g_wakeupFuncTable[])(const uint8_t *, size_t) = {TestSetSensibility,
    TestSetWakeupHapInfo,
    TestSetParameter,
    TestGetParameter,
    TestRelease,
    TestSetCallback,
    TestStartCapturer,
    TestRead,
    TestStopCapturer,
    TestGetWakeupPcm};
}
void TestWakeupEngineRandomFuzzer(const uint8_t *data, size_t sizeIn)
{
    INTELL_VOICE_LOG_INFO("TestWakeupRandomFuzzer in");
    if (data == nullptr) {
        return;
    }
    uint8_t type = data[0] % INTELL_WAKEUP_TEST_RANDOM_NUM;
    INTELL_VOICE_LOG_INFO("function id: %{public}d", type);
    (*g_wakeupFuncTable[type])(data, sizeIn);
    return;
}
}  // namespace IntellVoiceFuzzTest
}  // namespace OHOS