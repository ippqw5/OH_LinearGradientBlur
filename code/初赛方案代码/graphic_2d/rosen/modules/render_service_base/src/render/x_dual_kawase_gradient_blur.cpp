#include "render/x_dual_kawase_gradient_blur.h"

#include "src/core/SkOpts.h"

#include "common/rs_optional_trace.h"
#include "platform/common/rs_log.h"

#ifdef USE_ROSEN_DRAWING
#include "draw/surface.h"
namespace OHOS {
namespace Rosen {
std::shared_ptr<Drawing::RuntimeEffect> DualKawaseGradientBlur::firstDownSampleShaderEffect_ = nullptr;
std::shared_ptr<Drawing::RuntimeEffect> DualKawaseGradientBlur::downSampleShaderEffect_ = nullptr;
std::shared_ptr<Drawing::RuntimeEffect> DualKawaseGradientBlur::upSampleShaderEffect_ = nullptr;
std::shared_ptr<Drawing::RuntimeEffect> DualKawaseGradientBlur::lastUpSampleShaderEffect_ = nullptr;

void DualKawaseGradientBlur::DrawDualKawaseGraidentBlur(const std::shared_ptr<Drawing::Image>& image,
    Drawing::Canvas& canvas, float radius, std::shared_ptr<Drawing::ShaderEffect> alphaGradientShader,
    const Drawing::Rect& dst)
{
    radius = std::clamp(radius / 2.0f, 0.0f, 30.0f) / 10.0f;
    if (radius < 0.1f) {
        return;
    }
    uint32_t numberOfPasses = std::max(std::min(3u, (uint32_t)ceil(radius)), (uint32_t)1);
    ROSEN_LOGI("DrawDualKawaseGraidentBlur pass: %{public}d", numberOfPasses);

    MakeFirstDownSampleEffect();
    MakeDownSampleEffect();
    MakeUpSampleEffect();
    MakeLastUpSampleEffect();

    auto skGradientShader = alphaGradientShader->ExportSkShader();
    alphaGradientShader->SetSkShader(
        skGradientShader->makeWithLocalMatrix(SkMatrix::Translate(-dst.GetLeft(), -dst.GetTop())));

    auto originImageInfo = image->GetImageInfo();
    // auto offsreenSurface = Drawing::Surface::MakeRenderTarget(canvas.GetGPUContext().get(), false,
    // image->GetImageInfo()); auto offscreenCanvas = offsreenSurface->GetCanvas();

    Drawing::Matrix matrix;
    float scale = 1.0f;
    int width = image->GetWidth();
    int height = image->GetHeight();
    std::shared_ptr<Drawing::Image> currentImage = image;

    // first downsample
    Drawing::RuntimeShaderBuilder firstDownSampleShaderBuilder(firstDownSampleShaderEffect_);
    scale /= 2.0f;
    width /= 2;
    height /= 2;
    firstDownSampleShaderBuilder.SetUniform("downscale", 1.0f / scale);
    firstDownSampleShaderBuilder.SetChild(
        "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                   Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));
    firstDownSampleShaderBuilder.SetChild("gradientShader", alphaGradientShader);
    auto scaledInfo = Drawing::ImageInfo(
        width, height, originImageInfo.GetColorType(), originImageInfo.GetAlphaType(), originImageInfo.GetColorSpace());
    currentImage = firstDownSampleShaderBuilder.MakeImage(canvas.GetGPUContext().get(), nullptr, scaledInfo, false);

    // downsample
    Drawing::RuntimeShaderBuilder downSampleShaderBuilder(downSampleShaderEffect_);
    for (uint32_t i = 1; i < numberOfPasses; i++) {
        scale /= 2.0f;
        width /= 2;
        height /= 2;
        downSampleShaderBuilder.SetUniform("downscale", 1.0f / scale);
        downSampleShaderBuilder.SetChild(
            "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                       Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));
        downSampleShaderBuilder.SetChild("gradientShader", alphaGradientShader);
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

    // upsample
    Drawing::RuntimeShaderBuilder upSampleShaderBuilder(upSampleShaderEffect_);
    for (uint32_t i = 0; i < numberOfPasses - 1; i++) {
        scale *= 2.0f;
        width *= 2;
        height *= 2;
        upSampleShaderBuilder.SetUniform("upscale", (float)(1.0 / scale));
        upSampleShaderBuilder.SetChild(
            "src", Drawing::ShaderEffect::CreateImageShader(*currentImage, Drawing::TileMode::CLAMP,
                       Drawing::TileMode::CLAMP, Drawing::SamplingOptions(Drawing::FilterMode::LINEAR), matrix));
        upSampleShaderBuilder.SetChild("gradientShader", alphaGradientShader);
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
    Drawing::RuntimeShaderBuilder lastUpSampleShaderBuilder(lastUpSampleShaderEffect_);
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

void DualKawaseGradientBlur::MakeFirstDownSampleEffect()
{
    static const std::string FirstDownSampleString(
        R"(
uniform shader src;
uniform shader gradientShader;
uniform half downscale;
half4 main(vec2 xy) 
{
    vec2 scaled_xy = xy * 2.0;  
    half gradient = gradientShader.eval(xy * downscale).a;
    if (gradient < 0.01) {
		return src.eval(scaled_xy);
	}

	half offset = 3.0;

    half4 sum = src.eval(scaled_xy) * 4.0;
    sum += src.eval(scaled_xy + vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0, 1.0) * offset);
	sum += src.eval(scaled_xy + vec2(1.0,-1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0,-1.0) * offset);
	return sum / 8.0;
}

    )");
    if (firstDownSampleShaderEffect_ == nullptr) {
        firstDownSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(FirstDownSampleString);
    }
}

void DualKawaseGradientBlur::MakeDownSampleEffect()
{
    static const std::string DownSampleString(
        R"(
uniform shader src;
uniform half downscale;
half4 main(vec2 xy) 
{
    vec2 scaled_xy = xy * 2.0;  

	half offset = 3.0;

    half4 sum = src.eval(scaled_xy) * 4.0;
    sum += src.eval(scaled_xy + vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0, 1.0) * offset);
	sum += src.eval(scaled_xy + vec2(1.0,-1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0,-1.0) * offset);
	return sum / 8.0;
}

    )");
    if (downSampleShaderEffect_ == nullptr) {
        downSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(DownSampleString);
    }
}

void DualKawaseGradientBlur::MakeUpSampleEffect()
{
    static const std::string UpSampleString(
        R"(
uniform shader src;
uniform half upscale;
half4 main(vec2 xy) 
{
    vec2 scaled_xy = (xy + 0.5)* 0.5;  

	half offset = 3.0;
    half4 sum = half4(0.0);
    sum += src.eval(scaled_xy + vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy + vec2(1.0,-1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0,-1.0) * offset);
    return sum / 4.0;
}
        )");
    if (upSampleShaderEffect_ == nullptr) {
        upSampleShaderEffect_ = Drawing::RuntimeEffect::CreateForShader(UpSampleString);
    }
}

void DualKawaseGradientBlur::MakeLastUpSampleEffect()
{
    static const std::string LastUpSampleString(
        R"(
uniform shader src;
uniform shader gradientShader;
half4 main(vec2 xy) 
{
    vec2 scaled_xy = (xy + 0.5) * 0.5;  

    half gradient = gradientShader.eval(xy).a;
    if (gradient < 0.01) {
		return half4(0.0);
	}
    
	half offset = 2.0;
    
    half4 sum = half4(0.0);
    sum += src.eval(scaled_xy + vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0, 1.0) * offset);
    sum += src.eval(scaled_xy + vec2(1.0,-1.0) * offset);
    sum += src.eval(scaled_xy - vec2(1.0,-1.0) * offset);
    sum /= 4.0;
	
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