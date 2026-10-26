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

#include "accessories_cloud_reporter.h"
#include <sstream>
#include <string>
#include <algorithm>
#include "intell_voice_log.h"
#include "file_util.h"
#include "time_util.h"
#include "json/json.h"

#undef LOG_TAG
#define LOG_TAG "Accessories CloudReporter"

using namespace std;
using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t CHANNEL_ID_0 = 0;
static constexpr uint32_t MIN_BUFFER_SIZE = 640;
static constexpr uint32_t FRAME_NUMBER = 100;
const string REPORT_PCM_CACHE = "/report.pcm";
const string CLOUD_REPORT_JSON = "/wakeup.json";
const string CLOUD_REPORT_PCM = "/wakeup.pcm";
const string XYXY_UNICODE = "\u5c0f\u827a\u5c0f\u827a";
const string INTELL_VOICE_DEFAULT_DATA_PATH = "/data/service/el0/intell_voice/report";
const uint32_t WAKEUP_PCM_SAVE_NUM_MAX = 102;
const uint32_t WAKEUP_PCM_REPORT_NUM_MAX = 103;
#define UNUSED_PARAMETER(x) (void)(x)

void AccessoriesCloudReporter::SetPath()
{
    mReportPath = INTELL_VOICE_DEFAULT_DATA_PATH;
}

bool AccessoriesCloudReporter::InitRootDir()
{
    if (!FileUtil::CheckAndCreateDir(mReportPath)) {
        INTELL_VOICE_LOG_INFO("report dir make fail");
        return false;
    }

    string subDir = mReportPath + "/";
    if (!FileUtil::CheckAndCreateDir(subDir)) {
        INTELL_VOICE_LOG_WARN("wakeup dir make fail");
        return false;
    }

    vector<string> dirFile;
    FileUtil::GetDirFilePath(subDir, dirFile);
    unsigned int fileNum = dirFile.size();
    if (fileNum >= WAKEUP_PCM_REPORT_NUM_MAX) {
        INTELL_VOICE_LOG_WARN("PCM report too many, %d, maxNum:%d", fileNum, WAKEUP_PCM_REPORT_NUM_MAX);
        ClosePcm();
        mReportDir = "";
        return false;
    }

    mReportDir = subDir;
    return true;
}

void AccessoriesCloudReporter::Init()
{
    try {
        mReportInfo.clear();
        mReportInfo["type"] = deviceType_;
        mReportInfo["time"] = TimeUtil::GetCurrTime(TIME_FORMAT_STANDARD);
        mReportInfo["text"] = XYXY_UNICODE;
    } catch (...) {
        INTELL_VOICE_LOG_ERROR("init report info, logic error");
        return;
    }

    SetPath();
    if (!InitRootDir()) {
        ClosePcm();
        return;
    }
    beginIndex_ = 0;
    endIndex_ = 0;
    indexCount_ = 0;
    CreatePcm(MIN_BUFFER_SIZE * FRAME_NUMBER);
    mTimeStamp = TimeUtil::GetCurrTime(TIME_FORMAT_CONTINOUS);
}

void AccessoriesCloudReporter::CreatePcm(uint32_t maxPcmBufferSize)
{
    ClosePcm();
    try {
        mPcmBuffer = std::make_unique<PcmBuffer>(maxPcmBufferSize);
    } catch (const std::bad_alloc&) {
        INTELL_VOICE_LOG_ERROR("Make unique bad alloc");
    } catch (const std::length_error& err) {
        INTELL_VOICE_LOG_ERROR("Make unique length error");
    }
}

void AccessoriesCloudReporter::ClosePcm()
{
    mPcmBuffer.reset();
}

void AccessoriesCloudReporter::SetEndIndex(int32_t endIndex)
{
    if (endIndex == 0) {
        INTELL_VOICE_LOG_WARN("endindex invalid");
        return;
    }
    endIndex_ = endIndex;
    beginIndex_ = (endIndex_ >= FRAME_NUMBER - 1) ? (endIndex_ - (FRAME_NUMBER - 1)) : 0;
    INTELL_VOICE_LOG_INFO("set end index = %{public}d, begin_index = %{public}d", endIndex_, beginIndex_);
}

