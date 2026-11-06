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

#ifndef MOCK_ACCESSTOKEN_KIT_H
#define MOCK_ACCESSTOKEN_KIT_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace OHOS {
namespace Security {
namespace AccessToken {

using AccessTokenID = uint32_t;

enum TokenTypeFlag : int32_t {
    TOKEN_NATIVE = 0,
    TOKEN_SHELL = 1,
    TOKEN_HAP = 2,
};

enum PermissionState {
    PERMISSION_DENIED = -1,
    PERMISSION_GRANTED = 0,
};

enum {
    ERR_PERMISSION_ALREADY_START_USING = 0,
};

struct PermStateChangeInfo {
    int32_t permStateChangeType;
    std::string permissionName;
    AccessTokenID tokenId;
};

struct PermStateChangeScope {
    AccessTokenID tokenId = 0;
    int32_t scopeSize = 0;
    std::vector<std::string> permList;
};

class PermStateChangeCallbackCustomize {
public:
    explicit PermStateChangeCallbackCustomize(const PermStateChangeScope &scopeInfo) {}
    virtual ~PermStateChangeCallbackCustomize() {}
    virtual void PermStateChangeCallback(PermStateChangeInfo& result) {}
};

class AccessTokenKit {
public:
    static int VerifyAccessToken(AccessTokenID tokenId, const std::string& permissionName)
    {
        (void)tokenId;
        (void)permissionName;
        return static_cast<int>(PermissionState::PERMISSION_GRANTED);
    }

    static TokenTypeFlag GetTokenTypeFlag(AccessTokenID tokenId)
    {
        (void)tokenId;
        return TokenTypeFlag::TOKEN_NATIVE;
    }

    static int RegisterPermStateChangeCallback(std::shared_ptr<PermStateChangeCallbackCustomize> callbackPtr)
    {
        (void)callbackPtr;
        return 0;
    }
};

class TokenIdKit {
public:
    static bool IsSystemAppByFullTokenID(uint64_t fullTokenId)
    {
        (void)fullTokenId;
        return true;
    }
};

class PrivacyKit {
public:
    static int StartUsingPermission(AccessTokenID tokenId, const std::string& permissionName)
    {
        (void)tokenId;
        (void)permissionName;
        return 0;
    }

    static int StopUsingPermission(AccessTokenID tokenId, const std::string& permissionName)
    {
        (void)tokenId;
        (void)permissionName;
        return 0;
    }

    static int AddPermissionUsedRecord(AccessTokenID tokenId, const std::string& permissionName,
        int32_t count, int32_t duration)
    {
        (void)tokenId;
        (void)permissionName;
        (void)count;
        (void)duration;
        return 0;
    }
};

} // namespace AccessToken
} // namespace Security
} // namespace OHOS

#endif