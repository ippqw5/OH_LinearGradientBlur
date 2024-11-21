#ifndef DUAL_KAWASE_GRADIENT_BLUR_H
#define DUAL_KAWASE_GRADIENT_BLUR_H

#include "render/rs_gradient_blur_para.h"
#include "render/rs_skia_filter.h"

#ifdef USE_ROSEN_DRAWING
#include "effect/runtime_effect.h"
#include "effect/runtime_shader_builder.h"
namespace OHOS {
namespace Rosen {
class DualKawaseGradientBlur {
public:
    static void DrawDualKawaseGraidentBlur(const std::shared_ptr<Drawing::Image>& image, Drawing::Canvas& canvas,
        float radius, std::shared_ptr<Drawing::ShaderEffect> alphaGradientShader, const Drawing::Rect& dst);

    static void MakeFirstDownSampleEffect();
    static void MakeDownSampleEffect();
    static void MakeUpSampleEffect();
    static void MakeLastUpSampleEffect();

    static std::shared_ptr<Drawing::RuntimeEffect> firstDownSampleShaderEffect_;
    static std::shared_ptr<Drawing::RuntimeEffect> downSampleShaderEffect_;
    static std::shared_ptr<Drawing::RuntimeEffect> upSampleShaderEffect_;
    static std::shared_ptr<Drawing::RuntimeEffect> lastUpSampleShaderEffect_;
};
} // namespace Rosen
} // namespace OHOS
#endif
#endif
