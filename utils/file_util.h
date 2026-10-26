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

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <dirent.h>

namespace OHOS {
namespace IntellVoiceUtils {
static const int S_IRWXQ = 0770;
enum FileStatus {
    FILE_STATUS_FILE = 0,
    FILE_STATUS_DIR,
    FILE_STATUS_ERROR
};

class FileUtil {
public:
    FileUtil() {};
    ~FileUtil() {};
    static bool FileExist(const char *file);
    static bool CheckDir(const char *path, uint32_t &loop);
    static FileStatus IsDir(const char *path);
    static bool CheckAndCreateDir(const std::string &dir);
    static bool DeleteDir(const char *path, uint32_t loop = 0);
    static bool GetDirFiles(const std::string &path, std::vector<std::string> &files);
    static void GetDirFilePath(const std::string &dir, std::vector<std::string> &filePath);
    static size_t GetFileSize(const char *file);
    static int GetFileString(const std::string &path, std::string &result, int sizeMax);
    static inline bool MakeDir(const char *dir);
private:
    FileUtil(FileUtil &&) = delete;
    FileUtil &operator = (FileUtil &&) = delete;

    FileUtil(const FileUtil &) = delete;
    FileUtil &operator = (const FileUtil &) = delete;
};

inline bool FileUtil::MakeDir(const char *dir)
{
    if (!FileExist(dir)) {
        if (mkdir(dir, S_IRWXQ) == -1) {
            return false;
        }
    }

    return true;
}
}
}
#endif