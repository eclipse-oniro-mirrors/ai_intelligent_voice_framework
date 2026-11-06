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

#include "trigger_manager_test.h"

#include <fstream>
#include <iostream>

#include "intell_voice_log.h"
#include "trigger_base_type.h"
#include "trigger_detector.h"
#include "trigger_detector_callback.h"

#define LOG_TAG "TriggerManager"
#define private public

using namespace std;

namespace OHOS {
namespace IntellVoiceTrigger {

TriggerManagerTest::TriggerManagerTest()
{
    triggerManager_ = std::make_shared<TriggerManager>();
    if (triggerManager_ == nullptr) {
        return;
    }
    triggerService_ = triggerManager_->service_;
    if (triggerService_ == nullptr) {
        return;
    }
    triggerHelper_ = triggerService_->triggerHelper_;
}

TriggerManagerTest::~TriggerManagerTest()
{
    triggerManager_ = nullptr;
    triggerService_ = nullptr;
    triggerHelper_ = nullptr;
    triggerDetector_ = nullptr;
}

bool TriggerManagerTest::InitRecognition(int32_t uuid)
{
    if (triggerManager_ == nullptr) {
        return false;
    }

    auto data = ReadFile("/data/test/resource/model_one.txt");

    triggerManager_->UpdateModel(data, uuid, TriggerModelType::VOICE_WAKEUP_TYPE);
    triggerManager_->CreateDetector(uuid, [this, uuid]() { OnDetected(uuid); });
    triggerDetector_ = triggerManager_->detectors_[uuid];
    if (triggerDetector_ == nullptr) {
        return false;
    }
    return true;
}

bool TriggerManagerTest::StartRecognition()
{
    if (triggerDetector_ == nullptr) {
        return false;
    }
    return triggerDetector_->StartRecognition();
}

bool TriggerManagerTest::StopRecognition()
{
    if (triggerDetector_ == nullptr) {
        return false;
    }
    return triggerDetector_->StopRecognition();
}

bool TriggerManagerTest::UnloadTriggerModel()
{
    if (triggerDetector_ == nullptr) {
        return false;
    }
    triggerDetector_->UnloadTriggerModel();
    return true;
}

bool TriggerManagerTest::DeleteModel(int32_t uuid)
{
    if (triggerManager_ == nullptr) {
        return false;
    }
    triggerManager_->DeleteModel(uuid);
    return true;
}

bool TriggerManagerTest::OnCallStateUpdated(int32_t callState)
{
    if (triggerHelper_ == nullptr) {
        return false;
    }
    triggerHelper_->OnCallStateUpdated(callState);
    return true;
}

bool TriggerManagerTest::OnCapturerStateChange(bool isActive)
{
    if (triggerHelper_ == nullptr) {
        return false;
    }
    triggerHelper_->OnCapturerStateChange(isActive);
    return true;
}

ModelState TriggerManagerTest::GetState(int32_t uuid)
{
    auto modelData = triggerHelper_->GetTriggerModelData(uuid);
    auto state = modelData->GetState();
    INTELL_VOICE_LOG_INFO("state: %{public}d", state);
    return modelData->GetState();
}

std::vector<uint8_t> TriggerManagerTest::ReadFile(const std::string &path)
{
    INTELL_VOICE_LOG_INFO("path: %{public}s", path.c_str());
    ifstream infile;
    infile.open(path, ios::in);

    if (!infile.is_open()) {
        INTELL_VOICE_LOG_INFO("open file failed");
    }

    infile.seekg(0, infile.end);
    uint32_t modelSize = static_cast<uint32_t>(infile.tellg());
    std::vector<uint8_t> datas(modelSize);
    infile.seekg(0, infile.beg);
    infile.read(reinterpret_cast<char *>(datas.data()), modelSize);
    infile.close();

    return datas;
}

void TriggerManagerTest::OnDetected(int32_t uuid)
{}

}  // namespace IntellVoiceTrigger
}  // namespace OHOS
