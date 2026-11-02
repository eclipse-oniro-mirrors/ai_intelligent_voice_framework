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

#include "engine_offline_audio_processor.h"
#include <algorithm>
#include <fstream>
#include <securec.h>
#include "intell_voice_log.h"
#include "engine_host_manager.h"

#define LOG_TAG "EngineOfflineAudio"
static constexpr uint32_t BYTES_PER_FRAME = 320;
static constexpr uint32_t OFFLINE_BYTES_MAX_16K_MONO = 5120;

using namespace OHOS::HDI::IntelligentVoice::Engine::V1_0;
using namespace OHOS::IntellVoiceUtils;
using namespace OHOS::AudioStandard;
using OHOS::IntellVoiceEngine::EngineHostManager;
namespace OHOS {
namespace IntelligentVoice {
namespace Engine {
namespace IntellVoiceEngine {

const std::string OfflineAudioProcessor::CHAIN_NAME_POST_EDIT = "asr_tse_reg";

namespace {
constexpr size_t HEX_DUMP_BYTES_PER_LINE = 16;   // bytes per line in hex dump log
constexpr size_t HEX_DUMP_MAX_PRINT_BYTES = 64;  // max bytes printed in hex dump log
} // namespace

OfflineAudioProcessor::OfflineAudioProcessor()
{
    INTELL_VOICE_LOG_INFO("OfflineAudioProcessor constructor");
}

OfflineAudioProcessor::~OfflineAudioProcessor()
{
    ReleaseResources();
    INTELL_VOICE_LOG_INFO("OfflineAudioProcessor destructor");
}

void OfflineAudioProcessor::ReleaseResources()
{
    if (offlineEffectChain_ != nullptr) {
        offlineEffectChain_->Release();
        INTELL_VOICE_LOG_INFO("Release offlineEffectChain");
        offlineEffectChain_ = nullptr;
    }
    offlineAudioEffectManager_ = nullptr;
}

void OfflineAudioProcessor::PrintAndSaveBuffer(const std::vector<uint8_t>& buffer, const std::string& fileName)
{
    // 1. Print basic buffer information
    INTELL_VOICE_LOG_INFO("Buffer info for %{public}s: size=%{public}zu bytes, capacity=%{public}zu",
                         fileName.c_str(), buffer.size(), buffer.capacity());

    // 2. Print content of first 64 bytes (hexadecimal format)
    if (!buffer.empty()) {
        std::string hexContent = "First bytes: ";
        size_t printCount = std::min(buffer.size(), HEX_DUMP_MAX_PRINT_BYTES);

        for (size_t i = 0; i < printCount; ++i) {
            char hexByte[8];
            if (sprintf_s(hexByte, sizeof(hexByte), "%02X ", buffer[i]) > 0) {
                hexContent += hexByte;
            }

            if ((i + 1) % HEX_DUMP_BYTES_PER_LINE == 0 && i > 0) {
                hexContent += "\n              ";
            }
        }

        INTELL_VOICE_LOG_INFO("%{public}s", hexContent.c_str());
    }

    // 3. Save as PCM file
    std::string pcmFileName = fileName + "dec.pcm";
    std::ofstream outFile(pcmFileName, std::ios::binary);

    if (outFile.is_open()) {
        if (!buffer.empty()) {
            outFile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
        }
        outFile.close();

        INTELL_VOICE_LOG_INFO("Successfully saved PCM file: %{public}s, size=%{public}zu bytes",
                             pcmFileName.c_str(), buffer.size());
    } else {
        INTELL_VOICE_LOG_ERROR("Failed to create PCM file: %{public}s", pcmFileName.c_str());
    }
}

int32_t OfflineAudioProcessor::GetWakeupBackupAudio(const std::string& filePath, std::vector<uint8_t>& buffer)
{
    INTELL_VOICE_LOG_DEBUG("GetWakeupBackupAudio path: %{public}s", filePath.c_str());

    std::vector<std::string> cloneFiles;
    int32_t ret = EngineHostManager::GetInstance().GetWakeupSourceFilesList(cloneFiles);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("GetWakeupSourceFilesList failed, ret: %{public}d", ret);
        return ret;
    }

    std::string allFiles;
    for (const auto& file : cloneFiles) {
        if (!allFiles.empty()) {
            allFiles += ", ";
        }
        allFiles += file;
    }
    INTELL_VOICE_LOG_DEBUG("Clone files: %{public}s", allFiles.c_str());

