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

#ifndef APP_NAPI_H
#define APP_NAPI_H

#include <string>
#include <unordered_map>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <napi/native_api.h>
#include "opengl_draw.h"

#define CIRCUMFERENCE_DEGREE 360

class AppNapi {
public:
    explicit AppNapi(std::string &id);
    static AppNapi *GetInstance(std::string &id);
    static OH_NativeXComponent_Callback *GetNXComponentCallback();
    void SetNativeXComponent(OH_NativeXComponent *component);

public:
    // NAPI interface
    napi_value Export(napi_env env, napi_value exports);

    // Exposed to ArkTS/JS developers by NAPI
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Image(napi_env env, napi_callback_info info);
    static napi_value LinearGradientBlur(napi_env env, napi_callback_info info);
    
    // Callback, called by ACE XComponent
    void OnSurfaceCreated(OH_NativeXComponent *component, void *window);
    void OnSurfaceDestroyed(OH_NativeXComponent *component, void *window);
    
    OpenglDraw *GetOpenglDraw(void);

public:
    static std::unordered_map<std::string, AppNapi *> instance_;
    static OH_NativeXComponent_Callback callback_;
    static uint32_t isCreated_;

    void* window_;
    OH_NativeXComponent *component_;
    OpenglDraw* openglDraw_;
    std::string id_;
    uint64_t width_;
    uint64_t height_;
};

#endif // APP_NAPI_H
