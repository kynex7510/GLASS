/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef _GLASS_BASE_READ_H
#define _GLASS_BASE_READ_H

#include "Base/Context.h"

void GLASS_read_colorBuffer(const FramebufferInfo* fb, GLint x, GLint y, size_t width, size_t height, RIPPixelFormat pixelFormat, u8* out);

#endif /* _GLASS_READ_H */