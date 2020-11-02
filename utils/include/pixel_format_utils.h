/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
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

#ifndef GRAPHIC_LITE_PIXEL_FORMAT_UTILS
#define GRAPHIC_LITE_PIXEL_FORMAT_UTILS

#include <stdint.h>
#include "surface_type.h"

namespace OHOS {
union PF_ARGB1555
{
    struct {
        uint16_t blue : 5;
        uint16_t green : 5;
        uint16_t red : 5;
        uint16_t alpha : 1;
    };
    uint16_t full;
};

union PF_ARGB8888
{
    struct {
        uint16_t blue : 8;
        uint16_t green : 8;
        uint16_t red : 8;
        uint16_t alpha : 8;
    };
    uint16_t full;
};

class PixelFormatUtils {
public:
    static bool BppOfPixelFormat(ImagePixelFormat pixelFormat, int16_t& bpp);
    static uint16_t ARGB8888ToARGB1555(uint32_t color);
    static uint32_t ARGB1555ToARGB8888(uint16_t color);
};
}
#endif