    return EngineHostManager::GetInstance().GetWakeupSourceFile(filePath, buffer);
}

std::vector<int> OfflineAudioProcessor::parseFrameNumbers(const std::vector<uint8_t>& buffer4)
{
    std::vector<int> frameNumbers;
    int lineCount = 0;

    // Convert vector<uint8_t> to string
    std::string text(buffer4.begin(), buffer4.end());
    INTELL_VOICE_LOG_DEBUG("Parse text: %{public}s", text.c_str());

    lineCount = 0;
    if (!text.empty()) {
        lineCount = 1; // At least one line
        for (char c : text) {
            if (c == '\n') {
                lineCount++;
            }
        }
    }

    // Split text by lines
    std::istringstream iss(text);
    std::string line;

    std::regex frameRegex(R"((?:beginFrameNo|endFrameNo|cmdBeginFrame|cmdEndFrame)\s*:\s*(\d+))");

    while (std::getline(iss, line)) {
        if (line.empty()) {
            continue;
        };

        // Find all frame numbers in each line
        std::sregex_iterator iter(line.begin(), line.end(), frameRegex);
        std::sregex_iterator end;

        // Extract all matched numbers
        for (; iter != end; ++iter) {
            std::smatch match = *iter;
            if (match.size() <= 1) {
                continue;
            }
            try {
                int number = std::stoi(match[1].str());
                frameNumbers.push_back(number);
            } catch (const std::exception& e) {
                INTELL_VOICE_LOG_WARN("Failed to convert frame number: %{public}s", e.what());
                continue;
            }
        }
    }

    INTELL_VOICE_LOG_INFO("Total frame numbers found: %{public}zu, line count: %{public}d",
                          frameNumbers.size(), lineCount);

    return frameNumbers;
}

int32_t OfflineAudioProcessor::CreateOfflineAudioEffectChain()
{
    offlineAudioEffectManager_ = std::make_unique<OfflineAudioEffectManager>();
    if (offlineAudioEffectManager_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to create OfflineAudioEffectManager");
        return -ENOMEM;
    }

    std::vector<std::string> effectChains = offlineAudioEffectManager_->GetOfflineAudioEffectChains();
    auto iter = std::find_if(effectChains.begin(), effectChains.end(),
                             [](const std::string &s) { return s.find(CHAIN_NAME_POST_EDIT) != std::string::npos; });
    if (iter != effectChains.end()) {
        selectedChain_ = *iter;
    } else {
        INTELL_VOICE_LOG_ERROR("Cannot find effect chain: %{public}s", CHAIN_NAME_POST_EDIT.c_str());
        return -ENODEV;
    }

    offlineEffectChain_ = offlineAudioEffectManager_->CreateOfflineAudioEffectChain(selectedChain_);
    if (offlineEffectChain_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Failed to create OfflineAudioEffectChain for chain: %{public}s",
            selectedChain_.c_str());
        return -ENODEV;
    }

