//
// Created on 2024/10/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef SAT_SHADERS_H
#define SAT_SHADERS_H

#include <string>

class SAT {
public:
    static std::string vs;
    static std::string fs;
    static std::string fs_split2;
    static std::string scan_n0_s;
    static std::string scan_e0_s;
    static std::string scan_e0_s_vec3;
    static std::string scan_e0_s_l2;
    static std::string scan_e0_s_l4;
    static std::string scan_e0_s_l4_vec3;
    static std::string scan_e0_s_l8;
};
#endif // SAT_SHADERS_H
