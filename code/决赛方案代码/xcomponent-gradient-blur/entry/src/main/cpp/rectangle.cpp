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

#include "rectangle.h"
#include "log.h"
#include <string>
#include <iostream>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "util/stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "util/stb_image_resize2.h"
#include "sat_shaders.h"


inline void CheckGLESInfo() {
    GLint maxWorkGroupInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
    LOGI("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: %{public}d", maxWorkGroupInvocations);

    GLint range[2];
    GLint precision;

    // 检查顶点着色器中的 highprecision 支持
    glGetShaderPrecisionFormat(GL_VERTEX_SHADER, GL_HIGH_FLOAT, range, &precision);
    LOGI("Vertex Shader highprecision: %{public}d", precision);

    // 检查片段着色器中的 highprecision 支持
    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
    if (precision > 0) {
        LOGI("Fragment Shader highprecision: %{public}d", precision);
    } else {
        LOGE("Fragment Shader highprecision is not supported.");
    }

    glGetShaderPrecisionFormat(GL_COMPUTE_SHADER, GL_HIGH_FLOAT, range, &precision);
    LOGI("Compute Shader highprecision: %{public}d", precision);
    
    GLint num = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &num);
    LOGI("Max Texture Size: %{public}d", num);
}

int32_t Rectangle::Init(int width, int height, std::string image)
{
    
    CheckGLESInfo();
    mWindowWidth = width, mWindowHeight = height;
    
    mSATConfig = BestSATConfig(mWindowWidth, mWindowHeight);
    
    glGenTextures(3, mImages);
    LoadImage(mImages, (sanboxFileDir + image).c_str(), mSATConfig);
    
    if (mSATConfig.split_num == 1)
        mDrawShader = LoadShader(SAT::vs, SAT::fs, mSATConfig.internalFormat);
    else 
        mDrawShader = LoadShader(SAT::vs, SAT::fs_split2, mSATConfig.internalFormat);
    
    mScan0 = LoadShader(mSATConfig.used_algorithm,mSATConfig.threads_num,mSATConfig.internalFormat);
    mScan1 = LoadShader(mSATConfig.used_algorithm,mSATConfig.threads_num,mSATConfig.internalFormat);
    
    glGenFramebuffers(1, &mFBO);

    // 创建VBO
    MakeLinearGradient();
    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mVertices.size(), mVertices.data(), GL_DYNAMIC_DRAW);

    // 创建VAO && 设置顶点属性指针
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return 0;
}

// 绘图
void Rectangle::Update()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    static bool isFirst = true;
    if (isFirst || mIsRealTimeSAT) {
        isFirst = false;
        ComputeSAT(); // 计算积分图
    }

    mDrawShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mImages[2]); // 积分图保存在mImages[2]
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mImages[0]); // 原图像保存在mImages[0]

    glUniform4f(glGetUniformLocation(mDrawShader->ID, "bias"), mSATConfig.bias[0], mSATConfig.bias[1],
                mSATConfig.bias[2], mSATConfig.bias[3]);
    glUniform2f(glGetUniformLocation(mDrawShader->ID, "resolution"), mWindowWidth, mWindowHeight);
    glUniform2f(glGetUniformLocation(mDrawShader->ID, "ori"), mSATConfig.origin_x, mSATConfig.origin_y);
    glUniform2f(glGetUniformLocation(mDrawShader->ID, "imagesize"), mSATConfig.resize_image_width, mSATConfig.resize_image_height);
    glUniform1f(glGetUniformLocation(mDrawShader->ID, "baseRadius"), mBlurOption.baseRadius);
    
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(GLfloat) * mVertices.size(), mVertices.data());
    
    // 绑定VAO并绘制
    glBindVertexArray(mVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, mVertices.size() / 3);
}

int32_t Rectangle::Quit(void)
{
    glFinish();
    glDeleteVertexArrays(1, &mVAO);
    glDeleteBuffers(1, &mVBO);
    glDeleteFramebuffers(1, &mFBO);
    glDeleteTextures(3, mImages);
    
    glDeleteProgram(mDrawShader->ID);
    glDeleteProgram(mScan0->ID);
    glDeleteProgram(mScan1->ID);

    delete mDrawShader;
    delete mScan0;
    delete mScan1;

    LOGE("Rectangle Quit success.");
    return 0;
}