    INTELL_VOICE_LOG_INFO("Successfully created OfflineAudioEffectChain of chain name: %{public}s",
        selectedChain_.c_str());
    return 0;
}

int32_t OfflineAudioProcessor::ProcessDataWithOfflineEffect(
    uint8_t* inputData,
    size_t inputDataSize,
    uint8_t* outputBuffer,
    size_t outputBufferSize,
    size_t maxProcessSize)
{
    if (inputData == nullptr || outputBuffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("Input or Output buffer is null!");
        return -EINVAL;
    }

    if (inputDataSize <= 0) {
        INTELL_VOICE_LOG_INFO("Input data size is not positive!");
        return 0;
    }

    if (offlineEffectChain_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("Offline effect chain is not initialized!");
        return -ENODEV;
    }

    // Initialize processing parameters
    size_t remainingSize = inputDataSize;
    size_t inOffset = 0;
    int sendCount = 0;
    int32_t ret_status = 0;

    INTELL_VOICE_LOG_INFO("Starting to process data. Total size: %{public}zu bytes", inputDataSize);

    // Loop processing: until all data is processed
    while (remainingSize > 0) {
        // Calculate the size to process in this iteration
        size_t currentProcessSize = (remainingSize > maxProcessSize) ? maxProcessSize : remainingSize;
        INTELL_VOICE_LOG_DEBUG("Processing chunk: offset=%{public}zu, size=%{public}zu, remaining=%{public}zu",
                              inOffset, currentProcessSize, remainingSize);
        // Call Process interface
        ret_status = offlineEffectChain_->Process(
            inputData + inOffset,       // Starting address of input data for this processing
            currentProcessSize,         // Number of bytes to process
            outputBuffer,               // Output buffer
            outputBufferSize            // Output buffer size
        );
        sendCount++;

        if (ret_status != 0) {
            INTELL_VOICE_LOG_ERROR(
            "Process failed : %{public}d, offset: %{public}zu, remain: %{public}zu, sendCnt: %{public}d",
                ret_status, inOffset, remainingSize, sendCount);
            break; // Processing failed, exit loop
        }

        // Processing failed, exit loop
        remainingSize -= currentProcessSize;
        inOffset += currentProcessSize;
    }

    // Log processing results
    if (ret_status == 0) {
        INTELL_VOICE_LOG_INFO("Success processed data. Total processed: %{public}zu bytes, sendCnt: %{public}d",
                             inputDataSize - remainingSize, sendCount);
    } else {
        INTELL_VOICE_LOG_ERROR("Processing errors. Success processed: %{public}zu bytes, sendCnt: %{public}d",
                             inputDataSize - remainingSize, sendCount);
    }

    return ret_status;
}

void OfflineAudioProcessor::LogFrameNumbers(const std::vector<int>& frameNumbers)
{
    std::ostringstream oss;
    oss << "frameNumbers: ";
    for (size_t i = 0; i < frameNumbers.size(); ++i) {
        oss << frameNumbers[i];
        if (i < frameNumbers.size() - 1) {
            oss << ", ";
        }
    }
    INTELL_VOICE_LOG_INFO("%{public}s", oss.str().c_str());
}

int OfflineAudioProcessor::SetOfflineEffectChainParams(const std::vector<int>& frameNumbers)
{
    if (offlineEffectChain_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("offlineEffectChain_ is null");
        return -1;
    }

    // Build parameter string
    std::string paramStr = "FRAME_NUMBERS:";
    for (size_t i = 0; i < frameNumbers.size(); ++i) {
        paramStr += std::to_string(frameNumbers[i]);
        if (i < frameNumbers.size() - 1) {
            paramStr += ",";
        }
    }

    std::vector<uint8_t> param(paramStr.begin(), paramStr.end());

    // Set parameters
    int ret = offlineEffectChain_->SetParam(param);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("SetParam failed, ret: %{public}d", ret);
        return ret;
    }

    INTELL_VOICE_LOG_INFO("Successfully set offline effect chain parameters");
    return 0;
}

int32_t OfflineAudioProcessor::ConfigureAudioStreamAndEffectChain()
{
    if (offlineEffectChain_ == nullptr) {
        INTELL_VOICE_LOG_ERROR("offlineEffectChain_ is null");
        return -1;
    }
    // 1. Configure audio stream information
    AudioStreamInfo inputStreamInfo;
    AudioStreamInfo outputStreamInfo = inputStreamInfo;
    inputStreamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    outputStreamInfo.encoding = AudioEncodingType::ENCODING_PCM;
    inputStreamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    outputStreamInfo.format = AudioSampleFormat::SAMPLE_S16LE;
    inputStreamInfo.channels = AudioChannel::MONO;
    outputStreamInfo.channels = AudioChannel::MONO;
    inputStreamInfo.samplingRate = static_cast<AudioSamplingRate>(AudioSamplingRate::SAMPLE_RATE_16000);
    outputStreamInfo.samplingRate = static_cast<AudioSamplingRate>(AudioSamplingRate::SAMPLE_RATE_16000);
    outputStreamInfo_ = outputStreamInfo;

    // 2. Configure effect chain
    int32_t ret = offlineEffectChain_->Configure(inputStreamInfo, outputStreamInfo);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to configure offline effect chain, ret: %{public}d", ret);
        return ret;
    }

