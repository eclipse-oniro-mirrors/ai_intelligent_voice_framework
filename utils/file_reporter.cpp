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

#include "file_reporter.h"
#include "file_util.h"
#include "intell_voice_log.h"
#include "huks_aes_adapter.h"
#include <random>

#undef LOG_TAG
#define LOG_TAG "FileReporter"

using namespace std;

namespace OHOS {
namespace IntellVoiceUtils {
const string INTELL_VOICE_DEFAULT_DATA_PATH = "/data/service/el0/intell_voice/report";
constexpr int REPORT_JSON_SIZE_MAX = 4096;
const string PCM_FILE_SUFFIX = ".pcm";
int FileReporter::GetReportFiles(bool needDeCrypt, int count, vector<ReportFilesInfo> &reportFiles)
{
    if (count <= 0) {
        INTELL_VOICE_LOG_WARN("count %d is invalid", count);
        return 0;
    }

    vector<string> reportDirs;
    if (!FileUtil::GetDirFiles(INTELL_VOICE_DEFAULT_DATA_PATH, reportDirs)) {
        INTELL_VOICE_LOG_WARN("get report file fail");
        return 0;
    }

    vector<string> selectedFiles = reportDirs;
    if (static_cast<int>(reportDirs.size()) > count) {
        selectedFiles.clear();
        random_device rd;
        uint32_t end = reportDirs.size() - 1;
        for (int i = 0; i < count; i++) {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(end));
            uint32_t randomIndex = static_cast<uint32_t>(dist(rd));
            selectedFiles.push_back(reportDirs[randomIndex]);
            reportDirs[randomIndex] = reportDirs[end];
            end--;
        }
    }

    for (auto &iter : selectedFiles) {
        ReportFilesInfo reportInfo;
        string filePath = INTELL_VOICE_DEFAULT_DATA_PATH + "/" + iter;
        if (GetFileInfo(needDeCrypt, filePath, reportInfo)) {
            reportFiles.push_back(reportInfo);
        }
        FileUtil::DeleteDir(filePath.c_str());
    }
    return static_cast<int>(reportFiles.size());
}

bool FileReporter::GetFileInfo(bool needDeCrypt, const string &fileDir, ReportFilesInfo &filesInfor)
{
    filesInfor.type = FILE_TYPE_WAKEUP;
    string jsonFile = std::string("/") + "wakeup.json";

    if (FileUtil::GetFileString(fileDir + jsonFile, filesInfor.filesDescription, REPORT_JSON_SIZE_MAX) != 0) {
        INTELL_VOICE_LOG_ERROR("fail to read json file!");
        return false;
    }

    vector<string> dirFiles;
    if (!FileUtil::GetDirFiles(fileDir, dirFiles)) {
        INTELL_VOICE_LOG_WARN("fail to get dir files");
        return false;
    }

    size_t suffixLen = PCM_FILE_SUFFIX.length();
    for (auto &file : dirFiles) {
        string filePath = fileDir + "/" + file;
        // not end with .pcm, skip
        if (filePath.substr(filePath.length() - suffixLen, suffixLen) != PCM_FILE_SUFFIX) {
            continue;
        }

        size_t fileSize = FileUtil::GetFileSize(filePath.c_str());
        if (fileSize == 0) {
            INTELL_VOICE_LOG_WARN("file size error");
            continue;
        }

        vector<uint8_t> pcmContent(fileSize);
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) {
            INTELL_VOICE_LOG_WARN("fail to open file");
            continue;
        }
        inFile.read(reinterpret_cast<char*>(pcmContent.data()), fileSize);
        inFile.close();
        if (needDeCrypt) {
            std::unique_ptr<Uint8ArrayBuffer> inData = CreateArrayBuffer<uint8_t>(fileSize, pcmContent.data());
            std::unique_ptr<Uint8ArrayBuffer> outData = nullptr;
            int32_t ret = HuksAesAdapter::Decrypt(inData, outData);
            if (ret != HKS_SUCCESS) {
                INTELL_VOICE_LOG_ERROR("decrypt failed, ret:%{public}d", ret);
                continue;
            }
            filesInfor.filesContent.push_back(CreateAshmemFromArrayBuffer(outData));
        } else {
            filesInfor.filesContent.push_back(CreateAshmemFromFilesInfo(pcmContent));
        }
    }

    // if pcm file is null, return false
    if (filesInfor.filesContent.size() == 0) {
        INTELL_VOICE_LOG_WARN("pcm file null");
        return false;
    }

    return true;
}

sptr<Ashmem> FileReporter::CreateAshmemFromArrayBuffer(
    const std::unique_ptr<Uint8ArrayBuffer> &buffer)
{
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("buffer is nullptr");
        return nullptr;
    }

    if ((buffer->GetSize() == 0) || (buffer->GetData() == nullptr)) {
        INTELL_VOICE_LOG_ERROR("data is empty");
        return nullptr;
    }

    sptr<Ashmem> ashmem = OHOS::Ashmem::CreateAshmem("FilesContent", buffer->GetSize() * sizeof(uint8_t));
    if (ashmem == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create ashmem");
        return nullptr;
    }

    if (!ashmem->MapReadAndWriteAshmem()) {
        INTELL_VOICE_LOG_ERROR("failed to map ashmem");
        goto ERR_EXIT;
    }

    if (!ashmem->WriteToAshmem(buffer->GetData(), buffer->GetSize() * sizeof(uint8_t), 0)) {
        INTELL_VOICE_LOG_ERROR("failed to write ashmem");
        goto ERR_EXIT;
    }

    INTELL_VOICE_LOG_INFO("create ashmem success, size:%{public}u",
        static_cast<uint32_t>(buffer->GetSize() * sizeof(uint8_t)));
    return ashmem;

ERR_EXIT:
    if (ashmem != nullptr) {
        ashmem->UnmapAshmem();
        ashmem->CloseAshmem();
    }
    return nullptr;
}

sptr<Ashmem> FileReporter::CreateAshmemFromFilesInfo(const std::vector<uint8_t> &filesContent)
{
    if (filesContent.size() == 0) {
        INTELL_VOICE_LOG_ERROR("filesContent is empty");
        return nullptr;
    }

    sptr<Ashmem> buffer = OHOS::Ashmem::CreateAshmem("FilesContent", filesContent.size());
    if (buffer == nullptr) {
        INTELL_VOICE_LOG_ERROR("failed to create ashmem");
        return nullptr;
    }

    if (!buffer->MapReadAndWriteAshmem()) {
        INTELL_VOICE_LOG_ERROR("failed to map ashmem");
        goto ERR_EXIT;
    }

    if (!buffer->WriteToAshmem(filesContent.data(), filesContent.size(), 0)) {
        INTELL_VOICE_LOG_ERROR("failed to write ashmem");
        goto ERR_EXIT;
    }

    INTELL_VOICE_LOG_INFO("filesContent size:%{public}zu", filesContent.size());
    return buffer;

ERR_EXIT:
    buffer->UnmapAshmem();
    buffer->CloseAshmem();
    return nullptr;
}
}
}