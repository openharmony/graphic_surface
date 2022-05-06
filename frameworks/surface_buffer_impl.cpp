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

#include "surface_buffer_impl.h"
#include "securec.h"

namespace OHOS {
const uint16_t MAX_USER_DATA_COUNT = 1000;

SurfaceBufferImpl::SurfaceBufferImpl() : len_(0)
{
    struct SurfaceBufferData bufferData = {{0}, 0, 0, 0, BUFFER_STATE_NONE, NULL};
    bufferData_ = bufferData;
}

int32_t SurfaceBufferImpl::SetInt32(uint32_t key, int32_t value)
{
    return SetData(key, BUFFER_DATA_TYPE_INT_32, &value, sizeof(value));
}

int32_t SurfaceBufferImpl::GetInt32(uint32_t key, int32_t& value)
{
    uint8_t type = BUFFER_DATA_TYPE_NONE;
    void *data = nullptr;
    uint8_t size;
    if (GetData(key, &type, &data, &size) != SURFACE_ERROR_OK || type != BUFFER_DATA_TYPE_INT_32) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    if (size != sizeof(value)) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    value = *(reinterpret_cast<int32_t *>(data));
    return SURFACE_ERROR_OK;
}

int32_t SurfaceBufferImpl::SetInt64(uint32_t key, int64_t value)
{
    return SetData(key, BUFFER_DATA_TYPE_INT_64, &value, sizeof(value));
}

int32_t SurfaceBufferImpl::GetInt64(uint32_t key, int64_t& value)
{
    uint8_t type = BUFFER_DATA_TYPE_NONE;
    void *data = nullptr;
    uint8_t size;
    if (GetData(key, &type, &data, &size) != SURFACE_ERROR_OK || type != BUFFER_DATA_TYPE_INT_64) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    if (size != sizeof(value)) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    value = *(reinterpret_cast<int64_t *>(data));
    return SURFACE_ERROR_OK;
}

int32_t SurfaceBufferImpl::SetData(uint32_t key, uint8_t type, const void* data, uint8_t size)
{
    if (type <= BUFFER_DATA_TYPE_NONE ||
        type >= BUFFER_DATA_TYPE_MAX ||
        size <= 0 ||
        size > sizeof(int64_t)) {
        GRAPHIC_LOGI("Invalid Param");
        return SURFACE_ERROR_INVALID_PARAM;
    }
    if (extDatas_.size() > MAX_USER_DATA_COUNT) {
        GRAPHIC_LOGI("No more data can be saved because the storage space is full.");
        return SURFACE_ERROR_SYSTEM_ERROR;
    }
    ExtraData extData = {0};
    std::map<uint32_t, ExtraData>::iterator iter = extDatas_.find(key);
    if (iter != extDatas_.end()) {
        extData = iter->second;
        if (size != extData.size) {
            free(extData.value);
            extData.value = nullptr;
        }
    }
    if (extData.value == nullptr) {
        extData.value = malloc(size);
        if (extData.value == nullptr) {
            GRAPHIC_LOGE("Couldn't allocate %zu bytes for ext data", size);
            return SURFACE_ERROR_SYSTEM_ERROR;
        }
    }
    if (memcpy_s(extData.value, size, data, size) != EOK) {
        free(extData.value);
        GRAPHIC_LOGW("Couldn't copy %zu bytes for ext data", size);
        return SURFACE_ERROR_SYSTEM_ERROR;
    }
    extData.size = size;
    extData.type = type;
    extDatas_[key] = extData;
    return SURFACE_ERROR_OK;
}

int32_t SurfaceBufferImpl::GetData(uint32_t key, uint8_t* type, void** data, uint8_t* size)
{
    if ((type == nullptr) || (data == nullptr) || (size == nullptr)) {
        return SURFACE_ERROR_INVALID_PARAM;
    }

    std::map<uint32_t, ExtraData>::iterator iter = extDatas_.find(key);
    if (iter == extDatas_.end()) {
        return SURFACE_ERROR_INVALID_PARAM;
    }
    ExtraData extData = extDatas_[key];
    *data = extData.value;
    *size = extData.size;
    *type = extData.type;
    return SURFACE_ERROR_OK;
}

void SurfaceBufferImpl::ReadFromIpcIo(IpcIo& io)
{
    ReadInt32(&io, &(bufferData_.handle.key));
    ReadUint64(&io, &(bufferData_.handle.phyAddr));
    ReadUint32(&io, &(bufferData_.handle.reserveFds));
    ReadUint32(&io, &(bufferData_.handle.reserveInts));
    ReadUint32(&io, &(bufferData_.size));
    ReadUint32(&io, &(bufferData_.usage));
    ReadUint32(&io, &len_);
    uint32_t extDataSize;
    ReadUint32(&io, &extDataSize);
    if (extDataSize > 0 && extDataSize < MAX_USER_DATA_COUNT) {
        for (uint32_t i = 0; i < extDataSize; i++) {
            uint32_t key;
            ReadUint32(&io, &key);
            uint32_t type;
            ReadUint32(&io, &type);
            switch (type) {
                case BUFFER_DATA_TYPE_INT_32: {
                    int32_t value;
                    ReadInt32(&io, &value);
                    SetInt32(key, value);
                    break;
                }
                case BUFFER_DATA_TYPE_INT_64: {
                    int64_t value;
                    ReadInt64(&io, &value);
                    SetInt64(key, value);
                    break;
                }
                default:
                    break;
            }
        }
    }
}
void SurfaceBufferImpl::WriteToIpcIo(IpcIo& io)
{
    WriteInt32(&io, bufferData_.handle.key);
    WriteUint64(&io, bufferData_.handle.phyAddr);
    WriteUint32(&io, bufferData_.handle.reserveFds);
    WriteUint32(&io, bufferData_.handle.reserveInts);
    WriteUint32(&io, bufferData_.size);
    WriteUint32(&io, bufferData_.usage);
    WriteUint32(&io, len_);
    WriteUint32(&io, extDatas_.size());
    if (!extDatas_.empty()) {
        std::map<uint32_t, ExtraData>::iterator iter;
        for (iter = extDatas_.begin(); iter != extDatas_.end(); ++iter) {
            uint32_t key = iter->first;
            ExtraData value = iter->second;
            WriteUint32(&io, key);
            WriteUint32(&io, value.type);
            switch (value.type) {
                case BUFFER_DATA_TYPE_INT_32:
                    WriteInt32(&io, *(reinterpret_cast<int32_t *>(value.value)));
                    break;
                case BUFFER_DATA_TYPE_INT_64:
                    WriteInt64(&io, *(reinterpret_cast<int64_t *>(value.value)));
                    break;
                default:
                    break;
            }
        }
    }
}

void SurfaceBufferImpl::CopyExtraData(SurfaceBufferImpl& buffer)
{
    len_ = buffer.len_;
    extDatas_ = buffer.extDatas_;
    buffer.extDatas_.clear();
}

void SurfaceBufferImpl::ClearExtraData()
{
    if (!extDatas_.empty()) {
        std::map<uint32_t, ExtraData>::iterator iter;
        for (iter = extDatas_.begin(); iter != extDatas_.end(); ++iter) {
            ExtraData value = iter->second;
            free(value.value);
            value.value = nullptr;
        }
        extDatas_.clear();
    }
}

SurfaceBufferImpl::~SurfaceBufferImpl()
{
    ClearExtraData();
    struct SurfaceBufferData bufferData = {{0}, 0, 0, 0, BUFFER_STATE_NONE, NULL};
    bufferData_ = bufferData;
}
}
