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

#ifndef LLT_CONSTANTS_H
#define LLT_CONSTANTS_H

#include <cstdint>

namespace OHOS {
namespace IntellVoice {
namespace LltConstants {

// Audio sample parameters used by wakeup/enroll engine Attach
constexpr int32_t DEFAULT_MIN_BUF_SIZE = 640;          // bytes, 16kHz*16bit*1ch*20ms
constexpr int32_t DEFAULT_SAMPLE_CHANNELS = 1;          // mono
constexpr int32_t DEFAULT_BITS_PER_SAMPLE = 16;         // 16-bit PCM
constexpr int32_t DEFAULT_SAMPLE_RATE = 16000;          // 16 kHz

// Capturer / buffer sizes
constexpr int32_t DEFAULT_AUDIO_BUFFER_SIZE = 640;      // bytes, one 20ms frame
constexpr int32_t LARGE_AUDIO_BUFFER_SIZE = 4096;       // bytes, large file transfer scenario
constexpr int32_t DEFAULT_CAPTURER_BUFFER_SIZE = 4096;  // bytes, audio capturer buffer
constexpr int32_t DEFAULT_EFFECT_BUFFER_SIZE = 4096;    // bytes, offline audio effect buffer

// Capturer source id (start capturer flag)
constexpr int32_t DEFAULT_CAPTURER_SOURCE_ID = 1;

// Max upload file count passed to GetUploadFiles
constexpr int32_t MAX_UPLOAD_FILE_COUNT = 10;

// Asynchronous HDI callback wait durations (ms)
constexpr int WAIT_INIT_DONE_MS = 200;
constexpr int WAIT_RECOGNIZE_COMPLETE_MS = 400;
constexpr int WAIT_AUDIO_SWITCH_MS = 200;
constexpr int WAIT_HDI_ASYNC_CB_MS = 50;

// Generic error codes returned by engine/service in mock environment
constexpr int32_t ERR_CODE_SUCCESS = 0;
constexpr int32_t ERR_CODE_GENERIC_FAILURE = -1;
constexpr int32_t ERR_CODE_WAKEUP_PROCESSING = -2;  // mirrors WakeupEngineImplFacade::WAKEUP_PROCESSING

// EventDetail.version used in wakeup notifications
constexpr int32_t EVENT_VERSION_DEFAULT = 0;
constexpr int32_t EVENT_VERSION_HFP_50 = 50;
constexpr int32_t EVENT_VERSION_HFP_100 = 100;

// HeadsetWakeupWrapper::ReadHeadsetStream return value (hasAwakeWord=false, data present)
constexpr int32_t HEADSET_STREAM_DATA_READY = 1;

// HDI mock reported version
constexpr uint32_t HDI_MOCK_MAJOR_VERSION = 1;
constexpr uint32_t HDI_MOCK_MINOR_VERSION = 3;

}  // namespace LltConstants
}  // namespace IntellVoice
}  // namespace OHOS

#endif  // LLT_CONSTANTS_H