void Rectangle::Dispatch(GLuint input_image, GLuint output_image, int stage, SplitOption splitOption,bool use_shared) {
    GLint program = 0;
    if (stage == 0) {
        program = mScan0->ID;
    } else if (stage == 1) {
        program = mScan1->ID;
    }
    assert(program != 0);

    glUseProgram(program);
    
    if (use_shared) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input_image);
        glUniform1i(glGetUniformLocation(program, "input_image"), 0);
    } else {
        glBindImageTexture(0, input_image, 0, GL_FALSE, 0, GL_READ_ONLY, mSATConfig.internalFormat);
    }

    glBindImageTexture(1, output_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, mSATConfig.internalFormat);

    int split_texture_size = mSATConfig.texture_size / mSATConfig.split_num;
    int group_num = split_texture_size / mSATConfig.element_num;

    for (int group_id = 0; group_id < group_num; group_id++) {
        glUniform1ui(glGetUniformLocation(program, "group_id"), group_id);
        glUniform2i(glGetUniformLocation(program, "origin"), splitOption.origin_x, splitOption.origin_y);
        glUniform2i(glGetUniformLocation(program, "direction"), splitOption.direction_x, splitOption.direction_y);
        if (stage == 0) {
            glUniform4f(glGetUniformLocation(program, "bias"), mSATConfig.bias[0], mSATConfig.bias[1],
                        mSATConfig.bias[2], mSATConfig.bias[3]);
        } else {
            glUniform4f(glGetUniformLocation(program, "bias"), 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glDispatchCompute(split_texture_size, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
    
    glUseProgram(0);
}

void Rectangle::ComputeSAT() {
    SplitOption splitOption;
    if (mSATConfig.split_num == 1) {
        splitOption = mSATConfig.splitOptions[0];
        Dispatch(mImages[0], mImages[1], 0, splitOption);
        Dispatch(mImages[1], mImages[2], 1, splitOption);
    } else if (mSATConfig.split_num == 2) {
        splitOption = mSATConfig.splitOptions[0];
        Dispatch(mImages[0], mImages[1], 0, splitOption);
        Dispatch(mImages[1], mImages[2], 1, splitOption);

        splitOption = mSATConfig.splitOptions[1];
        Dispatch(mImages[0], mImages[1], 0, splitOption);
        Dispatch(mImages[1], mImages[2], 1, splitOption);

        splitOption = mSATConfig.splitOptions[2];
        Dispatch(mImages[0], mImages[1], 0, splitOption);
        Dispatch(mImages[1], mImages[2], 1, splitOption);

        splitOption = mSATConfig.splitOptions[3];
        Dispatch(mImages[0], mImages[1], 0, splitOption);
        Dispatch(mImages[1], mImages[2], 1, splitOption);
    }
}

void Rectangle::MakeLinearGradient()  {
    auto &anchor = mBlurOption.anchors;
    std::vector<GLfloat> new_vertices;
    for (int i = 0; i < anchor.size() - 1; i++) {
        auto start = anchor[i];
        auto end = anchor[i + 1];
        // LOGE("start.pos=%{public}f start.w=%{public}f, end.pos=%{public}f end.w=%{public}f",
        // start.pos,start.weight, end.pos, end.weight);
        assert(start.pos < end.pos);
        if (mBlurOption.direction == GradientDirection::Horizontal) {
            new_vertices.push_back(start.pos);
            new_vertices.push_back(-1.0f);
            new_vertices.push_back(start.weight);

            new_vertices.push_back(end.pos);
            new_vertices.push_back(-1.0f);
            new_vertices.push_back(end.weight);

            new_vertices.push_back(start.pos);
            new_vertices.push_back(1.0f);
            new_vertices.push_back(start.weight);

            new_vertices.push_back(end.pos);
            new_vertices.push_back(1.0f);
            new_vertices.push_back(end.weight);
        } else {
            new_vertices.push_back(-1.0f);
            new_vertices.push_back(start.pos);
            new_vertices.push_back(start.weight);

            new_vertices.push_back(1.0f);
            new_vertices.push_back(start.pos);
            new_vertices.push_back(start.weight);

            new_vertices.push_back(-1.0f);
            new_vertices.push_back(end.pos);
            new_vertices.push_back(end.weight);

            new_vertices.push_back(1.0f);
            new_vertices.push_back(end.pos);
            new_vertices.push_back(end.weight);
        }
    }
    mVertices = std::move(new_vertices);
}

///////////////////////////////////////////////////////////////////////////

bool LoadImage(GLuint textureID[3], const char *path, SATConfig &satConfig) {
    int raw_image_width, raw_image_height, channels;
    stbi_set_flip_vertically_on_load(1);
    float *raw_data = stbi_loadf(path, &raw_image_width, &raw_image_height, &channels, 4);
    if (!raw_data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return false;
    }
    
    satConfig.SetRawImageSize(raw_image_width, raw_image_height);
    
    int image_width = satConfig.resize_image_width, image_height = satConfig.resize_image_height;
    float *resize_data = new float[image_width * image_height * 4];
    stbir_resize_float_linear(raw_data, raw_image_width, raw_image_height, 0, resize_data, image_width, image_height, 0,
                              STBIR_RGBA);

    // split_num = 1 or 2
    for (int i = 0; i < pow(4, satConfig.split_num - 1); i++) {
        float sum[4] = {0.0};
        float avg[4] = {0.0};

        int split_width = image_width / satConfig.split_num;
        int split_height = image_height / satConfig.split_num;
        int offset = i * split_width * split_height * 4;
        for (int j = 0; j < split_width * split_height; j++) {
            for (int k = 0; k < 4; k++) {
                sum[k] += resize_data[offset + j * 4 + k];
            }
        }
        for (int j = 0; j < 4; j++) {
            avg[j] = sum[j] / (split_width * split_height);
            satConfig.bias[4 * i + j] = avg[j]; //
        }
    }

    glBindTexture(GL_TEXTURE_2D, textureID[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, satConfig.internalFormat, satConfig.texture_size, satConfig.texture_size);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image_width, image_height, GL_RGBA, GL_FLOAT, resize_data);

    for (int i = 1; i < 3; i++) {
        glBindTexture(GL_TEXTURE_2D, textureID[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexStorage2D(GL_TEXTURE_2D, 1, satConfig.internalFormat, satConfig.texture_size, satConfig.texture_size);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if (raw_data)
        stbi_image_free(raw_data);
    if (resize_data)
        delete[] resize_data;
    return true;
}

////////////////////////////////////////////////////////////////////////////
SATConfig BestSATConfig(int window_width, int window_height) {
    int bigger = std::max(window_width, window_height);
    int downscale = 2, split_num = 1, local_num = 4;
    if (bigger > 256) {
        return SATConfig(window_width, window_height, downscale, split_num, local_num, GL_RGBA32F);
    }
    // <= 256
    downscale = 1;
    return SATConfig(256, 256, downscale, split_num, local_num, GL_RGBA32F);
}

SATConfig::SATConfig(int window_width, int window_height, int downscale, int split_num, int local_num,
          GLenum format) {
    
    this->window_width = window_width;
    this->window_height = window_height;
    this->downscale = downscale;
    this->split_num = split_num;
    this->local_num = local_num;
    this->internalFormat = format;
    //////////////////////////////////////
    if (local_num == 1) {
        used_algorithm = SAT::scan_e0_s;
        this->local_num = 2; // local = 1 or 2, their threads_num is same : element_num / 2
    }
    else if(local_num == 2) used_algorithm = SAT::scan_e0_s_l2;
    else if(local_num == 4) used_algorithm = SAT::scan_e0_s_l4;
    else if(local_num == 8) used_algorithm = SAT::scan_e0_s_l8;
}

void SATConfig::SetRawImageSize(int raw_image_width, int raw_image_height) {
    this->raw_image_width = raw_image_width;
    this->raw_image_height = raw_image_height;
    
    resize_image_width = raw_image_width / downscale;
    resize_image_height = raw_image_height / downscale;
    int texture_size0 = nearestPowerOfTwo(resize_image_height >= resize_image_width ? resize_image_height : resize_image_width);
    int texture_size1 = nearestPowerOfTwo(window_height >= window_width ? window_height : window_width) / downscale;
    texture_size = std::min(texture_size0, texture_size1);
    LOGE("texture size %{public}d", texture_size);
    
    float resize_scale = std::min((float)texture_size / resize_image_width, (float)texture_size / resize_image_height);
    while (resize_image_width > texture_size || resize_image_height > texture_size) {
        resize_image_width *= resize_scale;
        resize_image_height *= resize_scale;
    }

    element_num = texture_size / split_num;
    threads_num = element_num / local_num;
    LOGE("threads_num  %{public}d", threads_num);
    if (split_num == 1){
        origin_x = origin_y = 0;
        splitOptions[0] = {origin_x, origin_y, 1, 1};
    }
    else if (split_num == 2) {
        origin_x = origin_y = texture_size / split_num;
        splitOptions[0] = {origin_x - 1, origin_y - 1, -1, -1};
        splitOptions[1] = {origin_x - 1, origin_y, -1, 1};
        splitOptions[2] = {origin_x, origin_y - 1, 1, -1};
        splitOptions[3] = {origin_x, origin_y, 1, 1};
    }
}
