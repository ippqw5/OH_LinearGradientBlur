//
// Created on 2024/10/5.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "include/sat_shaders.h"



std::string SAT::vs = R"(#version 320 es

layout (location = 0) in vec2 vPosition;
layout (location = 1) in float vGradient;

out float fGradient;

void main(void)
{
gl_Position = vec4(vPosition, 0.5, 1.0);
fGradient = vGradient;
}
)";



std::string SAT::fs = R"(#version 320 es

precision PRECISION float;
precision PRECISION sampler2D;

layout (binding = 0) uniform sampler2D sat;
layout (binding = 1) uniform sampler2D originalImage;

layout (location = 0) out vec4 color;

uniform vec4 bias;
uniform vec2 resolution;
uniform vec2 imagesize;
uniform vec2 ori;
uniform float baseRadius;

in float fGradient;

vec4 boxblur(vec2 C, float x, float y, vec2 texSize) {
vec2 P0 = C + vec2(-x, -y);
vec2 P1 = C + vec2(x, -y);
vec2 P2 = C + vec2(-x, y);
vec2 P3 = C + vec2(x, y);

vec4 a, b, c, d, res;


a = texture(sat, P0 / texSize); 
b = texture(sat, P1 / texSize); 
c = texture(sat, P2 / texSize); 
d = texture(sat, P3 / texSize); 
x *= 2.0;
y *= 2.0;

res = d - b - c + a;
return res / (x * y) + bias;
}

void main(void)
{
vec2 texSize = vec2(textureSize(sat, 0));

float m = baseRadius * fGradient;
if (m < 0.1f) { 
    color = texture(originalImage, gl_FragCoord.xy / resolution * imagesize / texSize) ;
    return;
}

vec2 C = (gl_FragCoord.xy / resolution) * (imagesize * 1.0f);

color = vec4(0.0);
color += boxblur(C, 0.5*m, m, texSize);
color += boxblur(C, m, 0.5*m, texSize);
color += boxblur(C, 0.75*m, 0.75*m, texSize);
color /= 3.0;
}
)";

std::string SAT::fs_split2 = R"(#version 320 es

precision PRECISION int;
precision PRECISION float;
precision PRECISION sampler2D;

layout (binding = 0) uniform sampler2D sat;
layout (binding = 1) uniform sampler2D originalImage;

layout (location = 0) out vec4 color;

uniform vec4 bias;
uniform vec2 resolution;
uniform vec2 ori;
uniform vec2 imagesize;
uniform float baseRadius;

in float fGradient;

vec4 box(vec2 C, float m, vec2 texSize) {
vec2 P0 = C + vec2(-m, -m);
vec2 P1 = C + vec2(m, -m);
vec2 P2 = C + vec2(-m, m);
vec2 P3 = C + vec2(m, m);

vec4 a, b, c, d, res;


a = texture(sat, P0 / texSize); 
b = texture(sat, P1 / texSize); 
c = texture(sat, P2 / texSize); 
d = texture(sat, P3 / texSize); 
m *= 2.0;
if (P3.x < ori.x && P3.y >= ori.y && P0.y < ori.y) {
    res = c - d + a - b;
    m += 0.5;
} 
else if (P0.x > ori.x &&  P0.y < ori.y && P3.y >= ori.y) {
    res = d - c + b - a;
    m += 0.5;
}
else if (P0.y > ori.y && P0.x < ori.x && P3.x >= ori.x) {
    res = d - b + c - a;
    m += 0.5;
}
else if (P3.y < ori.y && P3.x >= ori.x && P0.x < ori.x) {
    res = b - d + a - c;
    m += 0.5;
}
else if (P0.x < ori.x && P0.x < ori.y && P3.x >= ori.x && P3.y >= ori.y) {
    res = a + b + c + d;
    m += 1.0;
}
else if (P3.x < ori.x && P3.y < ori.y) {
    res = d - c - b + a;

}
else if (P0.x >= ori.x && P0.y >= ori.y) {
    res = a - b - c + d;
}
else {
    res = c - a - d + b;
}

return res / (m * m) + bias;
}


