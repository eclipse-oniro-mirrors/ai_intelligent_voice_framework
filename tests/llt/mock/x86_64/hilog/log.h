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
 * Stub hilog/log.h for x86_64 host build.
 * Provides macro definitions that route to HiLogPrintDictNew (hilog_stub.cpp).
 */

#ifndef HILOG_INCLUDE_HILOG_LOG_H
#define HILOG_INCLUDE_HILOG_LOG_H

#include <cstdarg>

#ifdef __cplusplus
extern "C" {
#endif

int HiLogPrintDictNew(const char *domain, int level, const char *tag,
    const char *fmt, ...);

#ifdef __cplusplus
}
#endif

/* Log levels matching OHOS hilog */
#define LOG_LEVEL_MIN 0
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_INFO 4
#define LOG_LEVEL_WARN 5
#define LOG_LEVEL_ERROR 6
#define LOG_LEVEL_FATAL 7

/* Log type */
#define LOG_CORE 0
#define LOG_APP  1

/* Domain / tag placeholders */
#ifndef LOG_DOMAIN
#define LOG_DOMAIN 0
#endif
#ifndef LOG_TAG
#define LOG_TAG "LLT"
#endif

#define HILOG_PRINT_LOCAL(level, fmt, ...) \
    HiLogPrintDictNew("D002105", level, LOG_TAG, fmt, ##__VA_ARGS__)

#define HILOG_DEBUG(type, fmt, ...) HILOG_PRINT_LOCAL(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define HILOG_INFO(type, fmt, ...)  HILOG_PRINT_LOCAL(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define HILOG_WARN(type, fmt, ...)  HILOG_PRINT_LOCAL(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define HILOG_ERROR(type, fmt, ...) HILOG_PRINT_LOCAL(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define HILOG_FATAL(type, fmt, ...) HILOG_PRINT_LOCAL(LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)

#endif /* HILOG_INCLUDE_HILOG_LOG_H */
