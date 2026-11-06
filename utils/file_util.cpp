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

#include "file_util.h"
#include <securec.h>
#include "intell_voice_log.h"

#undef LOG_TAG
#define LOG_TAG "FileUtil"

using namespace std;

namespace OHOS {
namespace IntellVoiceUtils {
static const int MAX_FILE_NUM = 10000;
static const int MAX_RECURSION_NUM = 500;
bool FileUtil::CheckAndCreateDir(const string &dir)
{
    if (!FileUtil::FileExist(dir.c_str())) {
        if (!FileUtil::MakeDir(dir.c_str())) {
            return false;
        }
    }
    return true;
}

bool FileUtil::DeleteDir(const char *path, uint32_t loop)
{
    char filePath[PATH_MAX] = {0};
    struct dirent *dirInfo = nullptr;

    uint32_t loopNum = loop;
    if (!CheckDir(path, loopNum)) {
        return false;
    }

    if (IsDir(path) == FILE_STATUS_FILE) {
        return !remove(path);
    }

    DIR *dir = opendir(path);
    if (dir == nullptr) {
        return false;
    }
    int32_t count = 0;
    while ((dirInfo = readdir(dir)) != nullptr) {
        if (strcmp(dirInfo->d_name, ".") == 0 || strcmp(dirInfo->d_name, "..") == 0) {
            continue;
        }
        std::string tmpPath = std::string(path) + "/" + std::string(dirInfo->d_name);
        if (strcpy_s(filePath, PATH_MAX, tmpPath.c_str()) != EOK) {
            (void)closedir(dir);
            return false;
        }
        FileStatus ret = IsDir(filePath);
        if (ret == FILE_STATUS_DIR) {
            if (!DeleteDir(filePath, loopNum)) {
                (void)closedir(dir);
                return false;
            }
        } else if (ret == FILE_STATUS_FILE) {
            if (remove(filePath) != 0) {
                (void)closedir(dir);
                return false;
            }
        }

        if (++count > MAX_FILE_NUM) {
            INTELL_VOICE_LOG_WARN("files num is more than maximum: %u, stop to delete dir", MAX_FILE_NUM);
            break;
        }
    }
    (void)closedir(dir);
    if (rmdir(path) != 0) {
        return false;
    }

    return true;
}

void FileUtil::GetDirFilePath(const string &dir, vector<string> &filePath)
{
    vector<string> fileName;

    if (!GetDirFiles(dir, fileName)) {
        INTELL_VOICE_LOG_ERROR("get dir files failed");
        return;
    }

    for (uint32_t i = 0; i < fileName.size(); i++) {
        filePath.push_back(dir + "/" + fileName[i]);
    }
}

// info -- parameter 'files' of function 'GetDirFiles' could be reference to const is false alarm, ignore
bool FileUtil::GetDirFiles(const string &path, vector<string> &files) //lint !e1764
{
    if (path.empty()) {
        INTELL_VOICE_LOG_ERROR("path is empty");
        return false;
    }

    string dirPath = path;
    DIR *dir = opendir(dirPath.append("/").c_str());
    if (dir == nullptr) {
        INTELL_VOICE_LOG_ERROR("can not open file");
        return false;
    }

    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        } else {
            files.push_back(entry->d_name);
            if (files.size() > MAX_FILE_NUM) {
                INTELL_VOICE_LOG_ERROR("files num is too big");
                (void)closedir(dir);
                return false;
            }
        }
    }

    closedir(dir);
    return true;
}

bool FileUtil::CheckDir(const char *path, uint32_t &loop)
{
    if (path == nullptr) {
        INTELL_VOICE_LOG_ERROR("path is nullptr");
        return false;
    }

    if (++loop > MAX_RECURSION_NUM) {
        INTELL_VOICE_LOG_ERROR("number of recursions is more than maximum");
        return false;
    }

    if (IsDir(path) == FILE_STATUS_ERROR) {
        INTELL_VOICE_LOG_INFO("file status is error");
        return false;
    }
    return true;
}

FileStatus FileUtil::IsDir(const char *path)
{
    if (path == nullptr) {
        INTELL_VOICE_LOG_ERROR("path is nullptr");
        return FILE_STATUS_ERROR;
    }
    struct stat statBuf;
    if (stat(path, &statBuf) != 0) {
        INTELL_VOICE_LOG_WARN("get file status failed");
        return FILE_STATUS_ERROR;
    }

    if (S_ISDIR(statBuf.st_mode) != 0) {
        return FILE_STATUS_DIR;
    } else if (S_ISREG(statBuf.st_mode) != 0) {
        return FILE_STATUS_FILE;
    }

    return FILE_STATUS_ERROR;
}

bool FileUtil::FileExist(const char *file)
{
    if (file == nullptr) {
        INTELL_VOICE_LOG_ERROR("file is nullptr");
        return false;
    }

    struct stat sb;
    int ret = stat(file, &sb);
    if (ret != 0) {
        INTELL_VOICE_LOG_INFO("stat failed");
    }
    return (ret == 0);
}

size_t FileUtil::GetFileSize(const char *file)
{
    if (file == nullptr) {
        INTELL_VOICE_LOG_ERROR("file is nullptr");
        return 0;
    }

    struct stat sb;
    if (stat(file, &sb) == -1) {
        INTELL_VOICE_LOG_ERROR("get file status failed");
        return 0;
    }
    return static_cast<size_t>(sb.st_size);
}

int FileUtil::GetFileString(const std::string &path, string &result, int sizeMax)
{
    int fileSize = FileUtil::GetFileSize(path.c_str());
    std::ifstream inStream(path);
    if (!inStream) {
        INTELL_VOICE_LOG_ERROR("fail open file");
        return -1;
    }

    int readLen = fileSize < sizeMax ? fileSize : sizeMax;
    auto buff = std::shared_ptr<char>(
        new char[sizeMax], 
        std::default_delete<char[]>()
    );
    if (buff == nullptr) {
        return -1;
    }

    inStream.read(buff.get(), readLen);
    result = std::string(buff.get(), readLen);
    return 0;
}
}
}