void main(void)
{
vec2 texSize = vec2(textureSize(sat, 0));

float m = 1.0 + baseRadius * fGradient;
vec2 C = ((gl_FragCoord.xy - 0.5f) / resolution) * (imagesize * 1.0f);

color = vec4(0.0);
color += box(C, clamp(m, 1.0, baseRadius), texSize);
}
)";


std::string SAT::scan_n0_s =
    R"(#version 320 es

precision highp int;
precision highp float;

layout (local_size_x = NUM_THREADS) in;

layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform highp sampler2D input_image;
layout (binding = 1, rgba32f) uniform writeonly highp image2D output_image;

shared vec4 shared_data[gl_WorkGroupSize.x * 2u];

void main(void)
{
uint thid = gl_LocalInvocationID.x;
uint group_offset = gl_WorkGroupSize.x * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_WorkGroupSize.x;

uint rd_id;
uint wr_id;
uint mask;

ivec2 P0 = origin + direction * ivec2(row, thid * 2u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 2u + 1u + group_offset);

shared_data[thid * 2u] = texelFetch(input_image, P0, 0) - bias; // texelFetch needs 0.25 cycle.
shared_data[thid * 2u + 1u] = texelFetch(input_image, P1, 0) - bias;

barrier();

const uint steps = uint(log2(float(gl_WorkGroupSize.x))) + 1u; // log2(512) + 1 = 10
uint step = 0u;

for (step = 0u; step < steps; step++) // 30 cycles
{
    mask = (1u << step) - 1u;
    rd_id = ((thid >> step) << (step + 1u)) + mask;
    wr_id = rd_id + 1u + (thid & mask);

    shared_data[wr_id] += shared_data[rd_id]; // Both shared memory's load/store need 1 cycle. Total 3 cycles.

    barrier();
    memoryBarrier();
}

imageStore(output_image, P0.yx, shared_data[thid * 2u]);
imageStore(output_image, P1.yx, shared_data[thid * 2u + 1u]);
}
)";

std::string SAT::scan_e0_s =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;

layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

shared vec4 shared_data[gl_WorkGroupSize.x * 2u];

void main(void)
{
uint thid = gl_LocalInvocationID.x;
uint group_offset = gl_WorkGroupSize.x * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_WorkGroupSize.x;

ivec2 P0 = origin + direction * ivec2(row, thid * 2u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 2u + 1u + group_offset);

shared_data[thid * 2u] = texelFetch(input_image, P0, 0) - bias;
shared_data[thid * 2u + 1u] = texelFetch(input_image, P1, 0) - bias;
barrier();

uint n = gl_WorkGroupSize.x * 2u;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

for (uint d = 2u; d < n; d <<= 1) {
    stride >>= 1;
    barrier();
    if (thid < d - 1u)
    {
        uint ai = stride * (thid + 1u) - 1u;
        uint bi = ai + (stride >> 1);
        shared_data[bi] += shared_data[ai];
    }
}

barrier();
imageStore(output_image, P0.yx, shared_data[thid * 2u]);
imageStore(output_image, P1.yx, shared_data[thid * 2u + 1u]);
}
)";

std::string SAT::scan_e0_s_vec3 =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;

layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

shared vec3 shared_data[gl_WorkGroupSize.x * 2u];

void main(void)
{
uint thid = gl_LocalInvocationID.x;
uint group_offset = gl_WorkGroupSize.x * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_WorkGroupSize.x;

ivec2 P0 = origin + direction * ivec2(row, thid * 2u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 2u + 1u + group_offset);

shared_data[thid * 2u] = (texelFetch(input_image, P0, 0) - bias).rgb;
shared_data[thid * 2u + 1u] = (texelFetch(input_image, P1, 0) - bias).rgb;
barrier();

uint n = gl_WorkGroupSize.x * 2u;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

for (uint d = 2u; d < n; d <<= 1) {
    stride >>= 1;
    barrier();
    if (thid < d - 1u)
    {
        uint ai = stride * (thid + 1u) - 1u;
        uint bi = ai + (stride >> 1);
        shared_data[bi] += shared_data[ai];
    }
}

barrier();
imageStore(output_image, P0.yx, vec4(shared_data[thid * 2u], 1.0));
imageStore(output_image, P1.yx, vec4(shared_data[thid * 2u + 1u], 1.0));
}
)";


