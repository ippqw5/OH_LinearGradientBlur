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

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <cstdint>
#include "shader.h"

#define sanboxFileDir std::string("/data/storage/el2/base/haps/entry/files/")

enum class GradientDirection { Vertical = 0, Horizontal };

struct Anchor {
    float pos, weight;
};

struct BlurOption {
    float baseRadius = 30.0f;
    GradientDirection direction = GradientDirection::Vertical;
    std::vector<Anchor> anchors = {
        {-1.0f, 0.0f},
        {1.0f, 0.0f},
    };
};

inline int nearestPowerOfTwo(int value) {
    if (value <= 0)
        return 1; // 处理非正数情况

    // 如果value已经是2的幂次方，直接返回
    if ((value & (value - 1)) == 0)
        return value;

    // 找到大于或等于value的最小2的幂次方
    int upper = 1;
    while (upper < value) {
        upper <<= 1;
    }

    // 找到小于或等于value的最大2的幂次方
    int lower = upper >> 1;

    // 返回距离value最近的那个
    return (upper - value) < (value - lower) ? upper : lower;
}
struct SplitOption {
    int origin_x = 0, origin_y = 0;
    int direction_x = 0, direction_y = 0;
};

struct SATConfig {
    int window_width, window_height;
    int raw_image_width, raw_image_height;
    int resize_image_width, resize_image_height;
    int downscale;
    int split_num;
    int element_num;
    int local_num;
    GLenum internalFormat = GL_RGBA32F;
    
    std::string used_algorithm = "";
    int threads_num;
    int texture_size;

    int origin_x = 0, origin_y = 0;
    float bias[16] = {0.0f};
    SplitOption splitOptions[4];
    
    SATConfig() = default;
    SATConfig(int window_width, int window_height, int downscale = 1, int split_num = 1, int local_num = 4,
              GLenum format = GL_RGBA32F);
    void SetRawImageSize(int raw_image_width, int raw_image_height);
};

inline SATConfig BestSATConfig(int window_width, int window_height);

inline bool LoadImage(GLuint images[3], const char *path, SATConfig &satConfig);

class Rectangle {
public:
    int32_t Init(int width, int height, std::string);
    int32_t Quit(void);
    
    void Update(void);
    void Animate(void) {};
    void UpdateBlurOption(BlurOption blurOption) {
        mBlurOption = blurOption;
        MakeLinearGradient();
        
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * mVertices.size(), mVertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    void RealTimeSAT(bool is) {
        mIsRealTimeSAT = is;
    }

private:
    Shader *mDrawShader = nullptr;
    Shader *mScan0 = nullptr;
    Shader *mScan1 = nullptr;
    
    GLuint mVBO = 0;
    GLuint mVAO = 0;
    GLuint mFBO = 0;
    GLuint mImages[3] = {0};
    
    SATConfig mSATConfig;
    BlurOption mBlurOption;
    
    bool mIsRealTimeSAT = false;
    int mWindowWidth, mWindowHeight;

    std::vector<GLfloat> mVertices;
    
private:
    Shader* LoadShader(std::string vs, std::string fs, GLenum format = GL_RGBA32F) {
        if (format == GL_RGBA32F) {
            ReplacePlaceholder(fs, "PRECISION", "highp");
        }
        else if (format == GL_RGBA16F) {
            ReplacePlaceholder(fs, "PRECISION", "mediump");
        }
        return new Shader(false, vs.c_str(), fs.c_str());
    }
    
    Shader* LoadShader(std::string cs, int num_threads, GLenum format = GL_RGBA32F) {
        ReplacePlaceholder(cs, "NUM_THREADS", std::to_string(num_threads));
        if (format == GL_RGBA32F) {
            ReplacePlaceholder(cs, "PRECISION", "highp");
            ReplacePlaceholder(cs, "FORMAT", "rgba32f");
        } else if (format == GL_RGBA16F) {
            ReplacePlaceholder(cs, "PRECISION", "mediump");
            ReplacePlaceholder(cs, "FORMAT", "rgba16f");
        }

        return new Shader(false, cs.c_str());
    }
    
    void ComputeSAT();

    void Dispatch(GLuint input_image, GLuint output_image, int stage, SplitOption splitOption = {},
                          bool use_shared = true);

    void MakeLinearGradient();
    
    void ReplacePlaceholder(std::string &src, const std::string &placeholder,
                                    const std::string &value) {
        size_t pos = src.find(placeholder);
        while (pos != std::string::npos) {
            src.replace(pos, placeholder.length(), value);
            pos = src.find(placeholder, pos + value.length());
        }
    }
};

#endif // RECTANGLE_H
