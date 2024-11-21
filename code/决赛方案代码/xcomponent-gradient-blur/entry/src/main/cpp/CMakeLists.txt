# the minimum version of CMake.
cmake_minimum_required(VERSION 3.4.1)

# project
project(XComponent)

set(NATIVE_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# include directory
include_directories(${NATIVE_ROOT_PATH}
                    ${NATIVE_ROOT_PATH}/include
                    ${NATIVE_ROOT_PATH}/include/util)

# build shared library
add_library(nativeblur SHARED
            module.cpp
            app_napi.cpp
            napi_manager.cpp
            opengl_draw.cpp
            rectangle.cpp
            sat_shaders.cpp
)

# link OpenHarmony native library （Use OpenGL）
target_link_libraries(nativeblur PUBLIC EGL)
target_link_libraries(nativeblur PUBLIC GLESv3)
target_link_libraries(nativeblur PUBLIC hilog_ndk.z)
target_link_libraries(nativeblur PUBLIC ace_ndk.z)
target_link_libraries(nativeblur PUBLIC ace_napi.z)
target_link_libraries(nativeblur PUBLIC libc++.a)
target_link_libraries(nativeblur PUBLIC z)
target_link_libraries(nativeblur PUBLIC uv)
target_link_libraries(nativeblur PUBLIC libace_napi.z.so)