std::string SAT::scan_e0_s_l2 =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;

layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

shared vec4 shared_data[gl_WorkGroupSize.x];

void main(void)
{
uint thid = gl_LocalInvocationID.x;
uint group_offset = gl_WorkGroupSize.x * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_WorkGroupSize.x;

ivec2 P0 = origin + direction * ivec2(row, thid * 2u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 2u + 1u + group_offset);

vec4 x0 = texelFetch(input_image, P0, 0) - bias;
vec4 x1 = x0 + texelFetch(input_image, P1, 0) - bias;
shared_data[thid] = x1;
barrier();

uint n = gl_WorkGroupSize.x;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

if (thid == 0u) shared_data[n - 1u] = vec4(0.0);

for (uint d = 1u; d < n; d *= 2u) {
    stride >>= 1;
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        vec4 t = shared_data[ai];
        shared_data[ai] = shared_data[bi];
        shared_data[bi] += t;
    }
}

barrier();
imageStore(output_image, P0.yx, shared_data[thid] + x0);
imageStore(output_image, P1.yx, shared_data[thid] + x1);
}
)";

std::string SAT::scan_e0_s_l4_vec3 =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;
layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

const uint workGroupSize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
shared vec3 shared_data[workGroupSize];

void main(void)
{
uint thid = gl_LocalInvocationIndex;
uint group_offset = workGroupSize * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * workGroupSize;

ivec2 P0 = origin + direction * ivec2(row, thid * 4u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 4u + 1u + group_offset);
ivec2 P2 = origin + direction * ivec2(row, thid * 4u + 2u + group_offset);
ivec2 P3 = origin + direction * ivec2(row, thid * 4u + 3u + group_offset);

vec3 x0 = (texelFetch(input_image, P0, 0) - bias).rgb;
vec3 x1 = x0 + (texelFetch(input_image, P1, 0) - bias).rgb;
vec3 x2 = x1 + (texelFetch(input_image, P2, 0) - bias).rgb;
vec3 x3 = x2 + (texelFetch(input_image, P3, 0) - bias).rgb;
shared_data[thid] = x3;
barrier();

uint n = workGroupSize;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

if (thid == 0u) shared_data[n - 1u] = vec3(0.0);

for (uint d = 1u; d < n; d *= 2u) {
    stride >>= 1;
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        vec3 t = shared_data[ai];
        shared_data[ai] = shared_data[bi];
        shared_data[bi] += t;
    }
}

barrier();
imageStore(output_image, P0.yx, vec4(shared_data[thid] + x0, 1.0));
imageStore(output_image, P1.yx, vec4(shared_data[thid] + x1, 1.0));
imageStore(output_image, P2.yx, vec4(shared_data[thid] + x2, 1.0));
imageStore(output_image, P3.yx, vec4(shared_data[thid] + x3, 1.0));
}
)";


std::string SAT::scan_e0_s_l4 =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;
layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

const uint workGroupSize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;
shared vec4 shared_data[workGroupSize];

void main(void)
{
uint thid = gl_LocalInvocationIndex;
uint group_offset = workGroupSize * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * workGroupSize;

ivec2 P0 = origin + direction * ivec2(row, thid * 4u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 4u + 1u + group_offset);
ivec2 P2 = origin + direction * ivec2(row, thid * 4u + 2u + group_offset);
ivec2 P3 = origin + direction * ivec2(row, thid * 4u + 3u + group_offset);

vec4 x0 = texelFetch(input_image, P0, 0) - bias;
vec4 x1 = x0 + texelFetch(input_image, P1, 0) - bias;
vec4 x2 = x1 + texelFetch(input_image, P2, 0) - bias;
vec4 x3 = x2 + texelFetch(input_image, P3, 0) - bias;
shared_data[thid] = x3;
barrier();

uint n = workGroupSize;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

if (thid == 0u) shared_data[n - 1u] = vec4(0.0);

for (uint d = 1u; d < n; d *= 2u) {
    stride >>= 1;
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        vec4 t = shared_data[ai];
        shared_data[ai] = shared_data[bi];
        shared_data[bi] += t;
    }
}

