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

#ifndef PCM_BUFFER_H
#define PCM_BUFFER_H

#include <cstdint>
#include <memory>
#include <string>

namespace OHOS {
namespace IntellVoiceUtils {
class PcmBuffer {
public:
    explicit PcmBuffer(size_t capacity);
    bool Write(const uint8_t* data, size_t len);
    bool SaveToFile(bool needCrypt, const std::string &fileName) const;
    bool Clear();

private:
    PcmBuffer(PcmBuffer &&) = delete;
    PcmBuffer &operator = (PcmBuffer &&) = delete;

    PcmBuffer(const PcmBuffer &) = delete;
    PcmBuffer &operator = (const PcmBuffer &) = delete;

    const size_t mCapacity;
    std::unique_ptr<uint8_t[]> mBuffer;
    size_t mWritePos;
};
}
}
#endif
