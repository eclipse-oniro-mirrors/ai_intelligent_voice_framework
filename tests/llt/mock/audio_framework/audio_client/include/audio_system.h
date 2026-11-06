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

#ifndef MOCK_AUDIO_SYSTEM_H
#define MOCK_AUDIO_SYSTEM_H

#include <cstdint>
#include <string>
#include <memory>
#include <vector>
#include <refbase.h>

namespace OHOS {
namespace AudioStandard {

enum class AudioScene {
    AUDIO_SCENE_DEFAULT = 0,
    AUDIO_SCENE_RINGING = 1,
    AUDIO_SCENE_VOICE_COMMUNICATION = 2,
    AUDIO_SCENE_VOICE_ASSISTANT = 3,
};

enum class AudioMode {
    AUDIO_MODE_PLAYBACK = 0,
    AUDIO_MODE_RECORD = 1,
};

enum DeviceType {
    DEVICE_TYPE_NONE = 0,
    DEVICE_TYPE_EARPIECE = 1,
    DEVICE_TYPE_SPEAKER = 2,
    DEVICE_TYPE_WIRED_HEADSET = 3,
    DEVICE_TYPE_WIRED_HEADPHONES = 4,
    DEVICE_TYPE_BLUETOOTH_SCO = 7,
    DEVICE_TYPE_BLUETOOTH_A2DP = 8,
    DEVICE_TYPE_MIC = 10,
};

enum StreamUsage {
    STREAM_USAGE_UNKNOWN = 0,
    STREAM_USAGE_MUSIC = 1,
    STREAM_USAGE_VOICE_COMMUNICATION = 2,
    STREAM_USAGE_VOICE_ASSISTANT = 3,
};

enum class DeviceFlag {
    NONE_FLAG = 0,
    OUTPUT_DEVICES_FLAG = 1,
    INPUT_DEVICES_FLAG = 2,
    ALL_DEVICES_FLAG = 3,
};

enum AudioEncodingType : int32_t {
    ENCODING_PCM = 0,
    ENCODING_PCM_FLOAT = 1,
    ENCODING_PCM_16BIT = 2,
    ENCODING_PCM_8BIT = 3,
};

enum AudioSampleFormat : int32_t {
    SAMPLE_S16LE = 0,
    SAMPLE_S24LE = 1,
    SAMPLE_S32LE = 2,
    SAMPLE_F32LE = 3,
    INVALID_WIDTH = 4,
};

enum AudioChannel : int32_t {
    MONO = 1,
    STEREO = 2,
    CHANNEL_4 = 4,
    CHANNEL_CNT = 4,
};

struct AudioChannelWrapper {
    AudioChannel value = AudioChannel::MONO;
    AudioChannelWrapper() = default;
    AudioChannelWrapper(AudioChannel v) : value(v) {}
    AudioChannelWrapper(uint32_t v) : value(static_cast<AudioChannel>(v)) {}
    operator uint32_t() const { return static_cast<uint32_t>(value); }
    AudioChannelWrapper& operator=(AudioChannel v)
    {
        value = v;
        return *this;
    }
    AudioChannelWrapper& operator=(uint32_t v)
    {
        value = static_cast<AudioChannel>(v);
        return *this;
    }
};

enum SourceType : int32_t {
    SOURCE_TYPE_INVALID = -1,
    SOURCE_TYPE_MIC = 0,
    SOURCE_TYPE_VOICE_CALL = 1,
    SOURCE_TYPE_VOICE_COMMUNICATION = 2,
    SOURCE_TYPE_VOICE_RECOGNITION = 7,
    SOURCE_TYPE_WAKEUP = 6,
};

enum AudioSamplingRate : int32_t {
    SAMPLE_RATE_16000 = 16000,
    SAMPLE_RATE_44100 = 44100,
    SAMPLE_RATE_48000 = 48000,
};

enum class AsrAecMode {
    STANDARD = 0,
    HEADSET = 1,
    BYPASS = 2,
};

enum AsrNoiseSuppressionMode {
    NORMAL_REG = 0,
    TOPMIC_REG = 1,
    HIGH_PERFORMANCE = 2,
    NEAR_FIELD = 3,
    STANDARD = 4,
    BYPASS = 5,
};

class WakeUpSourceCloseCallback {
public:
    virtual ~WakeUpSourceCloseCallback() = default;
    virtual void OnWakeupClose() = 0;
};

struct AudioStreamInfo {
    int32_t encoding = 0;
    int32_t format = 0;
    uint32_t channels = 0;
    int32_t samplingRate = SAMPLE_RATE_16000;
};

struct AudioCapturerInfo {
    int32_t sourceType = 0;
    int32_t capturerFlags = 0;
};

struct AudioDeviceDesc {
    uint32_t deviceType = 0;
    uint32_t deviceId = 0;
    std::string deviceName;
    std::string deviceRole;
    std::string marshal;
    std::string macAddress;
    uint32_t deviceType_ = 0;
    std::string macAddress_;
};

using AudioDeviceDescriptor = AudioDeviceDesc;

class AudioPreferredOutputDeviceChangeCallback {
public:
    virtual ~AudioPreferredOutputDeviceChangeCallback() = default;
    virtual void OnPreferredOutputDeviceUpdated(const std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc) {}
};

struct AudioCapturerChangeInfo {
    AudioDeviceDescriptor deviceDescriptor;
    AudioCapturerInfo capturerInfo;
    std::vector<AudioStreamInfo> streamInfos;
};

struct AudioRendererInfo {
    StreamUsage streamUsage = StreamUsage::STREAM_USAGE_UNKNOWN;
    int32_t streamId = 0;
    DeviceType deviceType = DeviceType::DEVICE_TYPE_NONE;
};

struct AudioCapturerOptions {
    AudioStreamInfo streamInfo;
    AudioCapturerInfo capturerInfo;
};

struct AudioCapturerFilter : public RefBase {
    int32_t sourceType = 0;
    int32_t capturerFlags = 0;
    int32_t uid = 0;
    AudioCapturerInfo capturerInfo;
};

class AudioSystemManager {
public:
    static AudioSystemManager* GetInstance()
    {
        static AudioSystemManager instance;
        return &instance;
    }
    virtual ~AudioSystemManager() = default;

