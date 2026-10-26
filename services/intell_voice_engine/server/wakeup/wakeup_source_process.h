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
#ifndef WAKEUP_SOURCE_PROCESS_H
#define WAKEUP_SOURCE_PROCESS_H

#include <memory>
#include "queue_util.h"
#include "audio_debug.h"

namespace OHOS {
namespace IntellVoiceEngine {
using OHOS::IntellVoiceUtils::Uint8ArrayBufferQueue;
constexpr uint32_t CHANNEL_ID_0 = 0;
constexpr uint32_t CHANNEL_ID_1 = 1;
constexpr uint32_t CHANNEL_ID_2 = 2;
constexpr uint32_t CHANNEL_ID_3 = 3;
constexpr uint32_t CHANNEL_CNT_FOUR = 4;

class WakeupSourceProcess {
public:
    WakeupSourceProcess() = default;
    ~WakeupSourceProcess();
    void Init(uint32_t channelCnt);
    void Write(const std::vector<std::vector<uint8_t>> &audioData);
    int32_t Read(std::vector<uint8_t> &data, int32_t readChannel);
    void SetReadBufferFlag(bool readBuffer)
    {
        readBufferPCM = readBuffer;
    }
    void Release();

private:
    void WriteChannelData(const std::vector<uint8_t> &channelData, uint32_t channelId);
    bool ReadChannelData(std::vector<uint8_t> &channelData, uint32_t channelId);
    void ReadBufferedData(std::vector<uint8_t> &channelData, uint32_t channelId);
    void InitDebugFile(uint32_t channelCnt);
    void WriteDebugData(const std::vector<std::shared_ptr<AudioDebug>> &debugVec,
        const std::vector<uint8_t> &channelData, uint32_t channelId);
    void ReleaseDebugFile();
    int AddMultiFrame(std::vector<uint8_t> &channelData, uint32_t channelId, int frameLen);
    bool readBufferPCM = false;
    uint32_t channelCnt_ = 0;
    std::vector<std::unique_ptr<Uint8ArrayBufferQueue>> bufferQueue_;
    std::vector<std::shared_ptr<AudioDebug>> writeDebug_;
    std::vector<std::shared_ptr<AudioDebug>> readDebug_;
};
}
}
#endif