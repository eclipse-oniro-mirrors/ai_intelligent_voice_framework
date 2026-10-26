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
#include "wakeup_source_process.h"
#include "intell_voice_log.h"

#define LOG_TAG "WakeupSourceProc"

using namespace OHOS::IntellVoiceUtils;

namespace OHOS {
namespace IntellVoiceEngine {
static constexpr uint32_t WAIT_TIME = 1000;  // 1000ms
static constexpr uint32_t CHANNEL_CNT_1 = 1;
static constexpr uint32_t CHANNEL_CNT_4 = 4;
static constexpr uint32_t MAX_CHANNEL_CNT = 4;
static constexpr uint32_t FRAME_CNT = 100;
static const std::string WRITE_SOURCE = "_write_source";
static const std::string READ_SOURCE = "_read_source";

WakeupSourceProcess::~WakeupSourceProcess()
{
    Release();
}

void WakeupSourceProcess::Init(uint32_t channelCnt)
{
    if (channelCnt_ != 0) {
        INTELL_VOICE_LOG_WARN("need to release before init, channel cnt:%{public}u", channelCnt_);
        Release();
    }
    for (uint32_t i = 0; i < channelCnt; i++) {
        auto queue = std::make_unique<Uint8ArrayBufferQueue>();
        if (queue == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to create buffer queue");
            return;
        }
        queue->Init();
        bufferQueue_.push_back(std::move(queue));
    }
    channelCnt_ = channelCnt;
    InitDebugFile(channelCnt);
}

void WakeupSourceProcess::Write(const std::vector<std::vector<uint8_t>> &audioData)
{
    if ((static_cast<uint32_t>(audioData.size()) != channelCnt_) ||
        (static_cast<uint32_t>(bufferQueue_.size()) != channelCnt_)) {
        INTELL_VOICE_LOG_ERROR("data size:%{public}u, queue size:%{public}u, channel cnt:%{public}u",
            static_cast<uint32_t>(audioData.size()), static_cast<uint32_t>(bufferQueue_.size()), channelCnt_);
        return;
    }

    if (channelCnt_ == CHANNEL_CNT_1) {
        WriteChannelData(audioData[CHANNEL_ID_0], CHANNEL_ID_0);
    } else if (channelCnt_ == CHANNEL_CNT_4) {
        WriteChannelData(audioData[CHANNEL_ID_0], CHANNEL_ID_0);
        WriteChannelData(audioData[CHANNEL_ID_1], CHANNEL_ID_1);
        WriteChannelData(audioData[CHANNEL_ID_2], CHANNEL_ID_2);
        WriteChannelData(audioData[CHANNEL_ID_3], CHANNEL_ID_3);
    } else {
        INTELL_VOICE_LOG_WARN("not support channel cnt:%{public}u", channelCnt_);
    }
}

int32_t WakeupSourceProcess::Read(std::vector<uint8_t> &data, int32_t readChannel)
{
    INTELL_VOICE_LOG_DEBUG("enter, read channel:%{public}d", readChannel);
    if ((readChannel == 0) || (readChannel >= (0x1 << MAX_CHANNEL_CNT))) {
        return -1;
    }

    for (uint32_t i = 0; i < CHANNEL_CNT_4; i++) {
        if (!(readChannel & (0x1 << i))) {
            continue;
        }
        if (!ReadChannelData(data, i)) {
            return -1;
        }
    }

    return 0;
}

void WakeupSourceProcess::Release()
{
    for (auto &queue : bufferQueue_) {
        queue ->Uninit();
    }
    std::vector<std::unique_ptr<Uint8ArrayBufferQueue>>().swap(bufferQueue_);
    ReleaseDebugFile();
    channelCnt_ = 0;
}

void WakeupSourceProcess::WriteChannelData(const std::vector<uint8_t> &channelData, uint32_t channelId)
{
    if ((channelId >= bufferQueue_.size()) || (bufferQueue_[channelId] == nullptr)) {
        INTELL_VOICE_LOG_ERROR("no buffer queue, channel id:%{public}d", channelId);
        return;
    }

    auto arrayBuffer = CreateArrayBuffer<uint8_t>(channelData.size(), &channelData[0]);
    if (arrayBuffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create array buffer");
        return;
    }
    if (!bufferQueue_[channelId]->Push(std::move(arrayBuffer), false)) {
        return;
    }

    WriteDebugData(writeDebug_, channelData, channelId);
}

bool WakeupSourceProcess::ReadChannelData(std::vector<uint8_t> &channelData, uint32_t channelId)
{
    if ((channelId >= bufferQueue_.size()) || (bufferQueue_[channelId] == nullptr)) {
        INTELL_VOICE_LOG_ERROR("no buffer queue, channel id:%{public}d", channelId);
        return false;
    }
    if (readBufferPCM) {
        ReadBufferedData(channelData, channelId);
        readBufferPCM = false;
    } else {
        std::unique_ptr<Uint8ArrayBuffer> arrayBuffer = nullptr;
        bufferQueue_[channelId]->PopUntilTimeout(WAIT_TIME, arrayBuffer);
        if (arrayBuffer == nullptr) {
            INTELL_VOICE_LOG_ERROR("failed to pop data");
            return false;
        }
        channelData.insert(channelData.end(), arrayBuffer->GetData(), arrayBuffer->GetData() + arrayBuffer->GetSize());
    }

    WriteDebugData(readDebug_, channelData, channelId);
    return true;
}

void WakeupSourceProcess::ReadBufferedData(std::vector<uint8_t> &channelData, uint32_t channelId)
{
    int frameLen = FRAME_CNT;
    if (channelId != CHANNEL_ID_0) {
        // channel-0
        frameLen = AddMultiFrame(channelData, CHANNEL_ID_0, frameLen);
        // channel-n, frameLen - keep same length with channel-0 data
        AddMultiFrame(channelData, channelId, frameLen);
        INTELL_VOICE_LOG_INFO("readed channel 0 and : %{public}d", channelId);
    } else {
        AddMultiFrame(channelData, channelId, frameLen);
        channelData.reserve(channelData.size() * 2);
        channelData.insert(channelData.end(), channelData.begin(), channelData.end());
    }
    INTELL_VOICE_LOG_INFO("readed buffered length: %{public}d", frameLen);
}

int WakeupSourceProcess::AddMultiFrame(std::vector<uint8_t> &channelData, uint32_t channelId, int frameLen)
{
    std::vector<std::unique_ptr<Uint8ArrayBuffer>> frameList;
    frameList.reserve(frameLen);
    bufferQueue_[channelId]->PopLen(frameLen, frameList);
    if (frameList.empty()) {
        return 0;
    }
    int len = frameList.size() * (frameList[0]->GetSize());
    channelData.reserve(channelData.size() + len);
    for (uint32_t i = 0; i < frameList.size(); i++) {
        channelData.insert(
            channelData.end(), frameList[i]->GetData(), frameList[i]->GetData() + frameList[i]->GetSize());
    }
    return frameList.size();
}

void WakeupSourceProcess::InitDebugFile(uint32_t channelCnt)
{
    for (uint32_t i = 0; i < channelCnt; i++) {
        auto writeObj = std::make_shared<AudioDebug>();
        auto readObj = std::make_shared<AudioDebug>();
        if ((writeObj == nullptr) || (readObj == nullptr)) {
            INTELL_VOICE_LOG_ERROR("failed to create audio debug obj");
            return;
        }

        writeObj->CreateAudioDebugFile(WRITE_SOURCE + std::to_string(i));
        readObj->CreateAudioDebugFile(READ_SOURCE + std::to_string(i));
        writeDebug_.emplace_back(writeObj);
        readDebug_.emplace_back(readObj);
    }
}

void WakeupSourceProcess::WriteDebugData(const std::vector<std::shared_ptr<AudioDebug>> &debugVec,
    const std::vector<uint8_t> &channelData, uint32_t channelId)
{
    if ((channelId >= debugVec.size()) || (debugVec[channelId] == nullptr)) {
        INTELL_VOICE_LOG_ERROR("no debug obj, channel id:%{public}d", channelId);
        return;
    }

    debugVec[channelId]->WriteData(reinterpret_cast<const char *>(channelData.data()), channelData.size());
}

void WakeupSourceProcess::ReleaseDebugFile()
{
    for (auto writeObj : writeDebug_) {
        writeObj->DestroyAudioDebugFile();
    }
    for (auto readObj : readDebug_) {
        readObj->DestroyAudioDebugFile();
    }
    writeDebug_.clear();
    readDebug_.clear();
}
}
}
