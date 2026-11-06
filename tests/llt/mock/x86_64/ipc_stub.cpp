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
 * x86_64 stub for IPC framework (libipc_single.z.so).
 * Includes real IPC headers for type compatibility, provides no-op implementations.
 * The LLT tests do not use real binder IPC — these stubs just satisfy the linker.
 */

#include <unistd.h>
// Include real IPC headers (not mock versions) — stub provides implementations for linking
// Use the real header directory explicitly to avoid picking up mock versions
#include "message_option.h"
#include "iremote_object.h"
#include "ipc_object_stub.h"
#include "ipc_object_proxy.h"
#include "message_parcel.h"
#include "iremote_broker.h"
#include "peer_holder.h"
#include "ashmem.h"
// For IPCSkeleton, the mock dir has a stripped-down version.
// We match the mock-declared subset.
#include "ipc_skeleton.h"

namespace OHOS {

// ---- Ashmem stubs ----

sptr<Ashmem> Ashmem::CreateAshmem(const char *name, int size)
{
    (void)name;
    (void)size;
    return nullptr;
}

void Ashmem::CloseAshmem() {}
int32_t Ashmem::GetAshmemSize() const { return 0; }

bool Ashmem::MapAshmem(int mapType)
{
    (void)mapType;
    return false;
}

bool Ashmem::MapReadAndWriteAshmem() { return false; }
bool Ashmem::MapReadOnlyAshmem() { return false; }
void Ashmem::UnmapAshmem() {}

bool Ashmem::WriteToAshmem(const void *data, int32_t size, int32_t offset)
{
    (void)data;
    (void)size;
    (void)offset;
    return false;
}

const void *Ashmem::ReadFromAshmem(int32_t size, int32_t offset)
{
    (void)size;
    (void)offset;
    return nullptr;
}

// ---- IPCObjectStub stubs ----

IPCObjectStub::IPCObjectStub(std::u16string descriptor, bool serialInvokeFlag)
    : IRemoteObject(descriptor), serialInvokeFlag_(serialInvokeFlag),
      lastRequestTime_(0), requestSidFlag_(false) {}

IPCObjectStub::~IPCObjectStub() = default;

int IPCObjectStub::SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}

int IPCObjectStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}

int IPCObjectStub::Dump(int fd, const std::vector<std::u16string> &args)
{
    (void)fd;
    (void)args;
    return 0;
}

