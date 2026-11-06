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

#ifndef MOCK_IPC_SKELETON_H
#define MOCK_IPC_SKELETON_H

#include <cstdint>
#include <string>

namespace OHOS {
class IPCSkeleton {
public:
    static pid_t GetCallingUid();
    static pid_t GetCallingPid();
    static uint32_t GetCallingTokenID();
    static uint64_t GetCallingFullTokenID();
    static std::string GetCallingDeviceID();
    static int64_t GetCallingTime();
    static bool IsLocalCalling();
    static void FlushCommands();
    static void JoinWorkThread();
};
} // namespace OHOS

#endif // MOCK_IPC_SKELETON_H
