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
 * x86_64 stub for hilog.
 * Provides HiLogPrintDictNew as a printf fallback.
 * Strips %{public}/%{private} modifiers so standard vprintf works correctly.
 */

#include <cstdio>
#include <cstdarg>
#include <string>

static std::string StripHilogModifiers(const char *fmt)
{
    std::string result;
    std::string src(fmt);
    size_t i = 0;
    while (i < src.size()) {
        if (src[i] == '%' && i + 1 < src.size() && src[i + 1] == '{') {
            size_t end = src.find('}', i + 2);
            if (end != std::string::npos) {
                result += '%';
                i = end + 1;
                continue;
            }
        }
        result += src[i];
        i++;
    }
    return result;
}

extern "C" int HiLogPrintDictNew(const char *domain, int level, const char *tag,
    const char *fmt, ...)
{
    (void)domain;
    (void)level;
    (void)tag;
    std::string stripped = StripHilogModifiers(fmt);
    va_list ap;
    va_start(ap, fmt);
    int ret = vprintf(stripped.c_str(), ap);
    va_end(ap);
    putchar('\n');
    return ret;
}