bool IPCObjectStub::AddDeathRecipient(
    const sptr<IRemoteObject::DeathRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectStub::RemoveDeathRecipient(
    const sptr<IRemoteObject::DeathRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectStub::AddRefreshRecipient(
    const sptr<IRemoteObject::RefreshRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectStub::RemoveRefreshRecipient(
    const sptr<IRemoteObject::RefreshRecipient> &recipient)
{
    (void)recipient;
    return true;
}

int32_t IPCObjectStub::GetObjectRefCount() { return 1; }
int IPCObjectStub::GetObjectType() const { return IRemoteObject::IF_PROT_BINDER; }

void IPCObjectStub::OnFirstStrongRef(const void *objectId)
{
    (void)objectId;
}

void IPCObjectStub::OnLastStrongRef(const void *objectId)
{
    (void)objectId;
}

int32_t IPCObjectStub::ProcessProto(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}

int IPCObjectStub::GetAndSaveDBinderData(pid_t pid, uid_t uid)
{
    (void)pid;
    (void)uid;
    return 0;
}

int IPCObjectStub::OnRemoteDump(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}

uint64_t IPCObjectStub::GetLastRequestTime() { return 0; }
bool IPCObjectStub::GetRequestSidFlag() const { return false; }

void IPCObjectStub::SetRequestSidFlag(bool flag)
{
    (void)flag;
}

int IPCObjectStub::GetCallingPid() { return getpid(); }
int IPCObjectStub::GetCallingUid() { return getuid(); }
uint32_t IPCObjectStub::GetCallingTokenID() { return 0; }
uint64_t IPCObjectStub::GetCallingFullTokenID() { return 0; }
uint32_t IPCObjectStub::GetFirstTokenID() { return 0; }
uint64_t IPCObjectStub::GetFirstFullTokenID() { return 0; }

#ifndef CONFIG_IPC_SINGLE
int IPCObjectStub::DBinderClearServiceState(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}
#endif

// ---- MessageOption stubs ----

MessageOption::MessageOption(int flags, int waitTime) : flags_(flags), waitTime_(waitTime) {}
void MessageOption::SetFlags(int flags) { flags_ = flags; }
int MessageOption::GetFlags() const { return flags_; }
void MessageOption::SetWaitTime(int waitTime) { waitTime_ = waitTime; }
int MessageOption::GetWaitTime() const { return waitTime_; }

// ---- IPCObjectProxy stubs ----

IPCObjectProxy::IPCObjectProxy(int handle, std::u16string descriptor, int proto)
    : IRemoteObject(descriptor)
{
    (void)handle;
    (void)proto;
}

IPCObjectProxy::~IPCObjectProxy() = default;

int32_t IPCObjectProxy::GetObjectRefCount() { return 1; }

int IPCObjectProxy::SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    (void)code;
    (void)data;
    (void)reply;
    (void)option;
    return 0;
}

bool IPCObjectProxy::IsObjectDead() const { return false; }

bool IPCObjectProxy::AddDeathRecipient(
    const sptr<IRemoteObject::DeathRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectProxy::RemoveDeathRecipient(
    const sptr<IRemoteObject::DeathRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectProxy::AddRefreshRecipient(
    const sptr<IRemoteObject::RefreshRecipient> &recipient)
{
    (void)recipient;
    return true;
}

bool IPCObjectProxy::RemoveRefreshRecipient(
    const sptr<IRemoteObject::RefreshRecipient> &recipient)
{
    (void)recipient;
    return true;
}

int IPCObjectProxy::Dump(int fd, const std::vector<std::u16string> &args)
{
    (void)fd;
    (void)args;
    return 0;
}

std::u16string IPCObjectProxy::GetInterfaceDescriptor() { return u""; }

void IPCObjectProxy::OnFirstStrongRef(const void *objectId)
{
    (void)objectId;
}

void IPCObjectProxy::OnLastStrongRef(const void *objectId)
{
    (void)objectId;
}

bool IPCObjectProxy::CanPromote() { return false; }

// ---- MessageParcel stubs ----

MessageParcel::MessageParcel() {}
MessageParcel::~MessageParcel() {}

sptr<Ashmem> MessageParcel::ReadAshmem() { return nullptr; }
std::u16string MessageParcel::ReadInterfaceToken() { return u""; }
sptr<IRemoteObject> MessageParcel::ReadRemoteObject() { return nullptr; }

bool MessageParcel::WriteAshmem(sptr<Ashmem> ashmem)
{
    (void)ashmem;
    return true;
}

bool MessageParcel::WriteInterfaceToken(std::u16string descriptor)
{
    (void)descriptor;
    return true;
}

bool MessageParcel::WriteRemoteObject(const sptr<IRemoteObject> &object)
{
    (void)object;
    return true;
}

// ---- IRemoteObject stubs ----

IRemoteObject::IRemoteObject(std::u16string descriptor) : descriptor_(descriptor) {}

sptr<IRemoteBroker> IRemoteObject::AsInterface() { return nullptr; }
bool IRemoteObject::CheckObjectLegality() const { return false; }
std::u16string IRemoteObject::GetInterfaceDescriptor() { return u""; }
bool IRemoteObject::IsObjectDead() const { return false; }
bool IRemoteObject::Marshalling(Parcel &) const { return false; }
bool IRemoteObject::IsProxyObject() const { return false; }

sptr<IRemoteObject> IRemoteObject::Unmarshalling(Parcel &parcel)
{
    (void)parcel;
    return nullptr;
}

bool IRemoteObject::Marshalling(Parcel &parcel, const sptr<IRemoteObject> &object)
{
    (void)parcel;
    (void)object;
    return false;
}

std::u16string IRemoteObject::GetObjectDescriptor() const { return descriptor_; }

// ---- PeerHolder stubs ----

PeerHolder::PeerHolder(const sptr<IRemoteObject> &object) : remoteObject_(object) {}
sptr<IRemoteObject> PeerHolder::Remote() { return remoteObject_; }

// ---- IPCSkeleton stubs (matching mock/ipc_skeleton.h declarations) ----

pid_t IPCSkeleton::GetCallingUid() { return getuid(); }
pid_t IPCSkeleton::GetCallingPid() { return getpid(); }
uint32_t IPCSkeleton::GetCallingTokenID() { return 0; }
uint64_t IPCSkeleton::GetCallingFullTokenID() { return 0; }
std::string IPCSkeleton::GetCallingDeviceID() { return ""; }
int64_t IPCSkeleton::GetCallingTime() { return 0; }
bool IPCSkeleton::IsLocalCalling() { return true; }
void IPCSkeleton::FlushCommands() {}
void IPCSkeleton::JoinWorkThread() {}

// ---- BrokerRegistration stubs ----

BrokerRegistration &BrokerRegistration::Get()
{
    static BrokerRegistration instance;
    return instance;
}

BrokerRegistration::~BrokerRegistration() = default;

sptr<IRemoteBroker> BrokerRegistration::NewInstance(
    const std::u16string &descriptor, const sptr<IRemoteObject> &object)
{
    (void)descriptor;
    (void)object;
    return nullptr;
}

bool BrokerRegistration::Register(const std::u16string &descriptor,
    const Constructor &creator, const BrokerDelegatorBase *object)
{
    (void)descriptor;
    (void)creator;
    (void)object;
    return true;
}

void BrokerRegistration::Unregister(const std::u16string &descriptor)
{
    (void)descriptor;
}

} // namespace OHOS