    void IncStrongRef(const void* objectId) { (void)objectId; }
    void DecStrongRef(const void* objectId) { (void)objectId; }
    
    int32_t SetAudioScene(AudioScene scene)
    {
        (void)scene;
        return 0;
    }
    AudioScene GetAudioScene()
    {
        return AudioScene::AUDIO_SCENE_DEFAULT;
    }
    int32_t SetDeviceActive(DeviceType deviceType, DeviceFlag flag)
    {
        (void)deviceType;
        (void)flag;
        return 0;
    }
    bool IsStreamActive(int32_t streamType)
    {
        (void)streamType;
        return true;
    }
    bool IsWhispering()
    {
        return false;
    }
    int32_t SetWakeUpSourceCloseCallback(WakeUpSourceCloseCallback* callback)
    {
        (void)callback;
        return 0;
    }
    int32_t SetWakeUpSourceCloseCallback(std::shared_ptr<WakeUpSourceCloseCallback> callback)
    {
        (void)callback;
        return 0;
    }
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetDevices(DeviceFlag flag)
    {
        (void)flag;
        return {};
    }
    int32_t SelectInputDevice(void* filter, std::vector<std::shared_ptr<AudioDeviceDescriptor>>& devices)
    {
        (void)filter;
        (void)devices;
        return 0;
    }
    int32_t SetAsrAecMode(AsrAecMode mode)
    {
        (void)mode;
        return 0;
    }
    int32_t SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode mode)
    {
        (void)mode;
        return 0;
    }
    std::vector<AudioCapturerChangeInfo> GetCurrentCapturerChangeInfos()
    {
        return {};
    }
};

class AudioStreamManager {
public:
    static AudioStreamManager* GetInstance()
    {
        static AudioStreamManager instance;
        return &instance;
    }
    virtual ~AudioStreamManager() = default;
    int32_t GetCurrentCapturerChangeInfos(
        std::vector<std::shared_ptr<AudioCapturerChangeInfo>>& audioCapturerChangeInfos)
    {
        (void)audioCapturerChangeInfos;
        return 0;
    }
};

} // namespace AudioStandard
} // namespace OHOS

#endif