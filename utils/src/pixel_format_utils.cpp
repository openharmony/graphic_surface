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

#include "pixel_format_utils.h"

namespace OHOS {
static struct {
    ImagePixelFormat pixelFormat;
    int16_t bpp;
} g_mapBpp[] = {
    {IMAGE_PIXEL_FORMAT_RGB565, 2},
    {IMAGE_PIXEL_FORMAT_ARGB1555, 2},
    {IMAGE_PIXEL_FORMAT_RGB888, 3},
    {IMAGE_PIXEL_FORMAT_ARGB8888, 4},
};

bool PixelFormatUtils::BppOfPixelFormat(ImagePixelFormat pixelFormat, int16_t& bpp)
{
    int16_t len = sizeof(g_mapBpp) / sizeof(g_mapBpp[0]);
    for (int16_t i = 0; i < len; i++) {
        if (pixelFormat == g_mapBpp[i].pixelFormat) {
            bpp = g_mapBpp[i].bpp;
            return true;
        }
    }
    return false;
}
}