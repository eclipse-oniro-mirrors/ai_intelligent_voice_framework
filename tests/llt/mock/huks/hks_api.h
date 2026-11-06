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

#ifndef MOCK_HKS_API_H
#define MOCK_HKS_API_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#define HKS_SUCCESS 0
#define HKS_FAILURE (-1)

// Mock random fills at most this many bytes (typical AES-128 key / digest size)
static constexpr size_t HKS_MOCK_RANDOM_FILL_MAX_BYTES = 16;
// Mock HKS pack info version reported by HksGetPackInfo
static constexpr uint32_t HKS_MOCK_PACK_MAJOR_VERSION = 3;
static constexpr uint32_t HKS_MOCK_PACK_MINOR_VERSION = 0;
static constexpr uint32_t HKS_MOCK_PACK_PATCH_VERSION = 0;

#define HKS_TAG_ALGORITHM 1
#define HKS_TAG_PURPOSE 2
#define HKS_TAG_KEY_SIZE 3
#define HKS_TAG_DIGEST 4
#define HKS_TAG_PADDING 5
#define HKS_TAG_BLOCK_MODE 6
#define HKS_TAG_AUTH_STORAGE_LEVEL 201
#define HKS_TAG_ASSOCIATED_DATA 202
#define HKS_TAG_NONCE 203
#define HKS_TAG_AE_TAG 204

#define HKS_ALG_AES 20

#define HKS_KEY_PURPOSE_ENCRYPT 1
#define HKS_KEY_PURPOSE_DECRYPT 2

#define HKS_AUTH_STORAGE_LEVEL_DE 0
#define HKS_AUTH_STORAGE_LEVEL_SE 1

#define HKS_AES_KEY_SIZE_128 128
#define HKS_AES_KEY_SIZE_256 256

#define HKS_PADDING_NONE 0
#define HKS_PADDING_PKCS7 1

#define HKS_MODE_GCM 32

#define HKS_DIGEST_NONE 0

#define HKS_ERROR_MALLOC_FAIL (-12)
#define HKS_ERROR_INVALID_ARGUMENT (-2)

struct HksBlob {
    uint32_t size;
    uint8_t *data;
};

struct HksParam {
    uint32_t tag;
    union {
        uint32_t uint32Param;
        int32_t int32Param;
        uint64_t uint64Param;
        bool boolParam;
        void *pointerParam;
        HksBlob blob;
    };
};

struct HksKeyInfo {
    HksBlob *alias;
    HksBlob *userId;
};

struct HksPackInfo {
    uint32_t majorVersion;
    uint32_t minorVersion;
    uint32_t patchVersion;
};

struct HksParamSet {
    uint32_t paramsCnt;
    HksParam* params;
};

inline int HksInitialize()
{
    return HKS_SUCCESS;
}

inline int HksGenerateKey(const HksBlob *keyAlias, const HksParamSet *paramSet, void *placeholder)
{
    (void)keyAlias;
    (void)paramSet;
    (void)placeholder;
    return HKS_SUCCESS;
}

inline int HksDeleteKey(HksBlob *keyAlias, const HksParam *params, uint32_t paramCount)
{
    (void)keyAlias;
    (void)params;
    (void)paramCount;
    return HKS_SUCCESS;
}

inline int HksEncrypt(const HksBlob *keyAlias, const HksParam *param, const HksBlob *plainText, HksBlob *cipherText)
{
    (void)keyAlias;
    (void)param;
    (void)plainText;
    (void)cipherText;
    return HKS_SUCCESS;
}

inline int HksDecrypt(const HksBlob *keyAlias, const HksParam *param, const HksBlob *cipherText, HksBlob *plainText)
{
    (void)keyAlias;
    (void)param;
    (void)cipherText;
    (void)plainText;
    return HKS_SUCCESS;
}

inline int HksGetPackInfo(HksPackInfo *packInfo)
{
    if (packInfo != nullptr) {
        packInfo->majorVersion = HKS_MOCK_PACK_MAJOR_VERSION;
        packInfo->minorVersion = HKS_MOCK_PACK_MINOR_VERSION;
        packInfo->patchVersion = HKS_MOCK_PACK_PATCH_VERSION;
    }
    return HKS_SUCCESS;
}

