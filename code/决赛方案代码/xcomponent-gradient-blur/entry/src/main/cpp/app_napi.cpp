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

#include <bits/alltypes.h>
#include <cstdint>
#include "napi_manager.h"
#include "native_common.h"
#include "opengl_draw.h"
#include "log.h"
#include "app_napi.h"

std::unordered_map<std::string, AppNapi *> AppNapi::instance_;
OH_NativeXComponent_Callback AppNapi::callback_;

uint32_t AppNapi::isCreated_ = 0;

inline std::string GetXComponentID(const char* functionName,napi_env env, napi_callback_info info){

    if ((env == nullptr) || (info == nullptr)) {
        LOGE("%{public}s: env or info is null", functionName);
        return "";
    }

    napi_value thisArg;
    if (napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr) != napi_ok) {
        LOGE("%{public}s: napi_get_cb_info fail", functionName);
        return "";
    }

    napi_value exportInstance;
    if (napi_get_named_property(env, thisArg, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
        LOGE("%{public}s: napi_get_named_property fail", functionName);
        return "";
    }

    OH_NativeXComponent *nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent)) != napi_ok) {
        LOGE("%{public}s: napi_unwrap fail", functionName);
        return "";
    }

    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        LOGE("%{public}s: Unable to get XComponent id", functionName);
        return nullptr;
    }
    return std::string(idStr);
    
}

OpenglDraw *AppNapi::GetOpenglDraw(void) {  return openglDraw_; }

static void OnSurfaceCreatedCB(OH_NativeXComponent *component, void *window)
{
    LOGE("OnSurfaceCreatedCB");
    int32_t ret;
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        return;
    }

    std::string id(idStr);
    auto instance = AppNapi::GetInstance(id);
    instance->OnSurfaceCreated(component, window);
}


static void OnSurfaceDestroyedCB(OH_NativeXComponent *component, void *window)
{
    int32_t ret;
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    ret = OH_NativeXComponent_GetXComponentId(component, idStr, &idSize);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        return;
    }
    std::string id(idStr);
    auto instance = AppNapi::GetInstance(id);
    instance->OnSurfaceDestroyed(component, window);
}


AppNapi::AppNapi(std::string &id)
{
    id_ = id;
    component_ = nullptr;
    auto appCallback = AppNapi::GetNXComponentCallback();
    appCallback->OnSurfaceCreated = OnSurfaceCreatedCB;
    appCallback->OnSurfaceDestroyed = OnSurfaceDestroyedCB;
    
    openglDraw_ = new OpenglDraw();
}

AppNapi *AppNapi::GetInstance(std::string &id)
{
    if (instance_.find(id) == instance_.end()) {
        AppNapi *instance = new AppNapi(id);
        instance_[id] = instance;
        return instance;
    } else {
        return instance_[id];
    }
}

OH_NativeXComponent_Callback *AppNapi::GetNXComponentCallback() 
{
    return &AppNapi::callback_; 
}

void AppNapi::SetNativeXComponent(OH_NativeXComponent *component)
{
    component_ = component;
    OH_NativeXComponent_RegisterCallback(component_, &AppNapi::callback_);
}

void AppNapi::OnSurfaceCreated(OH_NativeXComponent *component, void *window)
{
    LOGE("AppNapi::OnSurfaceCreated");
    window_ = window;
    int32_t ret = OH_NativeXComponent_GetXComponentSize(component, window, &width_, &height_);

    if (ret == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        isCreated_++;
        LOGE("AppNapi::OnSurfaceCreated success ");
    } else {
        LOGE("AppNapi::OnSurfaceCreated failed");
    }
}