    // 3. Prepare effect chain
    ret = offlineEffectChain_->Prepare();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to prepare offline effect chain, ret: %{public}d", ret);
        return ret;
    }

    // 4. Get buffer size
    uint32_t effectInputBufferSize = 0;
    uint32_t effectOutputBufferSize = 0;
    ret = offlineEffectChain_->GetEffectBufferSize(effectInputBufferSize, effectOutputBufferSize);
    INTELL_VOICE_LOG_INFO("GetEffectBufferSize result: %{public}d, inputSize: %{public}u, outputSize: %{public}u",
                        ret, effectInputBufferSize, effectOutputBufferSize);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to get effect buffer size, ret: %{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t OfflineAudioProcessor::ProcessRegistrationCorpusData(std::vector<std::vector<uint8_t>>& buffers,
    const std::vector<int>& frameNumbers)
{
    if (buffers.size() < 4) { // Total 4 files
        INTELL_VOICE_LOG_ERROR("Insufficient buffers provided, need 3 buffers but got %{public}zu", buffers.size());
        return -1;
    }

    uint8_t outBuffer[BYTES_PER_FRAME] = {};
    int32_t totalRetStatus = 0;
    if (frameNumbers.size() < 12) { // 3 sentences * (2 begins + 2 ends) = 12 groups
        INTELL_VOICE_LOG_ERROR("Insufficient frame numbers, need at least 12 but got %{public}zu", frameNumbers.size());
        return -1;
    }

    totalRetStatus += ProcessDataWithOfflineEffect(// First sentence - use data from buffer1 (buffers[0])
        buffers[0].data() + frameNumbers[0] * BYTES_PER_FRAME, // 0 is first wake word begin   buffers[0]
        (frameNumbers[1] - frameNumbers[0] + 1) * BYTES_PER_FRAME, // 1 is first wake word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    totalRetStatus += ProcessDataWithOfflineEffect(
        buffers[0].data() + frameNumbers[2] * BYTES_PER_FRAME, // 2 is first command word begin   buffers[0]
        (frameNumbers[3] - frameNumbers[2] + 1) * BYTES_PER_FRAME, // 3 is first command word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    totalRetStatus += ProcessDataWithOfflineEffect(// Second sentence - use data from buffer2 (buffers[1])
        buffers[1].data() + frameNumbers[4] * BYTES_PER_FRAME, // 4 is second wake word begin   buffers[1]
        (frameNumbers[5] - frameNumbers[4] + 1) * BYTES_PER_FRAME, // 5 is second wake word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    totalRetStatus += ProcessDataWithOfflineEffect(
        buffers[1].data() + frameNumbers[6] * BYTES_PER_FRAME, // 6 is second command word begin   buffers[1]
        (frameNumbers[7] - frameNumbers[6] + 1) * BYTES_PER_FRAME, // 7 is second command word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    totalRetStatus += ProcessDataWithOfflineEffect(// Third sentence - use data from buffer3 (buffers[2])
        buffers[2].data() + frameNumbers[8] * BYTES_PER_FRAME, // 8 is third wake word begin   buffers[2]
        (frameNumbers[9] - frameNumbers[8] + 1) * BYTES_PER_FRAME, // 9 is third wake word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    totalRetStatus += ProcessDataWithOfflineEffect(
        buffers[2].data() + frameNumbers[10] * BYTES_PER_FRAME, // 10 is third command word begin buffers[2]
        (frameNumbers[11] - frameNumbers[10] + 1) * BYTES_PER_FRAME,  // 11 is third command word end
        outBuffer, BYTES_PER_FRAME, OFFLINE_BYTES_MAX_16K_MONO);
    if (totalRetStatus != 0) {
        INTELL_VOICE_LOG_ERROR("Data processing failed with error: %{public}d", totalRetStatus);
    } else {
        INTELL_VOICE_LOG_INFO("Data processing completed successfully");
    }

    return totalRetStatus;
}

int32_t OfflineAudioProcessor::audio_offline_init(const std::vector<int>& frameNumbers)
{
    int32_t ret = CreateOfflineAudioEffectChain(); // 3. Create offline audio effect chain
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to create offline audio effect chain, ret: %{public}d", ret);
        return ret;
    }
    ret = OfflineAudioProcessor::ConfigureAudioStreamAndEffectChain();
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to ConfigureAudioStreamAndEffectChain, ret: %{public}d", ret);
        return ret;
    }
    ret = SetOfflineEffectChainParams(frameNumbers); // 4. Set parameters
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to SetOfflineEffectChainParams, ret: %{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t OfflineAudioProcessor::ProcessOfflineAudio(const std::string& filePath1,
                                                  const std::string& filePath2,
                                                  const std::string& filePath3,
                                                  const std::string& filePath4)
{
    INTELL_VOICE_LOG_INFO("enter ProcessOfflineAudio");
    // 1. Read 4 files into buffers
    std::vector<std::vector<uint8_t>> buffers(4); // 4 buffers
    std::vector<std::string> filePaths = {filePath1, filePath2, filePath3, filePath4};
    int32_t ret = 0;
    for (size_t i = 0; i < filePaths.size(); ++i) {
        ret = GetWakeupBackupAudio(filePaths[i], buffers[i]);
        if (ret != 0) {
            INTELL_VOICE_LOG_ERROR("Failed to read file%{public}d: %{public}s, ret: %{public}d",
                static_cast<int>(i + 1), filePaths[i].c_str(), ret);
            return ret;
        }
    }
    INTELL_VOICE_LOG_INFO("Successfully read all files");

    // 2. Parse frame number information
    std::vector<int> frameNumbers = parseFrameNumbers(buffers[3]);
    if (frameNumbers.size() < 12) { // 6 sentences * (begin + end) = 12 pairs
        INTELL_VOICE_LOG_ERROR("Less than three pieces of corpus, or the corpus is not a prompt");
        return -1;
    }

    LogFrameNumbers(frameNumbers);
    ret = audio_offline_init(frameNumbers);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to audio_offline_init, ret: %{public}d", ret);
        return ret;
    }
    ret = ProcessRegistrationCorpusData(buffers, frameNumbers);
    // 5. Release resources
    if (offlineEffectChain_ != nullptr) {
        offlineEffectChain_->Release();
        INTELL_VOICE_LOG_INFO("Release offlineEffectChain");
        offlineEffectChain_ = nullptr;
    }
    offlineAudioEffectManager_ = nullptr;
    INTELL_VOICE_LOG_INFO("ProcessOfflineAudio exit, ret_status: %{public}d", ret);
    return ret;
}