barrier();
imageStore(output_image, P0.yx, shared_data[thid] + x0);
imageStore(output_image, P1.yx, shared_data[thid] + x1);
imageStore(output_image, P2.yx, shared_data[thid] + x2);
imageStore(output_image, P3.yx, shared_data[thid] + x3);
}
)";


std::string SAT::scan_e0_s_l8 =
    R"(#version 320 es

precision PRECISION int;
precision PRECISION float;

layout (local_size_x = NUM_THREADS) in;

layout (location = 0) uniform uint group_id;
layout (location = 1) uniform vec4 bias;
layout (location = 2) uniform ivec2 origin;
layout (location = 3) uniform ivec2 direction;

layout (binding = 0) uniform PRECISION sampler2D input_image;
layout (binding = 1, FORMAT) writeonly uniform PRECISION image2D output_image;

shared vec4 shared_data[gl_WorkGroupSize.x];

void main(void)
{
uint thid = gl_LocalInvocationID.x;
uint group_offset = gl_WorkGroupSize.x * 2u * group_id;
uint row = gl_WorkGroupID.x + gl_WorkGroupID.y * gl_WorkGroupSize.x;

ivec2 P0 = origin + direction * ivec2(row, thid * 8u + group_offset);
ivec2 P1 = origin + direction * ivec2(row, thid * 8u + 1u + group_offset);
ivec2 P2 = origin + direction * ivec2(row, thid * 8u + 2u + group_offset);
ivec2 P3 = origin + direction * ivec2(row, thid * 8u + 3u + group_offset);
ivec2 P4 = origin + direction * ivec2(row, thid * 8u + 4u + group_offset);
ivec2 P5 = origin + direction * ivec2(row, thid * 8u + 5u + group_offset);
ivec2 P6 = origin + direction * ivec2(row, thid * 8u + 6u + group_offset);
ivec2 P7 = origin + direction * ivec2(row, thid * 8u + 7u + group_offset);

vec4 x0 = texelFetch(input_image, P0, 0) - bias;
vec4 x1 = x0 + texelFetch(input_image, P1, 0) - bias;
vec4 x2 = x1 + texelFetch(input_image, P2, 0) - bias;
vec4 x3 = x2 + texelFetch(input_image, P3, 0) - bias;
vec4 x4 = x3 + texelFetch(input_image, P4, 0) - bias;
vec4 x5 = x4 + texelFetch(input_image, P5, 0) - bias;
vec4 x6 = x5 + texelFetch(input_image, P6, 0) - bias;
vec4 x7 = x6 + texelFetch(input_image, P7, 0) - bias;
shared_data[thid] = x7;
barrier();

uint n = gl_WorkGroupSize.x;
uint stride = 1u;

for (uint d = n >> 1; d > 0u; d >>= 1) {
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        shared_data[bi] += shared_data[ai];
    }
    stride <<= 1;
}

if (thid == 0u) shared_data[n - 1u] = vec4(0.0);

for (uint d = 1u; d < n; d *= 2u) {
    stride >>= 1;
    barrier();
    if (thid < d)
    {
        uint ai = stride * (2u * thid + 1u) - 1u;
        uint bi = stride * (2u * thid + 2u) - 1u;

        vec4 t = shared_data[ai];
        shared_data[ai] = shared_data[bi];
        shared_data[bi] += t;
    }
}

barrier();
imageStore(output_image, P0.yx, shared_data[thid] + x0);
imageStore(output_image, P1.yx, shared_data[thid] + x1);
imageStore(output_image, P2.yx, shared_data[thid] + x2);
imageStore(output_image, P3.yx, shared_data[thid] + x3);
imageStore(output_image, P4.yx, shared_data[thid] + x4);
imageStore(output_image, P5.yx, shared_data[thid] + x5);
imageStore(output_image, P6.yx, shared_data[thid] + x6);
imageStore(output_image, P7.yx, shared_data[thid] + x7);
}
)";