void AppNapi::OnSurfaceDestroyed(OH_NativeXComponent *component, void *window)
{
    LOGE("AppNapi::OnSurfaceDestroyed");
    isCreated_--;
    openglDraw_->Quit();
    LOGE("AppNapi::OnSurfaceDestroyed iscreated %{public}d", isCreated_);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

napi_value AppNapi::Export(napi_env env, napi_value exports) {
    LOGE("AppNapi::Export");
    // Register NAPI
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("start", AppNapi::Start),
        DECLARE_NAPI_FUNCTION("stop", AppNapi::Stop), 
        DECLARE_NAPI_FUNCTION("image", AppNapi::Image),
        DECLARE_NAPI_FUNCTION("xLinearGradientBlur", AppNapi::LinearGradientBlur)};
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

napi_value AppNapi::Start(napi_env env, napi_callback_info info) {
    std::string id = GetXComponentID("Start", env, info);
    auto instance = AppNapi::GetInstance(id);
    LOGI("Start Real Time SAT");
    OpenglDraw* openglDraw = instance->GetOpenglDraw();
    if (openglDraw != nullptr) {
        openglDraw->RealTimeSAT(true);
    }
    return nullptr;
}

napi_value AppNapi::Stop(napi_env env, napi_callback_info info) {
    std::string id = GetXComponentID("Stop", env, info);
    auto instance = AppNapi::GetInstance(id);
    LOGI("Stop Real Time SAT");
    OpenglDraw* openglDraw = instance->GetOpenglDraw();
    if (openglDraw != nullptr) {
        openglDraw->RealTimeSAT(false);
    }
    return nullptr;
}

napi_value AppNapi::Image(napi_env env, napi_callback_info info) {
    LOGE("Image");
    size_t argc = 1;
    napi_value args[1] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    size_t strSize;
    char strBuf[256];
    auto ret = napi_get_value_string_utf8(env, args[0], strBuf, sizeof(strBuf), &strSize);
    if (ret != napi_ok) {
        LOGE("AppNapi::Image Fail to get image str");
        return nullptr;
    }
    std::string id = GetXComponentID("Stop", env, info);
    AppNapi* instance = AppNapi::GetInstance(id);
    OpenglDraw *openglDraw = instance->GetOpenglDraw();
    
    std::string image(strBuf, strSize);
    LOGI("AppNapi image filename: %{public}s", image.c_str());
    openglDraw->Init(reinterpret_cast<EGLNativeWindowType>(instance->window_), instance->width_, instance->height_, image);
    openglDraw->Update();
    return nullptr;
}
//////////////////////////////////////////////////////////////////////////

BlurOption ParseBlurOptions(napi_env env, napi_value obj) {
    BlurOption blurOption;
    blurOption.anchors.clear();
    // 解析xFractionStops
    napi_value stopsArray;
    napi_get_named_property(env, obj, "xFractionStops", &stopsArray);

    uint32_t length;
    napi_get_array_length(env, stopsArray, &length);

    for (uint32_t i = 0; i < length; ++i) {
        napi_value stop;
        napi_get_element(env, stopsArray, i, &stop);

        napi_value first, second;
        napi_get_element(env, stop, 0, &first);
        napi_get_element(env, stop, 1, &second);

        double weight, pos;
        napi_get_value_double(env, first, &weight);
        napi_get_value_double(env, second, &pos);

        pos = pos * 2.0 - 1.0;
        Anchor fractionStop = {static_cast<float>(pos), static_cast<float>(weight)};
        blurOption.anchors.push_back(fractionStop);
    }
    if (blurOption.anchors[0].pos >= -0.999) {
        Anchor firstStop = {-1.0, 0.0};
        blurOption.anchors.insert(blurOption.anchors.begin(), firstStop);
    }
    if (blurOption.anchors[blurOption.anchors.size() - 1].pos <= 0.999) {
        Anchor EndStop = {1.0, 1.0};
        blurOption.anchors.push_back(EndStop);
    }
    // 解析xDirection
    napi_value direction;
    napi_get_named_property(env, obj, "xDirection", &direction);

    int32_t directionValue;
    napi_get_value_int32(env, direction, &directionValue);

    blurOption.direction = static_cast<GradientDirection>(directionValue);

    return blurOption;
}

napi_value AppNapi::LinearGradientBlur(napi_env env, napi_callback_info info){

    size_t argc = 2;
    napi_value argv[2];
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

    if (argc != 2) {
        napi_throw_type_error(env, nullptr, "Expected two arguments");
        return nullptr;
    }

    // 获取参数
    double value;
    napi_get_value_double(env, argv[0], &value);
    value /= 2.0;
    napi_value optionsObj = argv[1];

    // 解析选项
    BlurOption option = ParseBlurOptions(env, optionsObj);
    option.baseRadius = value;

    // 在这里实现你的逻辑
    std::string id = GetXComponentID("LinearGradientBlur", env, info);
    LOGE("UpdateLinearGradientBlurOptions");
    OpenglDraw *openglDraw = AppNapi::GetInstance(id)->GetOpenglDraw();
    if (openglDraw) {
        openglDraw->LinearGradientBlurOption(option);
        openglDraw->Update();
    }
    
    return nullptr;
}
