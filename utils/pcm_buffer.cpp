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

#include "pcm_buffer.h"
#include <fstream>
#include <algorithm>
#include <securec.h>
#include "huks_aes_adapter.h"
#include "array_buffer_util.h"
#include "intell_voice_log.h"

#undef LOG_TAG
#define LOG_TAG "PcmBuffer"

namespace OHOS {
namespace IntellVoiceUtils {
PcmBuffer::PcmBuffer(size_t capacity)
    : mCapacity(capacity),
      mBuffer(std::make_unique<uint8_t[]>(capacity)),
      mWritePos(0)
{
    std::fill(mBuffer.get(), mBuffer.get() + mCapacity, 0);
}

bool PcmBuffer::Write(const uint8_t* data, size_t len) {
    if (data == nullptr || len <= 0 || mWritePos + len > mCapacity) {
        INTELL_VOICE_LOG_ERROR("param is invalid, writePos:%zu, len:%zu, capacity:%zu", 
            mWritePos, len, mCapacity);
        return false;
    }
    
    (void)memcpy_s(mBuffer.get() + mWritePos, mCapacity - mWritePos, data, len);
    mWritePos += len;
    
    return true;
}

bool PcmBuffer::SaveToFile(bool needCrypt, const std::string &fileName) const {
    if (mWritePos == 0 || mBuffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is empty or no data written");
        return false;
    }

    std::ofstream outFile(fileName, std::ios::binary);
    if (!outFile.is_open()) {
        INTELL_VOICE_LOG_ERROR("file open failed");
        return false;
    }

    if (needCrypt) {
        auto inData = CreateArrayBuffer<uint8_t>(mWritePos, mBuffer.get());
        int32_t ret = HKS_SUCCESS;
        std::unique_ptr<Uint8ArrayBuffer> outData = nullptr;
        ret = HuksAesAdapter::Encrypt(inData, outData);
        if (ret != HKS_SUCCESS) {
            INTELL_VOICE_LOG_ERROR("encrypt failed, ret:%{public}d", ret);
            return false;
        }
        outFile.write(reinterpret_cast<const char*>(outData->GetData()), outData->GetSize());
    } else {
        outFile.write(reinterpret_cast<const char*>(mBuffer.get()), mWritePos);
    }

    if (!outFile) {
        INTELL_VOICE_LOG_ERROR("file write failed");
        return false;
    }
    outFile.close();
    return true;
}

bool PcmBuffer::Clear() {
    mWritePos = 0;
    std::fill(mBuffer.get(), mBuffer.get() + mCapacity, 0);
    return true;
}
}
}