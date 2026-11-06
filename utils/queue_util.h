/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef INTELL_VOICE_BUFFER_QUEUE_H
#define INTELL_VOICE_BUFFER_QUEUE_H

#include <unistd.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "array_buffer_util.h"
#include "intell_voice_log.h"

#define LOG_TAG "QueueUtil"

namespace OHOS {
namespace IntellVoiceUtils {
constexpr uint32_t MAX_CAPACITY = 500;

template <typename T>
class QueueUtil {
public:
    QueueUtil() = default;
    ~QueueUtil()
    {
        Uninit();
    }
    bool Init(uint32_t capacity = MAX_CAPACITY)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        SetAvailable(true);
        capacity_ = capacity;
        return true;
    }
    bool Push(const T &element, bool isWait = true)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");

        while (queue_.size() >= capacity_) {
            CHECK_CONDITION_RETURN_FALSE((!isWait), "queue is full, no need to wait");
            notFullCv_.wait(lock, [&]() { return ((queue_.size() < capacity_) || (!IsAvailable())); });
            CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");
        }

        queue_.push(element);
        notEmptyCv_.notify_one();
        return true;
    }
    bool Push(T &&element, bool isWait = true)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");

        while (queue_.size() >= capacity_) {
            if (!isWait) {
                return false;
            }
            notFullCv_.wait(lock, [&]() { return ((queue_.size() < capacity_) || (!IsAvailable())); });
            CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");
        }

        queue_.push(std::move(element));
        notEmptyCv_.notify_one();
        return true;
    }
    bool Pop(T &element)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");

        while (queue_.empty()) {
            notEmptyCv_.wait(lock, [&] { return (!queue_.empty() || !IsAvailable()); });
            CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");
        }

        element = std::move(queue_.front());
        queue_.pop();
        notFullCv_.notify_one();
        return true;
    }
    bool PopUntilTimeout(uint32_t timeLenMs, T &element)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");

        while (queue_.empty()) {
            if (!(notEmptyCv_.wait_for(lock, std::chrono::milliseconds(timeLenMs),
                [&] { return (!queue_.empty() || !IsAvailable()); }))) {
                INTELL_VOICE_LOG_WARN("wait time out");
                return false;
            }
            CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");
        }

        element = std::move(queue_.front());
        queue_.pop();
        notFullCv_.notify_one();
        return true;
    }
    bool PopLen(uint32_t maxLen, std::vector<T> &list)
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        CHECK_CONDITION_RETURN_FALSE(!IsAvailable(), "queue is not available");
        uint32_t count = 0;
        while (!queue_.empty() && count < maxLen) {
            list.push_back(std::move(queue_.front()));
            queue_.pop();
            count++;
        }
        return true;
    }
    void Uninit()
    {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            capacity_ = 0;
            ClearQueue();
            SetAvailable(false);
        }
        notEmptyCv_.notify_all();
        notFullCv_.notify_all();
    }
private:
    bool IsAvailable() const
    {
        return isAvailable_;
    }
    void SetAvailable(bool isAvailable)
    {
        isAvailable_ = isAvailable;
    }
    void ClearQueue()
    {
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    bool isAvailable_ = false;
    uint32_t capacity_ = 0;
    std::mutex queueMutex_;
    std::condition_variable notEmptyCv_;
    std::condition_variable notFullCv_;
    std::queue<T> queue_;
};

using Uint8ArrayBufferQueue = QueueUtil<std::unique_ptr<Uint8ArrayBuffer>>;
}
}

#undef LOG_TAG

#endif