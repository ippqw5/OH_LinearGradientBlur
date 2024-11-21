#include "render/x_dual_grainy_gradient_blur.h"

#include "src/core/SkOpts.h"

#include "common/rs_optional_trace.h"
#include "platform/common/rs_log.h"

#ifdef USE_ROSEN_DRAWING
#include "draw/surface.h"
namespace OHOS {
namespace Rosen {
std::shared_ptr<Drawing::RuntimeEffect> DualGrainyGradientBlur::downSampleShaderEffect_ = nullptr;
std::shared_ptr<Drawing::RuntimeEffect> DualGrainyGradientBlur::upSampleShaderEffect_ = nullptr;
std::shared_ptr<Drawing::RuntimeEffect> DualGrainyGradientBlur::lastUpSampleShaderEffect_ = nullptr;

void DualGrainyGradientBlur::DrawDualGrainyGraidentBlur(const std::shared_ptr<Drawing::Image>& image,
    Drawing::Canvas& canvas, float radius, std::shared_ptr<Drawing::ShaderEffect> alphaGradientShader,
    const Drawing::Rect& dst)
{
    radius = std::clamp(radius / 2.0f, 0.0f, 30.0f) / 10.0f;
    if (radius < 0.1f) {
        return;
    }
    uint32_t numberOfPasses = std::max(std::min(3u, (uint32_t)ceil(radius)), (uint32_t)1);
    ROSEN_LOGI("DrawDualGrainyGraidentBlur pass: %{public}d", numberOfPasses);

    MakeDownSampleEffect();
    MakeUpSampleEffect();
    MakeLastUpSampleEffect();

    auto skGradientShader = alphaGradientShader->ExportSkShader();
    alphaGradientShader->SetSkShader(
        skGradientShader->makeWithLocalMatrix(SkMatrix::Translate(-dst.GetLeft(), -dst.GetTop())));

    auto originImageInfo = image->GetImageInfo();
    // auto offsreenSurface = Drawing::Surface::MakeRenderTarget(canvas.GetGPUContext().get(), false,
    // image->GetImageInfo()); auto offscreenCanvas = offsreenSurface->GetCanvas();

    Drawing::Matrix matrix; // identity matrix
    float scale = 1.0f;
    float r = 6.0f;
    int width = image->GetWidth();
    int height = image->GetHeight();
    std::shared_ptr<Drawing::Image> currentImage = image;

    // down sample
    Drawing::RuntimeShaderBuilder downSampleShaderBuilder(downSampleShaderEffect_);
    for (uint32_t i = 0; i < numberOfPasses; i++) {
        width /= 2;
        height /= 2;
        r /= 2.0f;
        scale /= 2.0f;
        downSampleShaderBuilder.SetUniform("downscale", (float)(1.0f / scale));
        downSampleShaderBuilder.SetUniform("r", r);
        downSampleShaderBuilder.SetChild(
            "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                       Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));

        // Drawing::Brush brush;
        // brush.SetShaderEffect(downSampleShaderBuilder.MakeShader(nullptr, false));
        // offscreenCanvas->AttachBrush(brush);
        // offscreenCanvas->DrawRect(Drawing::Rect(0, 0, width, height));
        // offscreenCanvas->DetachBrush();
        // currentImage = offsreenSurface->GetImageSnapshot({0, 0, width, height});

        auto scaledInfo = Drawing::ImageInfo(width, height, originImageInfo.GetColorType(),
            originImageInfo.GetAlphaType(), originImageInfo.GetColorSpace());
        currentImage = downSampleShaderBuilder.MakeImage(canvas.GetGPUContext().get(), nullptr, scaledInfo, false);
    }

    // up sample
    Drawing::RuntimeShaderBuilder upSampleShaderBuilder(upSampleShaderEffect_);
    for (uint32_t i = 0; i < numberOfPasses - 1; i++) {
        width *= 2;
        height *= 2;
        r *= 2.0f;
        scale *= 2.0f;
        upSampleShaderBuilder.SetUniform("upscale", (float)(1.0f / scale));
        upSampleShaderBuilder.SetUniform("r", r);
        upSampleShaderBuilder.SetChild(
            "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                       Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));

        // Drawing::Brush brush;
        // brush.SetShaderEffect(upSampleShaderBuilder.MakeShader(nullptr, false));
        // offscreenCanvas->AttachBrush(brush);
        // offscreenCanvas->DrawRect(Drawing::Rect(0, 0, width, height));
        // offscreenCanvas->DetachBrush();
        // currentImage = offsreenSurface->GetImageSnapshot({0, 0, width, height});

        auto scaledInfo = Drawing::ImageInfo(width, height, originImageInfo.GetColorType(),
            originImageInfo.GetAlphaType(), originImageInfo.GetColorSpace());
        currentImage = upSampleShaderBuilder.MakeImage(canvas.GetGPUContext().get(), nullptr, scaledInfo, false);
    }

    // last upsample with blending
    width *= 2;
    height *= 2;
    r *= 2.0f;
    scale *= 2.0f;
    Drawing::RuntimeShaderBuilder lastUpSampleShaderBuilder(lastUpSampleShaderEffect_);
    lastUpSampleShaderBuilder.SetUniform("upscale", (float)(1.0f / scale));
    lastUpSampleShaderBuilder.SetUniform("r", r);
    lastUpSampleShaderBuilder.SetChild(
        "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                   Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));
    lastUpSampleShaderBuilder.SetChild("gradientShader", alphaGradientShader);

    Drawing::Brush brush;
    brush.SetShaderEffect(lastUpSampleShaderBuilder.MakeShader(nullptr, false));

    canvas.Save();
    canvas.Translate(dst.GetLeft(), dst.GetTop());
    canvas.AttachBrush(brush);
    canvas.DrawRect(Drawing::Rect(0, 0, dst.GetWidth(), dst.GetHeight()));
    canvas.DetachBrush();
    canvas.Restore();
}

void DualGrainyGradientBlur::MakeDownSampleEffect()
{
    static const std::string DownSampleString(
        R"(
        uniform shader src;
        uniform float downscale;
        uniform float r;

        half Rand(half2 n)
        {
        return sin(dot(n, half2(1233.224, 1743.335)));
        }

        half4 main(float2 xy) 
        {
            float2 scaled_xy = xy * 2.0;  

            half random = Rand(scaled_xy);
            half2 randomOffset = half2(0.0, 0.0);

            half4 sum = half4(0.0);
            random = fract(43758.5453 * random + 0.61432);
            randomOffset.x = (random - 0.5) * 2.0;
            random = fract(43758.5453 * random + 0.61432);
            randomOffset.y = (random - 0.5) * 2.0;
            sum += src.eval(scaled_xy + randomOffset * r);

            random = fract(43758.5453 * random + 0.61432);
            randomOffset.x = (random - 0.5) * 2.0;
            random = fract(43758.5453 * random + 0.61432);
            randomOffset.y = (random - 0.5) * 2.0;
            sum += src.eval(scaled_xy + randomOffset * r);

            return sum / 2.0;
        }
    )");
    if (downSampleShaderEffect_ == nullptr) {
        downSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(DownSampleString);
    }
}

void DualGrainyGradientBlur::MakeUpSampleEffect()
{
    static const std::string UpSampleString(
        R"(
        uniform shader src;
        uniform float upscale;
        uniform float r;

        half Rand(half2 n)
        {
        return sin(dot(n, half2(1233.224, 1743.335)));
        }

        half4 main(float2 xy) 
        {
            float2 scaled_xy = (xy + 0.5) * 0.5;  

            half random = Rand(scaled_xy);
            half2 randomOffset = half2(0.0, 0.0);

            half4 sum = half4(0.0,0.0,0.0,0.0);

            random = fract(43758.5453 * random + 0.61432);
            randomOffset.x = (random - 0.5) * 2.0;
            random = fract(43758.5453 * random + 0.61432);
            randomOffset.y = (random - 0.5) * 2.0;
            sum += src.eval(scaled_xy + randomOffset * r);

            return sum;
        }
        )");
    if (upSampleShaderEffect_ == nullptr) {
        upSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(UpSampleString);
    }
}

void DualGrainyGradientBlur::MakeLastUpSampleEffect()
{
    static const std::string LastUpSampleString(
        R"(
        uniform shader src;
        uniform shader gradientShader;
        uniform float upscale;
        uniform float r;

        half Rand(half2 n)
        {
        return sin(dot(n, half2(1233.224, 1743.335)));
        }

        half4 main(float2 xy) 
        {
            half gradient = gradientShader.eval(xy).a;
            float2 scaled_xy = (xy + 0.5) * 0.5; 

            half random = Rand(scaled_xy);
            half2 randomOffset = half2(0.0, 0.0);

            half4 sum = half4(0.0,0.0,0.0,0.0);

            random = fract(43758.5453 * random + 0.61432);
            randomOffset.x = (random - 0.5) * 2.0;
            random = fract(43758.5453 * random + 0.61432);
            randomOffset.y = (random - 0.5) * 2.0;
            sum += src.eval(scaled_xy + randomOffset * r);

            return half4(sum.rgb * gradient, gradient);
        }
        )");
    if (lastUpSampleShaderEffect_ == nullptr) {
        lastUpSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(LastUpSampleString);
    }
}
} // namespace Rosen
} // namespace OHOS
#endif