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

#include <cstdint>
#include <string>
#include <cstdio>

#include "log.h"
#include "napi_manager.h"

enum class ContextType {
    APP_LIFECYCLE = 0,
    JS_PAGE_LIFECYCLE,
};

bool NapiManager::Export(napi_env env, napi_value exports)
{
    napi_status status;
    napi_value exportInstance = nullptr;
    OH_NativeXComponent *nativeXComponent = nullptr;
    int32_t ret;
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;

    status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance);
    if (status != napi_ok) {
        return false;
    }

    status = napi_unwrap(env, exportInstance, reinterpret_cast<void **>(&nativeXComponent));
    if (status != napi_ok) {
        return false;
    }

    ret = OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize);
    if (ret != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        return false;
    }

    std::string id(idStr);
    auto context = NapiManager::GetInstance();
    if (context) {
        context->SetNativeXComponent(id, nativeXComponent);
        auto app = context->GetApp(id);
        app->SetNativeXComponent(nativeXComponent);
        app->Export(env, exports);
        return true;
    }

    return false;
}

void NapiManager::SetNativeXComponent(std::string &id, OH_NativeXComponent *nativeXComponent)
{
    if (nativeXComponentMap_.find(id) == nativeXComponentMap_.end()) {
        nativeXComponentMap_[id] = nativeXComponent;
    } else {
        if (nativeXComponentMap_[id] != nativeXComponent) {
            nativeXComponentMap_[id] = nativeXComponent;
        }
    }
}

OH_NativeXComponent *NapiManager::GetNativeXComponent(std::string &id)
{
    if (nativeXComponentMap_.find(id) == nativeXComponentMap_.end()) {
        return nullptr;
    } else {
        return nativeXComponentMap_[id];
    }
}

AppNapi *NapiManager::GetApp(std::string &id)
{
    if (appNapiMap_.find(id) == appNapiMap_.end()) {
        AppNapi *instance = AppNapi::GetInstance(id);
        appNapiMap_[id] = instance;
        return instance;
    } else {
        return appNapiMap_[id];
    }
}