int32_t OfflineAudioProcessor::ProcessOfflineAudioWhile(const std::string& filePath1,
    const std::string& filePath2, const std::string& filePath3, const std::string& filePath4)
{
    INTELL_VOICE_LOG_INFO("enter ProcessOfflineAudioWhile");
    std::vector<std::vector<uint8_t>> buffers(4); // 4 buffers
    std::vector<std::string> filePaths = {filePath1, filePath2, filePath3, filePath4};
    int32_t ret = -2;
    int32_t tryCount = 0;
    while (ret != 0) {
        ret = 0;
        for (size_t i = 0; i < filePaths.size(); ++i) {
            ret += GetWakeupBackupAudio(filePaths[i], buffers[i]);
            if (ret != 0) {
                INTELL_VOICE_LOG_ERROR("Failed to read file%{public}d: %{public}s, ret: %{public}d",
                    static_cast<int>(i + 1), filePaths[i].c_str(), ret);
            }
        }
        if (ret == 0) {
            break;
        }
        if (++tryCount > 10) { // Poll up to 10 times
            INTELL_VOICE_LOG_ERROR("Failed to read file: retry %{public}d, ret: %{public}d", tryCount, ret);
            return ret;
        }
        sleep(1); // Wait 1 s
    }

    INTELL_VOICE_LOG_INFO("Successfully read all files, tryCnt %{public}d", tryCount);

    std::vector<int> frameNumbers = parseFrameNumbers(buffers[3]);
    if (frameNumbers.size() < 12) { // 6 sentences * (begin + end) = 12 pairs
        INTELL_VOICE_LOG_ERROR("Less than three pieces of corpus, or the corpus is not a prompt");
        return -1;
    }

    LogFrameNumbers(frameNumbers);
    ret = audio_offline_init(frameNumbers);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to audio_offline_init, ret: %{public}d", ret);
        return ret;
    }
    ret = ProcessRegistrationCorpusData(buffers, frameNumbers);
    if (ret != 0) {
        INTELL_VOICE_LOG_ERROR("Failed to ProcessRegistrationCorpusData, ret: %{public}d", ret);
        return ret;
    }
    if (offlineEffectChain_ != nullptr) { // 5. Release resources
        offlineEffectChain_->Release();
        INTELL_VOICE_LOG_INFO("Release offlineEffectChain");
        offlineEffectChain_ = nullptr;
    }
    offlineAudioEffectManager_ = nullptr;
    INTELL_VOICE_LOG_INFO("ProcessOfflineAudio exit, ret_status: %{public}d", ret);
    return ret;
}

} // IntellVoiceEngine
} // namespace Engine
} // namespace IntelligentVoice
} // namespace OHOS
