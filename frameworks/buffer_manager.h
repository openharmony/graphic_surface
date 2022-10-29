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

#ifndef GRAPHIC_LITE_BUFFER_MANAGER_H
#define GRAPHIC_LITE_BUFFER_MANAGER_H

#include <map>
#include "display_gralloc.h"
#include "surface_buffer_impl.h"
#include "surface_type.h"

namespace OHOS {
/**
 * @brief Buffer Manager abstract class. Provide allocate, free, map, unmap buffer attr ability.
 *        It needs vendor to adapte it. Default Hisi support shm and physical memory.
 */
class BufferManager {
public:
    /**
     * @brief Buffer Manager Shared Memory Single Instance.
     *        It depends kernel supported shm.
     * @returns BufferManager pointer.
     */
    static BufferManager* GetInstance();

    /**
     * @brief Buffer Manager Init.
     * @returns Whether Buffer manager init succeed or not.
     */
    bool Init();

    /**
     * @brief Allocate buffer for producer.
     * @param [in] size, alloc buffer size.
     * @param [in] usage, alloc buffer usage.
     * @returns buffer pointer.
     */
    SurfaceBufferImpl* AllocBuffer(uint32_t size, uint32_t usage);

    /**
     * @brief Allocate buffer for producer.
     * @param [in] width, alloc buffer width.
     * @param [in] height, alloc buffer height.
     * @param [in] format, alloc buffer format.
     * @param [in] usage, alloc buffer usage.
     * @returns buffer pointer.
     */
    SurfaceBufferImpl* AllocBuffer(uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    /**
     * @brief Free the buffer.
     * @param [in] SurfaceBufferImpl double pointer, free the buffer size.
     */
    void FreeBuffer(SurfaceBufferImpl** buffer);

    /**
     * @brief Flush the buffer.
     * @param [in] Flush SurfaceBufferImpl cache to physical memory.
     * @returns 0 is succeed; other is failed.
     */
    int32_t FlushCache(SurfaceBufferImpl& buffer) const;

    /**
     * @brief Map the buffer for producer.
     * @param [in] SurfaceBufferImpl, need to map.
     * @returns Whether map buffer succeed or not.
     */
    bool MapBuffer(SurfaceBufferImpl& buffer) const;

    /**
     * @brief Unmap the buffer, which producer could not writed data.
     * @param [in] SurfaceBufferImpl, need to unmap.
     */
    void UnmapBuffer(SurfaceBufferImpl& buffer) const;

protected:
    BufferHandle* AllocateBufferHandle(SurfaceBufferImpl& buffer) const;
    SurfaceBufferImpl* AllocBuffer(AllocInfo info);
    bool ConvertUsage(uint64_t& destUsage, uint32_t srcUsage) const;
    bool ConvertFormat(PixelFormat& destFormat, uint32_t srcFormat) const;

private:
    BufferManager() : grallocFucs_(nullptr) {}
    ~BufferManager() {}

    GrallocFuncs* grallocFucs_;
    struct BufferKey {
        int32_t key;
        uint64_t phyAddr;
        bool operator < (const BufferKey &x) const
        {
            return (key < x.key) || (key == x.key && phyAddr < x.phyAddr);
        }
    };
    std::map<BufferKey, BufferHandle*> bufferHandleMap_;
};
} // end namespace
#endif
