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

#ifndef OFFLINE_AUDIO_PROCESSOR_H
#define OFFLINE_AUDIO_PROCESSOR_H

#include <memory>
#include <string>
#include <vector>
#include <regex>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "audio_info.h"
#include "audio_source.h"
#include "intell_voice_generic_factory.h"
#include "engine_base.h"
#include "engine_util.h"
#include "enroll_adapter_listener.h"
#include "task_executor.h"
#include "offline_audio_effect_manager.h"

namespace OHOS {
namespace IntelligentVoice {
namespace Engine {
namespace IntellVoiceEngine {
class OfflineAudioProcessor {
public:
    OfflineAudioProcessor();
    ~OfflineAudioProcessor();

    /**
     * @brief Process offline audio data
     * @param filePath1 First audio file path
     * @param filePath2 Second audio file path
     * @param filePath3 Third audio file path
     * @param filePath4 Text file path
     * @return 0 for success, other values for failure
     */
    int32_t ProcessOfflineAudio(const std::string& filePath1,
                               const std::string& filePath2,
                               const std::string& filePath3,
                               const std::string& filePath4);
    /**
     * @brief Process offline audio data (alternative method)
     * @param filePath1 First audio file path
     * @param filePath2 Second audio file path
     * @param filePath3 Third audio file path
     * @param filePath4 Text file path
     * @return 0 for success, other values for failure
     */
    int32_t ProcessOfflineAudioWhile(const std::string& filePath1,
                               const std::string& filePath2,
                               const std::string& filePath3,
                               const std::string& filePath4);

private:
    /**
     * @brief Print and save buffer data
     * @param buffer Data buffer
     * @param fileName File name
     */
    void PrintAndSaveBuffer(const std::vector<uint8_t>& buffer, const std::string& fileName);

    /**
     * @brief Get wakeup backup audio
     * @param filePath File path
     * @param buffer Output buffer
     * @return 0 for success, other values for failure
     */
    int32_t GetWakeupBackupAudio(const std::string& filePath, std::vector<uint8_t>& buffer);

    /**
     * @brief Parse frame numbers
     * @param buffer4 Text data buffer
     * @return Vector of frame numbers
     */
    std::vector<int> parseFrameNumbers(const std::vector<uint8_t>& buffer4);

    /**
     * @brief Create offline audio effect chain
     * @return 0 for success, other values for failure
     */
    int32_t CreateOfflineAudioEffectChain();

    /**
     * @brief Process data with offline effect
     * @param inputData Input data address
     * @param inputDataSize Input data size
     * @param outputBuffer Output buffer address
     * @param outputBufferSize Output buffer size
     * @param maxProcessSize Maximum processing unit size
     * @return 0 for success, other values for failure
     */
    int32_t ProcessDataWithOfflineEffect(
        uint8_t* inputData,
        size_t inputDataSize,
        uint8_t* outputBuffer,
        size_t outputBufferSize,
        size_t maxProcessSize = 5120);

    /**
     * @brief Release resources
     */
    void ReleaseResources();
    void LogFrameNumbers(const std::vector<int>& frameNumbers);
    int32_t ConfigureAudioStreamAndEffectChain();
    int SetOfflineEffectChainParams(const std::vector<int>& frameNumbers);
    int32_t ProcessRegistrationCorpusData(std::vector<std::vector<uint8_t>>& buffers,
        const std::vector<int>& frameNumbers);
    int32_t audio_offline_init(const std::vector<int>& frameNumbers);
    static const std::string CHAIN_NAME_POST_EDIT;
    std::unique_ptr<AudioStandard::OfflineAudioEffectManager> offlineAudioEffectManager_;
    std::unique_ptr<AudioStandard::OfflineAudioEffectChain> offlineEffectChain_;
    std::string selectedChain_;
    OHOS::AudioStandard::AudioCapturerOptions inputOptions_;
    OHOS::AudioStandard::AudioStreamInfo outputStreamInfo_;
};
}
} // namespace Engine
} // namespace IntelligentVoice
} // namespace OHOS

#endif // OFFLINE_AUDIO_PROCESSOR_H
