/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <queue>
#include <memory>
#include <string>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "nocopyable.h"

namespace OHOS {
namespace IntellVoiceUtils {
struct SynInfo {
    std::mutex mMutex;
    std::condition_variable mCV;
};
struct Message {
public:
    explicit Message(uint32_t what);
    Message(uint32_t what, int32_t arg1);
    Message(uint32_t what, int32_t arg1, int32_t arg2, float arg3);
    Message(uint32_t what, int32_t arg1, int32_t arg2, const std::string &obj);
    Message(uint32_t what, int32_t arg1, int32_t arg2, float arg3, const std::string &obj);
    Message(uint32_t what, std::shared_ptr<void> obj2);
    Message(uint32_t what, std::shared_ptr<void> obj2, std::shared_ptr<void> obj3);
    Message(uint32_t what, void* voidPtr);
    Message(uint32_t what, void* voidPtr, int32_t arg1);
    ~Message();

    uint32_t mWhat { 0 };
    int32_t arg1 { 0 };
    int32_t arg2 { 0 };
    float arg3 { 0.0 };
    std::string obj;
    std::shared_ptr<void> obj2 { nullptr };
    std::shared_ptr<void> obj3 { nullptr };
    void* voidPtr { nullptr };
    uint32_t mCallbackId { 0 };

    std::shared_ptr<SynInfo> result { nullptr };
};

class MessageQueue {
public:
    explicit MessageQueue(uint32_t size);
    ~MessageQueue();
    std::shared_ptr<Message> ReceiveMsg();
    bool SendMsg(std::shared_ptr<Message> msg);
    void Clear();

private:
    DISALLOW_COPY(MessageQueue);
    DISALLOW_MOVE(MessageQueue);

    uint32_t mSize;
    pthread_mutex_t mLock;
    pthread_cond_t mCond;
    std::queue<std::shared_ptr<Message>> mQueue;
};
}
}
#endif
