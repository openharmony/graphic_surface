/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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

#include "buffer_manager.h"

#include "buffer_common.h"
#include "securec.h"
#include "surface_buffer.h"

namespace OHOS {
BufferManager* BufferManager::GetInstance()
{
    static BufferManager instance;
    return &instance;
}

bool BufferManager::Init()
{
    if (grallocFucs_ != nullptr) {
        GRAPHIC_LOGI("BufferManager has init succeed.");
        return true;
    }
    if (GrallocInitialize(&grallocFucs_) != DISPLAY_SUCCESS) {
        return false;
    }
    return true;
}

bool BufferManager::ConversionUsage(uint64_t& destUsage, uint32_t srcUsage)
{
    switch (srcUsage) {
        case BUFFER_CONSUMER_USAGE_SORTWARE:
            destUsage = HBM_USE_MEM_SHARE;
            break;
        case BUFFER_CONSUMER_USAGE_HARDWARE:
        case BUFFER_CONSUMER_USAGE_HARDWARE_PRODUCER_CACHE:
            destUsage = HBM_USE_MEM_MMZ;
            break;
        case BUFFER_CONSUMER_USAGE_HARDWARE_CONSUMER_CACHE:
            destUsage = HBM_USE_MEM_MMZ_CACHE;
            break;
        default:
            GRAPHIC_LOGW("Conversion usage failed.");
            return false;
    }
    return true;
}

bool BufferManager::ConversionFormat(PixelFormat& destFormat, uint32_t srcFormat)
{
    switch (srcFormat) {
        case IMAGE_PIXEL_FORMAT_NONE:
            destFormat = PIXEL_FMT_BUTT;
            break;
        case IMAGE_PIXEL_FORMAT_RGB565:
            destFormat = PIXEL_FMT_RGB_565;
            break;
        case IMAGE_PIXEL_FORMAT_ARGB1555:
            destFormat = PIXEL_FMT_RGBA_5551;
            break;
        case IMAGE_PIXEL_FORMAT_RGB888:
            destFormat = PIXEL_FMT_RGB_888;
            break;
        case IMAGE_PIXEL_FORMAT_ARGB8888:
            destFormat = PIXEL_FMT_RGBA_8888;
            break;
        case IMAGE_PIXEL_FORMAT_NV12:
            destFormat = PIXEL_FMT_YCBCR_420_SP;
            break;
        case IMAGE_PIXEL_FORMAT_NV21:
            destFormat = PIXEL_FMT_YCRCB_420_SP;
            break;
        case IMAGE_PIXEL_FORMAT_YUV420:
            destFormat = PIXEL_FMT_YCBCR_420_P;
            break;
        case IMAGE_PIXEL_FORMAT_YVU420:
            destFormat = PIXEL_FMT_YCRCB_420_P;
            break;
        case IMAGE_PIXEL_FORMAT_YUYV:
        case IMAGE_PIXEL_FORMAT_YVYU:
        case IMAGE_PIXEL_FORMAT_VYUY:
        case IMAGE_PIXEL_FORMAT_AYUV:
        case IMAGE_PIXEL_FORMAT_UYVY:
        case IMAGE_PIXEL_FORMAT_YUV410:
        case IMAGE_PIXEL_FORMAT_YVU410:
        case IMAGE_PIXEL_FORMAT_YUV411:
        case IMAGE_PIXEL_FORMAT_YVU411:
        case IMAGE_PIXEL_FORMAT_YVU422:
        case IMAGE_PIXEL_FORMAT_YUV422:
        case IMAGE_PIXEL_FORMAT_YUV444:
        case IMAGE_PIXEL_FORMAT_YVU444:
        case IMAGE_PIXEL_FORMAT_NV16:
        case IMAGE_PIXEL_FORMAT_NV61:
        default:
            GRAPHIC_LOGW("Conversion format failed.");
            return false;
    }
    return true;
}

void BufferManager::SurfaceBufferToBufferHandle(const SurfaceBufferImpl& buffer, BufferHandle& graphicBuffer)
{
    graphicBuffer.key = buffer.GetKey();
    graphicBuffer.phyAddr = buffer.GetPhyAddr();
    graphicBuffer.size = buffer.GetSize();
    graphicBuffer.virAddr = buffer.GetVirAddr();
}

SurfaceBufferImpl* BufferManager::AllocBuffer(uint32_t width, uint32_t height, uint32_t format, uint32_t usage)
{
    RETURN_VAL_IF_FAIL((grallocFucs_ != nullptr), nullptr);
    uint64_t tempUsage;
    PixelFormat tempFormat;
    if (!ConversionUsage(tempUsage, usage)) {
        GRAPHIC_LOGW("Alloc graphic buffer failed --- conversion usage.");
        return nullptr;
    }
    if (!ConversionFormat(tempFormat, format)) {
        GRAPHIC_LOGW("Alloc graphic buffer failed --- conversion format.");
        return nullptr;
    }
    AllocInfo info = {
        .width = width,
        .height = height,
        .usage = tempUsage,
        .format = tempFormat
    };
    BufferHandle* bufferHandle = nullptr;
    if ((grallocFucs_->AllocMem == nullptr) || (grallocFucs_->AllocMem(&info, &bufferHandle) != DISPLAY_SUCCESS)) {
        GRAPHIC_LOGW("Alloc graphic buffer failed");
        return nullptr;
    }
    SurfaceBufferImpl* buffer = new SurfaceBufferImpl();
    if (buffer != nullptr) {
        buffer->SetMaxSize(bufferHandle->size);
        buffer->SetUsage(usage);
        buffer->SetVirAddr(bufferHandle->virAddr);
        buffer->SetKey(bufferHandle->key);
        buffer->SetPhyAddr(bufferHandle->phyAddr);
        bufferHandleMap_.insert(std::make_pair(bufferHandle->phyAddr, bufferHandle));
        GRAPHIC_LOGI("Alloc buffer succeed to shared memory segment.");
    } else {
        grallocFucs_->FreeMem(bufferHandle);
        GRAPHIC_LOGW("Alloc buffer failed to shared memory segment.");
    }
    return buffer;
}

void BufferManager::FreeBuffer(SurfaceBufferImpl** buffer)
{
    RETURN_IF_FAIL((grallocFucs_ != nullptr));
    if ((*buffer) == nullptr) {
        GRAPHIC_LOGW("Input param buffer is null.");
        return;
    }
    auto iter = bufferHandleMap_.find((*buffer)->GetPhyAddr());
    if (iter == bufferHandleMap_.end()) {
        return;
    }
    BufferHandle* bufferHandle = iter->second;
    if (grallocFucs_->FreeMem != nullptr) {
        grallocFucs_->FreeMem(bufferHandle);
        bufferHandleMap_.erase((*buffer)->GetPhyAddr());
        delete *buffer;
        *buffer = nullptr;
        GRAPHIC_LOGI("Free buffer succeed.");
    }
}

bool BufferManager::MapBuffer(SurfaceBufferImpl& buffer)
{
    RETURN_VAL_IF_FAIL((grallocFucs_ != nullptr), false);
    void* virAddr = NULL;
    BufferHandle bufferHandle;
    if (!ConversionUsage(bufferHandle.usage, buffer.GetUsage())) {
        GRAPHIC_LOGW("Conversion usage failed.");
        return false;
    }
    SurfaceBufferToBufferHandle(buffer, bufferHandle);
    if (buffer.GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE ||
        buffer.GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE_CONSUMER_CACHE ||
        buffer.GetUsage() == BUFFER_CONSUMER_USAGE_SORTWARE) {
        if (grallocFucs_->Mmap != nullptr) {
            virAddr = grallocFucs_->Mmap(&bufferHandle);
        }
    } else if (buffer.GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE_PRODUCER_CACHE) {
        if (grallocFucs_->MmapCache != nullptr) {
            virAddr = grallocFucs_->MmapCache(&bufferHandle);
        }
    } else {
        GRAPHIC_LOGW("No Suport usage.");
        return false;
    }
    if (virAddr == NULL) {
        GRAPHIC_LOGW("Map Buffer error.");
        return false;
    }
    buffer.SetVirAddr(virAddr);
    GRAPHIC_LOGW("Map Buffer succeed.");
    return true;
}

void BufferManager::UnmapBuffer(SurfaceBufferImpl& buffer)
{
    RETURN_IF_FAIL((grallocFucs_ != nullptr));
    BufferHandle bufferHandle;
    if (!ConversionUsage(bufferHandle.usage, buffer.GetUsage())) {
        GRAPHIC_LOGW("Conversion usage failed.");
        return;
    }
    SurfaceBufferToBufferHandle(buffer, bufferHandle);
    if ((grallocFucs_->Unmap == nullptr) || (grallocFucs_->Unmap(&bufferHandle) != DISPLAY_SUCCESS)) {
        GRAPHIC_LOGW("Umap buffer failed.");
    }
}

int32_t BufferManager::FlushCache(const SurfaceBufferImpl& buffer)
{
    RETURN_VAL_IF_FAIL((grallocFucs_ != nullptr), SURFACE_ERROR_NOT_READY);
    BufferHandle bufferHandle;
    if (!ConversionUsage(bufferHandle.usage, buffer.GetUsage())) {
        GRAPHIC_LOGW("Conversion usage failed.");
        return false;
    }
    SurfaceBufferToBufferHandle(buffer, bufferHandle);
    if (buffer.GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE_CONSUMER_CACHE) {
        if ((grallocFucs_->FlushCache == nullptr) || (grallocFucs_->FlushCache(&bufferHandle) != DISPLAY_SUCCESS)) {
            GRAPHIC_LOGW("Flush cache buffer failed.");
        }
    } else if (buffer.GetUsage() == BUFFER_CONSUMER_USAGE_HARDWARE_PRODUCER_CACHE) {
        if ((grallocFucs_->FlushMCache == nullptr) || (grallocFucs_->FlushMCache(&bufferHandle) != DISPLAY_SUCCESS)) {
            GRAPHIC_LOGW("Flush M cache buffer failed.");
        }
    }
    return SURFACE_ERROR_OK;
}
} // namespace OHOS