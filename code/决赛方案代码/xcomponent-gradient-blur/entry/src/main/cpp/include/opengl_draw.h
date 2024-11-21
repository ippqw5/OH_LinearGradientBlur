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

#ifndef OPENGLDRAW_H
#define OPENGLDRAW_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <cstdint>
#include <string>
#include "rectangle.h"

class OpenglDraw {
public:
    int32_t Init(EGLNativeWindowType window, int width, int height, std::string image);
    int32_t Quit(void);
    void Update(void);
    void Animate(void);
    void LinearGradientBlurOption(BlurOption);
    void RealTimeSAT(bool);

protected:
    int mWidth, mHeight;
    EGLNativeWindowType mEglWindow;
    EGLDisplay mEGLDisplay = EGL_NO_DISPLAY;
    EGLConfig mEGLConfig = nullptr;
    EGLContext mEGLContext = EGL_NO_CONTEXT;
    EGLContext mSharedEGLContext = EGL_NO_CONTEXT;
    EGLSurface mEGLSurface = nullptr;
    
    Rectangle *mRectangle = nullptr;
    
    std::string mImage = "";
};

#endif // OPENGLDRAW_H