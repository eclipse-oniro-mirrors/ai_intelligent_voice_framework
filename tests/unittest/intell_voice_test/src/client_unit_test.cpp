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
#include <gtest/gtest.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>

#include "intell_voice_log.h"
#include "i_intell_voice_engine.h"
#include "i_intell_voice_service.h"
#include "engine_callback_inner.h"
#include "engine_event_callback.h"
#include "wait_for_result.h"
#include "intell_voice_manager.h"

#define LOG_TAG "ClientTest"

using namespace OHOS::IntellVoiceTests;
using namespace OHOS::IntellVoice;
using namespace OHOS::IntellVoiceEngine;
using namespace OHOS;
using namespace testing::ext;

static sptr<IIntellVoiceService> g_sProxy = nullptr;

class ClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    std::shared_ptr<EngineEventCallback> cb_ = nullptr;
};

void ClientTest::SetUpTestCase(void)
{
    INTELL_VOICE_LOG_INFO("hello harmony OS!\n");
}

void ClientTest::TearDownTestCase(void)
{
}

void ClientTest::SetUp(void)
{
}

void ClientTest::TearDown(void)
{
}

HWTEST_F(ClientTest, ClientUtils, TestSize.Level1)
{
    WaitForResult waitForResult;

    sptr<IIntellVoiceEngine> engine;
    IntellVoiceManager::GetInstance()->CreateIntellVoiceEngine(INTELL_VOICE_ENROLL, engine);

    INTELL_VOICE_LOG_INFO("end\n");
}

HWTEST_F(ClientTest, CloudReporter, TestSize.Level1)
{
    std::vector<UploadFilesInfo> reportedFilesInfo;
    IntellVoiceManager::GetInstance()->GetUploadFiles(12,reportedFilesInfo);
}