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

#ifndef ACCESSORIES_CLOUD_REPORTER_H
#define ACCESSORIES_CLOUD_REPORTER_H
#include "json/json.h"
#include "pcm_buffer.h"


namespace OHOS {
namespace IntellVoiceEngine {
class AccessoriesCloudReporter {
public:
    explicit AccessoriesCloudReporter(const std::string& deviceType) : deviceType_(deviceType) {}
    ~AccessoriesCloudReporter() = default;
    void SetPath();
    void Init();
    void SetEndIndex(int32_t endIndex);
    void SetEndIndex(const std::string& json);
    void WriteStream(const std::vector<std::vector<uint8_t>> &audioData);
    bool CreateReportDir();
    bool SavePcm();
    bool SetReportInfo();

private:
    bool InitRootDir();
    void DeleteEarliestDir(const uint32_t maxNum);
    void CreatePcm(uint32_t maxPcmBufferSize);
    void ClosePcm();

private:
    std::string mReportDir { "" };
    std::string mReportPath { "" };
    std::string mTimeStamp { "" };
    std::unique_ptr<IntellVoiceUtils::PcmBuffer> mPcmBuffer;
    Json::Value mReportInfo;
    std::string deviceType_;
    int32_t endIndex_ = 0;
    int32_t beginIndex_ = 0;
    int32_t indexCount_ = 0;

};
}
}
#endif