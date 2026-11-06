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

/*
 * Stub linux/ashmem.h for x86_64 host build.
 * Provides the constants needed by c_utils ashmem.h without requiring kernel headers.
 */

#ifndef _LINUX_ASHMEM_H
#define _LINUX_ASHMEM_H

#define ASHMEM_NAME_LEN 256
#define ASHMEM_SET_NAME 0x41007701
#define ASHMEM_SET_SIZE 0x41007703
#define ASHMEM_GET_SIZE 0x80047703
#define ASHMEM_SET_PROT_MASK 0x41007705
#define ASHMEM_GET_PROT_MASK 0x80047705

#endif /* _LINUX_ASHMEM_H */
