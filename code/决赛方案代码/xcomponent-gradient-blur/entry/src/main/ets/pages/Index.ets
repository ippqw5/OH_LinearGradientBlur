import XComponentContext, { XGradientDirection } from '../interface/XComponentContext';
import displaySync from '@ohos.graphics.displaySync';
import Logger from '../util/Logger';


@Entry
@Component
struct Index {
  private xComponentContext: XComponentContext | undefined = undefined;
  @State private radius: number = 60.0;
  private blurEndPos: number = 0.9; // 0.6 ~ 0.9
  private direct: number = 1.0;
  private backDisplaySync: displaySync.DisplaySync | undefined = undefined;

  CreateDisplaySync() {
    let range : ExpectedFrameRateRange = { // 创建和配置帧率参数
      expected: 90, // 设置期望绘制帧率
      min: 90, // 配置帧率范围
      max: 120 // 配置帧率范围
    };

    let change = (intervalInfo: displaySync.IntervalInfo) => {
      this.blurEndPos += 0.01 * this.direct
      if (this.blurEndPos > 0.9) {
        this.blurEndPos = 0.9
        this.direct = -1.0
      }
      else if (this.blurEndPos < 0.6){
        this.blurEndPos = 0.6
        this.direct = 1.0
      }
      if (this.xComponentContext){
        this.xComponentContext.xLinearGradientBlur(
          this.radius, {xFractionStops:[[0, 0], [0, 0.5], [0.75, this.blurEndPos],[1, 1]], xDirection:XGradientDirection.Vertical}
        )
      }
    }

    this.backDisplaySync = displaySync.create(); // 创建DisplaySync实例
    this.backDisplaySync.setExpectedFrameRateRange(range);
    this.backDisplaySync.on("frame", change); // 订阅frame事件和注册订阅函数
  }

  build() {
    Stack({ alignContent: Alignment.Bottom })  {
      XComponent({ id: 'linearGradientBlur', type: 'surface', libraryname: 'nativeblur' })
        .onLoad((context) => {
          this.xComponentContext = context as XComponentContext;
          this.xComponentContext.image("image.jpg")
          this.xComponentContext.xLinearGradientBlur(
            this.radius, {xFractionStops:[[0, 0], [0, 0.5], [0.75, this.blurEndPos],[1, 1]], xDirection:XGradientDirection.Vertical}
          )
        })
        // .width('512px')
        // .height('512px')

      Flex({ direction: FlexDirection.Row }) {
        Row() {
          Column(){
            Text('End')
              .backgroundColor(Color.White)
            Radio({value:'End', group:'radioGroup1'}).checked(true)
              .height(50)
              .width(50)
              .backgroundColor(Color.White)
              .onClick((): void => {
                if (this.backDisplaySync) {
                  this.backDisplaySync.stop();
                }
              })
          }
          Column() {
            Text('Start')
              .backgroundColor(Color.White)
            Radio({ value: 'Start', group: 'radioGroup1' })
              .height(50)
              .width(50)
              .backgroundColor(Color.White)
              .onClick((): void => {
                if (this.backDisplaySync == undefined) {
                  this.CreateDisplaySync();
                }
                if (this.backDisplaySync) {
                  this.backDisplaySync.start();
                }
              })
          }
          Column() {
            Text("RT-SAT ON")
              .fontSize(8)
              .backgroundColor(Color.White)
            Radio({ value: 'On', group: 'radioGroup2'})
              .height(50)
              .width(50)
              .backgroundColor(Color.White)
              .onClick((): void => {
                if (this.xComponentContext) {
                  this.xComponentContext.start();
                }
              })
          }
          Column() {
            Text("RT-SAT OFF")
              .fontSize(8)
              .backgroundColor(Color.White)
            Radio({ value: 'OFF', group: 'radioGroup2'}).checked(true)
              .height(50)
              .width(50)
              .backgroundColor(Color.White)
              .onClick((): void => {
                if (this.xComponentContext) {
                  this.xComponentContext.stop();
                }
              })
          }
          Column() {
            Text("blur radius").fontColor(Color.White)
            Row() {
              Slider({
                value: 60,
                min:30,
                max:120,
                step: 10,
                style: SliderStyle.OutSet
              })
                .width('35%')
                .backgroundColor(Color.White)
                .showSteps(true)
                .onChange((value: number, mode: SliderChangeMode) => {
                  this.radius = value
                  if (this.xComponentContext){
                    this.xComponentContext.xLinearGradientBlur(
                      this.radius, {xFractionStops:[[0, 0], [0, 0.5], [0.75, this.blurEndPos],[1, 1]], xDirection:XGradientDirection.Vertical}
                    )
                  }
                })
              Text(this.radius.toFixed(0)).fontSize(12).fontColor(Color.White)
            }
          }


        }
      }
    }
  }
}