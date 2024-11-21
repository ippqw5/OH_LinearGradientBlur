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

// export function start(): void;
// export function stop(): void;
// export function changeShape(shapeIndex: number): void;

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