void AccessoriesCloudReporter::SetEndIndex(const std::string& json)
{
    std::istringstream jsonStrm(json);
    Json::Value root;
    std::string errs;
    Json::CharReaderBuilder reader;
    reader["collectComments"] = false;

    if (!Json::parseFromStream(reader, jsonStrm, &root, &errs)) {
        INTELL_VOICE_LOG_ERROR("parse From Stream failed");
        return;
    }

    const Json::Value& endIndex = root.get("VprInfo", Json::Value::null).get("EndFrame", 0);
    if (endIndex.asInt()) {
        endIndex_ = endIndex.asInt();
        beginIndex_ = (endIndex_ >= FRAME_NUMBER - 1) ? (endIndex_ - (FRAME_NUMBER - 1)) : 0;
        INTELL_VOICE_LOG_INFO("set end index = %{public}d, begin_index = %{public}d", endIndex_, beginIndex_);
    }
    mReportInfo["labels"] = json;
}

void AccessoriesCloudReporter::WriteStream(const vector<vector<uint8_t>> &audioData)
{
    if (mPcmBuffer == nullptr || endIndex_ == 0) {
        return;
    }
    if (audioData.empty()) {
        INTELL_VOICE_LOG_ERROR("audioData is empty");
        return;
    }
    if (indexCount_ >= beginIndex_ && indexCount_ <= endIndex_) {
        const uint8_t* data = audioData[CHANNEL_ID_0].data();
        size_t len = audioData[CHANNEL_ID_0].size();
        mPcmBuffer->Write(data, len);
    }
    indexCount_ += 1;
}


bool AccessoriesCloudReporter::SavePcm()
{
    if (mPcmBuffer == nullptr || mReportDir.empty() || endIndex_ == 0) {
        INTELL_VOICE_LOG_WARN("save pcm fail, reset");
        FileUtil::DeleteDir(mReportDir.c_str());
        ClosePcm();
        mReportDir = "";
        return false;
    }
    bool needCrypt = true;
#ifdef HP_AUDIO_DATA_NDEBUG
    needCrypt = false;
#endif
    INTELL_VOICE_LOG_INFO("save report pcm");
    string des = mReportDir + CLOUD_REPORT_PCM;
    return mPcmBuffer->SaveToFile(needCrypt, des);
}

void AccessoriesCloudReporter::DeleteEarliestDir(const uint32_t maxNum)
{
    string subDir = mReportPath + "/";
    vector<string> dirFile;
    FileUtil::GetDirFilePath(subDir, dirFile);
    unsigned int fileNum = dirFile.size();
    if (fileNum <= maxNum) {
        return;
    }
    string str = dirFile[0];
    for (const auto &path : dirFile) {
        if (path < str) {
            str = path;
        }
    }
    INTELL_VOICE_LOG_DEBUG("dirFile str: %s", str.c_str());
    FileUtil::DeleteDir(str.c_str());
}

bool AccessoriesCloudReporter::CreateReportDir()
{
    if (mTimeStamp.empty()) {
        return false;
    }

    mReportDir = mReportDir + mTimeStamp + "/";
    if (!FileUtil::CheckAndCreateDir(mReportDir)) {
        INTELL_VOICE_LOG_INFO("report dir make fail");
        return false;
    }
    return true;
}

bool AccessoriesCloudReporter::SetReportInfo()
{
    INTELL_VOICE_LOG_INFO("enter");
    if (mPcmBuffer == nullptr || mReportDir.empty() || endIndex_ == 0) {
        INTELL_VOICE_LOG_WARN("set report info fail, reset");
        FileUtil::DeleteDir(mReportDir.c_str());
        ClosePcm();
        mReportDir = "";
        return false;
    }

    try {
        mReportInfo["sound"] = "wakeup.pcm";
        ofstream out(mReportDir + CLOUD_REPORT_JSON);
        if (!out.is_open()) {
            FileUtil::DeleteDir(mReportDir.c_str());
            INTELL_VOICE_LOG_WARN("fail open json");
            return false;
        }

        Json::Value value;
        value["wakeups"].append(mReportInfo);
        out << value.toStyledString();
        out.close();
    } catch (...) {
        INTELL_VOICE_LOG_ERROR("json error");
        return false;
    }

    DeleteEarliestDir(WAKEUP_PCM_SAVE_NUM_MAX);
    mReportPath.clear();
    ClosePcm();
    return true;
}
}
}

