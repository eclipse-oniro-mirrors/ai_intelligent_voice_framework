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

#include "intell_voice_service.h"
#include "intell_voice_log.h"
#include "hdi_mock_init.h"

#define LOG_TAG "IntellVoiceServiceTest"

using namespace OHOS::IntellVoiceEngine;
using namespace OHOS;
using namespace testing::ext;

class IntellVoiceServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
};

void IntellVoiceServiceTest::SetUpTestCase(void)
{
    INTELL_VOICE_LOG_INFO("IntellVoiceService Test Setup");
    InitHdiMock();
}

void IntellVoiceServiceTest::TearDownTestCase(void)
{
}

void IntellVoiceServiceTest::SetUp(void)
{
}

void IntellVoiceServiceTest::TearDown(void)
{
}

HWTEST_F(IntellVoiceServiceTest, Service_CreateInstance, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing IntellVoiceService CreateInstance");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    EXPECT_NE(service, nullptr);
}

HWTEST_F(IntellVoiceServiceTest, Service_CreateIntellVoiceEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing CreateIntellVoiceEngine");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    sptr<IIntellVoiceEngine> engine = nullptr;
    int32_t ret = service->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(engine, nullptr);
}

HWTEST_F(IntellVoiceServiceTest, Service_ReleaseIntellVoiceEngine, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing ReleaseIntellVoiceEngine");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    int32_t ret = service->ReleaseIntellVoiceEngine(INTELL_VOICE_ENROLL);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(IntellVoiceServiceTest, Service_GetSetParameter, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GetParameter and SetParameter");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    // SetParameter returns 0 for any key (no-op without wakeup engine)
    int32_t ret = service->SetParameter("test.key=test.value");
    EXPECT_EQ(ret, 0);

    // GetParameter handles known service-level keys:
    // "isEnrolled" - checks HistoryInfoMgr
    std::string enrolled = service->GetParameter("isEnrolled");
    EXPECT_TRUE(enrolled == "true" || enrolled == "false");

    // "isSupportImplicitEnroll" - checks if HDI V1_3 proxy exists
    std::string supportImplicit = service->GetParameter("isSupportImplicitEnroll");
    EXPECT_TRUE(supportImplicit == "true" || supportImplicit == "false");
}

HWTEST_F(IntellVoiceServiceTest, Service_ClearUserData, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing ClearUserData");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    int32_t ret = service->ClearUserData();
    EXPECT_EQ(ret, 0);
}

HWTEST_F(IntellVoiceServiceTest, Service_GetUploadFiles, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GetUploadFiles");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    std::vector<UploadFilesFromHdi> files;
    int32_t ret = service->GetUploadFiles(10, files);
    EXPECT_GE(ret, 0);
}

HWTEST_F(IntellVoiceServiceTest, Service_GetWakeupSourceFilesList, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing GetWakeupSourceFilesList");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    std::vector<std::string> cloneFiles;
    int32_t ret = service->GetWakeupSourceFilesList(cloneFiles);
    EXPECT_GE(ret, 0);
}

HWTEST_F(IntellVoiceServiceTest, Service_RegisterDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing RegisterDeviceWakeupInfo");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    std::string macAddress = "00:11:22:33:44:55";
    WakeupDeviceInfo info = {};
    info.type = WakeupDeviceType::PHONE;
    info.serviceName = "test_service";
    info.parameters = "test_params";

    int32_t ret = service->RegisterDeviceWakeupInfo(macAddress, info);
    EXPECT_GE(ret, 0);
}

HWTEST_F(IntellVoiceServiceTest, Service_DeregisterDeviceWakeupInfo, TestSize.Level1)
{
    INTELL_VOICE_LOG_INFO("Testing DeregisterDeviceWakeupInfo");
    auto service = std::make_unique<IntellVoiceService>(INTELL_VOICE_SERVICE_ID, true);
    ASSERT_NE(service, nullptr);

    std::string macAddress = "00:11:22:33:44:55";
    int32_t ret = service->DeregisterDeviceWakeupInfo(macAddress);
    EXPECT_GE(ret, 0);
}
