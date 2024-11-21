# 方案1(初赛方案)

- [graphic_2d仓库链接](https://gitee.com/money-c/graphic_graphic_2d) (或者查看 `初赛方案代码/graphic_2d`)
  我们Fork了OpenHarmony的graphic_2d仓库，项目文件很多，但实际上我们只修改/新增了几个文件而已。你可以在  `qc_dev`分支下，查看最新的commit，找到我们修改和新增的文件。
- [arkui仓库链接](https://gitee.com/money-c/arkui_ace_engine)
  为了方便测试不同的渐变模糊算法，我们为前端接口 `LinearGradientBlurOptions`中新增一个字段 `BlurMethod`，默认值为 `BlurMethod::Gaussian`。在 `rs_linear_gradient_blur_filter.cpp`会根据前端传来的 `blurMethod` 参数，选择使用哪种渐变模糊方法。

```
declare enum BlurMethod {
    Gaussian, // OH4.1 已有算法
    DualKawase, // Ours
    DualGrainy, // Ours
    Box, // OH4.0 已有算法
    None
}
```

- [方案1 Demo仓库链接](https://gitee.com/money-c/demo-for-gradient-blur) (或者看 `初赛方案代码/demo-for-gradient-blur`)

# 方案2(决赛方案)

[方案2 仓库链接](https://gitee.com/money-c/xcomponent-gradient-blur.git) (或者看 `决赛方案代码/xcomponet-gradient-blur`)

本方案不需要编译系统源码。利用NAPI+Xcomponent，创建EGL和OpenGL ES环境，调用Computer Shader构造积分图SAT，基于SAT实现常数时间复杂度的渐变模糊效果。
