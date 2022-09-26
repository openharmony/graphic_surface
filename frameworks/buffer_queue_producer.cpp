/*
 * Copyright (c) 2020-2022 Huawei Device Co., Ltd.
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

#include "buffer_queue_producer.h"

#include "buffer_common.h"
#include "buffer_manager.h"
#include "buffer_queue.h"
#include "surface_buffer_impl.h"

namespace OHOS {
const int32_t DEFAULT_IPC_SIZE = 100;

extern "C" {
typedef int32_t (*IpcMsgHandle)(BufferQueueProducer* product, IpcIo *io, IpcIo *reply);
};

static int32_t OnRequestBuffer(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint8_t isWaiting;
    ReadUint8(io, &isWaiting);
    SurfaceBufferImpl* buffer = product->RequestBuffer(isWaiting);
    uint32_t ret = -1;
    if (buffer == nullptr) {
        GRAPHIC_LOGW("get buffer failed");
        WriteInt32(reply, -1);
        ret = -1;
    } else {
        WriteInt32(reply, 0);
        buffer->WriteToIpcIo(*reply);
        ret = 0;
    }
    return ret;
}

static int32_t OnFlushBuffer(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    SurfaceBufferImpl buffer;
    buffer.ReadFromIpcIo(*io);
    WriteInt32(reply, product->EnqueueBuffer(buffer));
    return 0;
}

static int32_t OnCancelBuffer(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    SurfaceBufferImpl buffer;
    buffer.ReadFromIpcIo(*io);
    product->Cancel(&buffer);
    WriteInt32(reply, 0);
    return 0;
}

static int32_t OnGetAttr(uint32_t attr, IpcIo *io, IpcIo *reply)
{
    WriteInt32(reply, 0);
    WriteUint32(reply, attr);
    return 0;
}

static int32_t OnSendReply(IpcIo *io, IpcIo *reply)
{
    WriteInt32(reply, 0);
    return 0;
}

static int32_t OnSetQueueSize(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t queueSize;
    ReadUint32(io, &queueSize);
    product->SetQueueSize(queueSize);
    return OnSendReply(io, reply);
}

static int32_t OnGetQueueSize(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetQueueSize(), io, reply);
}

static int32_t OnSetWidthAndHeight(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t width;
    ReadUint32(io, &width);
    uint32_t height;
    ReadUint32(io, &height);
    product->SetWidthAndHeight(width, height);
    return OnSendReply(io, reply);
}

static int32_t OnGetWidth(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetWidth(), io, reply);
}

static int32_t OnGetHeight(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetHeight(), io, reply);
}

static int32_t OnSetFormat(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t format;
    ReadUint32(io, &format);
    product->SetFormat(format);
    return OnSendReply(io, reply);
}

static int32_t OnGetFormat(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetFormat(), io, reply);
}

static int32_t OnSetStrideAlignment(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t strideAlignment;
    ReadUint32(io, &strideAlignment);
    product->SetStrideAlignment(strideAlignment);
    return OnSendReply(io, reply);
}

static int32_t GetStrideAlignment(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetStrideAlignment(), io, reply);
}

static int32_t OnGetStride(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetStride(), io, reply);
}

static int32_t OnSetSize(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t size;
    ReadUint32(io, &size);
    product->SetSize(size);
    return OnSendReply(io, reply);
}

static int32_t OnGetSize(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetSize(), io, reply);
}

static int32_t OnSetUsage(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    uint32_t usage;
    ReadUint32(io, &usage);
    product->SetUsage(usage);
    return OnSendReply(io, reply);
}

static int32_t OnGetUsage(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    return OnGetAttr(product->GetUsage(), io, reply);
}

static int32_t OnSetUserData(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    size_t len = 0;
    const char* key = reinterpret_cast<char *>(ReadString(io, &len));
    if (key == nullptr || len == 0) {
        GRAPHIC_LOGW("Get user data key failed");
        return -1;
    }
    const char* value = reinterpret_cast<char *>(ReadString(io, &len));
    if (value == nullptr || len == 0) {
        GRAPHIC_LOGW("Get user data value failed");
        return -1;
    }
    std::string sKey = key;
    std::string sValue = value;
    product->SetUserData(sKey, sValue);
    OnSendReply(io, reply);
    return 0;
}

static int32_t OnGetUserData(BufferQueueProducer* product, IpcIo *io, IpcIo *reply)
{
    size_t len = 0;
    const char* key = reinterpret_cast<char *>(ReadString(io, &len));
    if (key == nullptr || len == 0) {
        GRAPHIC_LOGW("Get user data key failed");
        return -1;
    }
    std::string sKey = key;
    std::string value = product->GetUserData(sKey);
    WriteString(reply, value.c_str());
    return 0;
}

static IpcMsgHandle g_ipcMsgHandleList[] = {
    OnRequestBuffer,      // REQUEST_BUFFER
    OnFlushBuffer,        // FLUSH_BUFFER
    OnCancelBuffer,       // CANCEL_BUFFER
    OnSetQueueSize,       // SET_QUEUE_SIZE
    OnGetQueueSize,       // GET_QUEUE_SIZE
    OnSetWidthAndHeight,  // SET_WIDTH_AND_HEIGHT
    OnGetWidth,           // GET_WIDTH
    OnGetHeight,          // GET_HEIGHT
    OnSetFormat,          // SET_FORMAT
    OnGetFormat,          // GET_FORMAT
    OnSetStrideAlignment, // SET_STRIDE_ALIGNMENT
    GetStrideAlignment,   // GET_STRIDE_ALIGNMENT
    OnGetStride,          // GET_STRIDE
    OnSetSize,            // SET_SIZE
    OnGetSize,            // GET_SIZE
    OnSetUsage,           // SET_USAGE
    OnGetUsage,           // GET_USAGE
    OnSetUserData,        // SET_USER_DATA
    OnGetUserData,        // GET_USER_DATA
};

BufferQueueProducer::BufferQueueProducer(BufferQueue* bufferQueue)
    : bufferQueue_(bufferQueue),
      consumerListener_(nullptr)
{
}
BufferQueueProducer::~BufferQueueProducer()
{
    consumerListener_ = nullptr;
    if (bufferQueue_ != nullptr) {
        delete bufferQueue_;
        bufferQueue_ = nullptr;
    }
}

SurfaceBufferImpl* BufferQueueProducer::RequestBuffer(uint8_t wait)
{
    RETURN_VAL_IF_FAIL(bufferQueue_, nullptr);
    SurfaceBufferImpl* buffer = bufferQueue_->RequestBuffer(wait);
    return buffer;
}

int32_t BufferQueueProducer::EnqueueBuffer(SurfaceBufferImpl& buffer)
{
    RETURN_VAL_IF_FAIL(bufferQueue_, SURFACE_ERROR_INVALID_PARAM);
    int32_t ret = bufferQueue_->FlushBuffer(buffer);
    if (ret == 0) {
        if (consumerListener_ != nullptr) {
            consumerListener_->OnBufferAvailable();
        }
    }
    return ret;
}

int32_t BufferQueueProducer::FlushBuffer(SurfaceBufferImpl* buffer)
{
    RETURN_VAL_IF_FAIL(buffer, SURFACE_ERROR_INVALID_PARAM);
    RETURN_VAL_IF_FAIL(bufferQueue_, SURFACE_ERROR_INVALID_PARAM);
    BufferManager* manager = BufferManager::GetInstance();
    RETURN_VAL_IF_FAIL(manager, SURFACE_ERROR_NOT_READY);
    if (buffer->GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE_CONSUMER_CACHE) {
        int32_t ret = manager->FlushCache(*buffer);
        if (ret != 0) {
            GRAPHIC_LOGW("Flush buffer failed, ret=%d", ret);
            return ret;
        }
    }
    return EnqueueBuffer(*buffer);
}

void BufferQueueProducer::Cancel(SurfaceBufferImpl* buffer)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->CancelBuffer(*buffer);
}

void BufferQueueProducer::SetQueueSize(uint8_t queueSize)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetQueueSize(queueSize);
}

uint8_t BufferQueueProducer::GetQueueSize()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetQueueSize();
}

void BufferQueueProducer::SetWidthAndHeight(uint32_t width, uint32_t height)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetWidthAndHeight(width, height);
}

uint32_t BufferQueueProducer::GetWidth()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetWidth();
}

uint32_t BufferQueueProducer::GetHeight()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetHeight();
}

void BufferQueueProducer::SetFormat(uint32_t format)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetFormat(format);
}

uint32_t BufferQueueProducer::GetFormat()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetFormat();
}

void BufferQueueProducer::SetStrideAlignment(uint32_t strideAlignment)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetStrideAlignment(strideAlignment);
}

uint32_t BufferQueueProducer::GetStrideAlignment()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetStrideAlignment();
}

uint32_t BufferQueueProducer::GetStride()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetStride();
}

void BufferQueueProducer::SetSize(uint32_t size)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetSize(size);
}

uint32_t BufferQueueProducer::GetSize()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetSize();
}

void BufferQueueProducer::SetUsage(uint32_t usage)
{
    RETURN_IF_FAIL(bufferQueue_);
    bufferQueue_->SetUsage(usage);
}

uint32_t BufferQueueProducer::GetUsage()
{
    RETURN_VAL_IF_FAIL(bufferQueue_, 0);
    return bufferQueue_->GetUsage();
}

void BufferQueueProducer::RegisterConsumerListener(IBufferConsumerListener& listener)
{
    consumerListener_ = &listener;
}

void BufferQueueProducer::UnregisterConsumerListener()
{
    consumerListener_ = nullptr;
}

int32_t BufferQueueProducer::OnIpcMsg(uint32_t code, IpcIo *data, IpcIo *reply, MessageOption option)
{
    if (data == NULL) {
        GRAPHIC_LOGW("Invalid parameter, null pointer");
        return SURFACE_ERROR_INVALID_PARAM;
    }

    if (code >= MAX_REQUEST_CODE) {
        GRAPHIC_LOGW("Resquest code(%u) does not support.", code);
        return SURFACE_ERROR_INVALID_REQUEST;
    }
    return g_ipcMsgHandleList[code](this, data, reply);
}

void BufferQueueProducer::SetUserData(const std::string& key, const std::string& value)
{
    bufferQueue_->SetUserData(key, value);
}

std::string BufferQueueProducer::GetUserData(const std::string& key)
{
    return bufferQueue_->GetUserData(key);
}
} // end namespace
