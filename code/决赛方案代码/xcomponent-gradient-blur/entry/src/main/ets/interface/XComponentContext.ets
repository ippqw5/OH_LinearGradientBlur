// interface/XComponentContext.ts
export type XFractionStop = [number, number];

export enum XGradientDirection {
  Vertical = 0,
  Horizontal
}

export interface XLinearGradientBlurOptions {
  xFractionStops: XFractionStop[];
  xDirection: XGradientDirection;
}

export default interface XComponentContext {
  start(): void; // 开始实时计算积分图
  stop(): void;  // 停止实时计算积分图
  image(rawFilename: string): void; // 上图rawfile图片
  xLinearGradientBlur(value: number, option: XLinearGradientBlurOptions): void; // 渐变模糊
}