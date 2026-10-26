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

#ifndef FILE_REPORTER_H
#define FILE_REPORTER_H

#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ashmem.h>
#include "array_buffer_util.h"

namespace OHOS {
namespace IntellVoiceUtils {
enum ReportFileType {
    FILE_TYPE_ENROLL = 0,
    FILE_TYPE_WAKEUP = 1,
    FILE_TYPE_INVALID = 2,
};

struct ReportFilesInfo {
    ReportFileType type;
    std::string filesDescription;
    std::vector<sptr<Ashmem>> filesContent;
};

class FileReporter {
public:
    FileReporter() {};
    ~FileReporter() {};
    static int GetReportFiles(bool needDeCrypt, int count, std::vector<ReportFilesInfo> &reportFiles);
private:
    static bool GetFileInfo(bool needDeCrypt, const std::string &fileDir, ReportFilesInfo &filesInfor);
    static sptr<Ashmem> CreateAshmemFromArrayBuffer(const std::unique_ptr<Uint8ArrayBuffer> &buffer);
    static sptr<Ashmem> CreateAshmemFromFilesInfo(const std::vector<uint8_t> &filesContent);
};
}
}
#endif