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

#ifndef XCOMPONENTDEMO_SHADER_H
#define XCOMPONENTDEMO_SHADER_H

#include <sstream>
#include <GLES3/gl32.h>

#include "log.h"

class Shader {
public:
    unsigned int ID;
    // 构造函数动态生成着色器
    // ------------------------------------------------------------------------
    Shader(bool isText, const char *vShaderCode, const char *fShaderCode)
    {
        // 2. 编译着色器
        unsigned int vertex;
        unsigned int fragment;
        // 顶点着色器
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // 片段着色器
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // 如果给定了几何体着色器，请编译几何体着色器
        // 着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // 删除着色器，因为它们现在链接到着色器程序中，不再需要了
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    Shader(bool isText, const char* cShaderCode) {
        // 2. 编译着色器
        unsigned int compute;
        // 顶点着色器
        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &cShaderCode, NULL);
        glCompileShader(compute);
        checkCompileErrors(compute, "COMPUTE");
        // 着色器程序
        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // 删除着色器，因为它们现在链接到着色器程序中，不再需要了
        glDeleteShader(compute);
    }
    
    void use() { glUseProgram(ID); }
    
private:
    // 用于检查着色器编译/链接错误的实用函数。
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog); // glGetShaderInfoLog 1024
                LOGE("ERROR::SHADER_COMPILATION_ERROR of type: %{public}s ,  %{public}s", type.c_str(), infoLog);
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog); // glGetShaderInfoLog 1024
                LOGE("ERROR::PROGRAM_LINKING_ERROR of type: %{public}s ,  %{public}s", type.c_str(), infoLog);
            }
        }
    }
};

#endif // XCOMPONENTDEMO_SHADER_H