inline int HksRefreshKeyInfo(const HksKeyInfo *keyInfo, const HksParam *params, uint32_t paramCount)
{
    (void)keyInfo;
    (void)params;
    (void)paramCount;
    return HKS_SUCCESS;
}

inline int HksCheckKeyValidity(const HksBlob *keyAlias, const HksParam *params, uint32_t paramCount)
{
    (void)keyAlias;
    (void)params;
    (void)paramCount;
    return HKS_SUCCESS;
}

inline int HksGetKeyInfo(const HksBlob *keyAlias, HksKeyInfo *keyInfo, const HksParam *params, uint32_t paramCount)
{
    (void)keyAlias;
    (void)keyInfo;
    (void)params;
    (void)paramCount;
    return HKS_SUCCESS;
}

inline int HksInit(const HksBlob *keyAlias, const HksParamSet *paramSet, HksBlob *handle, const HksBlob *token)
{
    (void)keyAlias;
    (void)paramSet;
    (void)handle;
    (void)token;
    return HKS_SUCCESS;
}

inline int HksFreeParamSet(HksParamSet *paramSet)
{
    if (paramSet != nullptr) {
        if (paramSet->params != nullptr) {
            delete[] paramSet->params;
            paramSet->params = nullptr;
        }
        delete paramSet;
    }
    return HKS_SUCCESS;
}

inline int HksFreeParamSet(HksParamSet **paramSet)
{
    if (paramSet != nullptr && *paramSet != nullptr) {
        if ((*paramSet)->params != nullptr) {
            delete[] (*paramSet)->params;
            (*paramSet)->params = nullptr;
        }
        delete *paramSet;
        *paramSet = nullptr;
    }
    return HKS_SUCCESS;
}

inline int HksKeyExist(const HksBlob *keyAlias, const HksParamSet *paramSet)
{
    (void)keyAlias;
    (void)paramSet;
    return HKS_SUCCESS;
}

inline int HksInitParamSet(HksParamSet **paramSet)
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

inline int HksAddParams(HksParamSet *paramSet, const HksParam *params, uint32_t paramCount) // NOLINT
{
    if (paramSet == nullptr || params == nullptr) {
        return HKS_FAILURE;
    }
    HksParam *newParams = new HksParam[paramSet->paramsCnt + paramCount];
    for (uint32_t i = 0; i < paramSet->paramsCnt; i++) {
        newParams[i] = paramSet->params[i];
    }
    for (uint32_t i = 0; i < paramCount; i++) {
        newParams[paramSet->paramsCnt + i] = params[i];
    }
    delete[] paramSet->params;
    paramSet->params = newParams;
    paramSet->paramsCnt += paramCount;
    return HKS_SUCCESS;
}

inline int HksBuildParamSet(HksParamSet **paramSet)
{
    (void)paramSet;
    return HKS_SUCCESS;
}

inline int HksUpdate(const HksBlob *handle, const HksParamSet *paramSet, const HksBlob *inData, HksBlob *outData)
{
    (void)handle;
    (void)paramSet;
    (void)inData;
    (void)outData;
    return HKS_SUCCESS;
}

inline int HksFinish(const HksBlob *handle, const HksParamSet *paramSet, const HksBlob *inData, HksBlob *outData)
{
    (void)handle;
    (void)paramSet;
    (void)inData;
    (void)outData;
    return HKS_SUCCESS;
}

inline int HksGenerateRandom(HksParamSet* paramSet, HksBlob* random)
{
    (void)paramSet;
    if (random != nullptr && random->data != nullptr) {
        for (size_t i = 0; i < random->size && i < HKS_MOCK_RANDOM_FILL_MAX_BYTES; i++) {
            random->data[i] = static_cast<uint8_t>(i);
        }
    }
    return HKS_SUCCESS;
}

inline int HksParamSetInit(HksParamSet** paramSet)
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

inline int HksParamSetAddParam(HksParamSet* paramSet, const HksParam* param) // NOLINT
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

#endif
