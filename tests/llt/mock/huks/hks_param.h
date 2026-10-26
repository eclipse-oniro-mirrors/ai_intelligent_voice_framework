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

#ifndef HKS_PARAM_H
#define HKS_PARAM_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace OHOS {
namespace Security {
namespace Huks {

struct HksBlob {
    uint32_t size;
    uint8_t* data;
};

struct HksParam {
    uint32_t tag;
    union {
        int32_t int32Value;
        uint32_t uint32Value;
        int64_t int64Value;
        uint64_t uint64Value;
        const char* stringValue;
        HksBlob blobValue;
    };
};

struct HksParamSet {
    uint32_t paramsCnt;
    HksParam* params;
};

enum HksTagType : uint32_t {
    HKS_TAG_TYPE_INVALID = 0,
    HKS_TAG_TYPE_INT32 = 1,
    HKS_TAG_TYPE_UINT32 = 2,
    HKS_TAG_TYPE_INT64 = 3,
    HKS_TAG_TYPE_UINT64 = 4,
    HKS_TAG_TYPE_BYTES = 5,
    HKS_TAG_TYPE_STRING = 6,
};

enum HksTag : uint32_t {
    HKS_TAG_ALGORITHM = 1,
    HKS_TAG_PURPOSE = 2,
    HKS_TAG_KEY_SIZE = 3,
    HKS_TAG_DIGEST = 4,
    HKS_TAG_PADDING = 5,
    HKS_TAG_BLOCK_MODE = 6,
};

enum HksResult : int32_t {
    HKS_SUCCESS = 0,
    HKS_FAILURE = -1,
};

enum HksCipherMode : uint32_t {
    HKS_MODE_ENCRYPT = 1,
    HKS_MODE_DECRYPT = 2,
    HKS_MODE_GCM = 32,
};

enum HksAlgType : uint32_t {
    HKS_ALG_AES = 20,
};

enum HksKeyPurpose : uint32_t {
    HKS_KEY_PURPOSE_ENCRYPT = 1,
    HKS_KEY_PURPOSE_DECRYPT = 2,
};

enum HksAuthStorageLevel : uint32_t {
    HKS_AUTH_STORAGE_LEVEL_DE = 0,
    HKS_AUTH_STORAGE_LEVEL_SE = 1,
};

enum HksKeyDigest : uint32_t {
    HKS_DIGEST_NONE = 0,
};

enum HksKeyPadding : uint32_t {
    HKS_PADDING_NONE = 0,
    HKS_PADDING_PKCS7 = 1,
};

enum HksKeySize : uint32_t {
    HKS_AES_KEY_SIZE_128 = 128,
    HKS_AES_KEY_SIZE_256 = 256,
};

inline int32_t HksEncrypt(const HksParamSet* paramSet, const HksBlob* key, const HksBlob* inData, HksBlob* outData)
{
    (void)paramSet;
    (void)key;
    (void)inData;
    (void)outData;
    return HKS_SUCCESS;
}

inline int32_t HksDecrypt(const HksParamSet* paramSet, const HksBlob* key, const HksBlob* inData, HksBlob* outData)
{
    (void)paramSet;
    (void)key;
    (void)inData;
    (void)outData;
    return HKS_SUCCESS;
}

inline int32_t HksInit(uint32_t keyAlias, const HksParamSet* paramSet, const HksBlob* verifToken)
{
    (void)keyAlias;
    (void)paramSet;
    (void)verifToken;
    return HKS_SUCCESS;
}

inline int32_t HksGenerateRandom(HksParamSet* paramSet, HksBlob* random)
{
    (void)paramSet;
    if (random != nullptr && random->data != nullptr) {
        for (size_t i = 0; i < random->size && i < 16; i++) {
            random->data[i] = static_cast<uint8_t>(i);
        }
    }
    return HKS_SUCCESS;
}

inline int32_t HksParamSetInit(HksParamSet** paramSet)
{
    if (paramSet == nullptr) {
        return HKS_FAILURE;
    }
    *paramSet = new HksParamSet();
    if (*paramSet == nullptr) {
        return HKS_FAILURE;
    }
    (*paramSet)->paramsCnt = 0;
    (*paramSet)->params = nullptr;
    return HKS_SUCCESS;
}

inline int32_t HksParamSetAddParam(HksParamSet* paramSet, const HksParam* param) // NOLINT
{
    if (paramSet == nullptr || param == nullptr) {
        return HKS_FAILURE;
    }
    HksParam* newParams = new HksParam[paramSet->paramsCnt + 1];
    for (uint32_t i = 0; i < paramSet->paramsCnt; i++) {
        newParams[i] = paramSet->params[i];
    }
    newParams[paramSet->paramsCnt] = *param;
    delete[] paramSet->params;
    paramSet->params = newParams;
    paramSet->paramsCnt++;
    return HKS_SUCCESS;
}

inline void HksParamSetFree(HksParamSet* paramSet, bool deleteEntry)
{
    if (paramSet != nullptr) {
        if (paramSet->params != nullptr) {
            delete[] paramSet->params;
            paramSet->params = nullptr;
        }
        paramSet->paramsCnt = 0;
        if (deleteEntry) {
            delete paramSet;
        }
    }
}

} // namespace Huks
} // namespace Security
} // namespace OHOS

using OHOS::Security::Huks::HKS_SUCCESS;

#endif
