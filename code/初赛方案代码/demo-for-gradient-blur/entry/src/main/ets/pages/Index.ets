import displaySync from '@ohos.graphics.displaySync';


@Entry
@Component
struct Index {
  @State radius: number = 59.0;
  @State blurEndPos: number = 0.6; // between 0.6 - 9.0
  @State height_: number = 50;
  @State direct: number = 1.0;
  @State blurMethod: BlurMethod = BlurMethod.Gaussian;
  private backDisplaySync: displaySync.DisplaySync | undefined = undefined;


  CreateDisplaySync() {
    let range : ExpectedFrameRateRange = { // 创建和配置帧率参数
      expected: 90, // 设置期望绘制帧率为90hz
      min: 60, // 配置帧率范围
      max: 90 // 配置帧率范围
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
    }
    this.backDisplaySync = displaySync.create(); // 创建DisplaySync实例
    this.backDisplaySync.setExpectedFrameRateRange(range);
    this.backDisplaySync.on("frame", change); // 订阅frame事件和注册订阅函数
  }

  build() {
    Stack({ alignContent: Alignment.Bottom }) {
      Image($r('app.media.image'))
        .objectFit(ImageFit.Fill)
        .overlay('Fill', { align: Alignment.Bottom, offset: { x: 0, y: 0 } })
        .linearGradientBlur(this.radius, {
          fractionStops: [[0, 0], [0, 0.5], [0.75, this.blurEndPos], [1, 1]],
          direction: GradientDirection.Top,
          blurMethod: this.blurMethod // 新增的字段
        })

      Flex({ direction: FlexDirection.Row }) {
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
          Text('OH4.1 Gaussian')
            .fontSize(12)
            .backgroundColor(Color.White)
          Radio({ value: 'Gaussian', group: 'radioGroup2' }).checked(true)
            .height(50)
            .width(50)
            .backgroundColor(Color.White)
            .onClick((): void => {
              this.blurMethod = BlurMethod.Gaussian
            })
        }

        Column() {
          Text('OH4.0 Box')
            .fontSize(12)
            .backgroundColor(Color.White)
          Radio({ value: 'Box', group: 'radioGroup2' })
            .height(50)
            .width(50)
            .backgroundColor(Color.White)
            .onClick((): void => {
              this.blurMethod = BlurMethod.Box
            })
        }
        Column() {
          Text('DualKawase')
            .fontSize(12)
            .backgroundColor(Color.White)
          Radio({ value: 'DualKawase', group: 'radioGroup2' })
            .height(50)
            .width(50)
            .backgroundColor(Color.White)
            .onClick((): void => {
              this.blurMethod = BlurMethod.DualKawase
            })
        }
        Column() {
          Text('DualGrainy')
            .backgroundColor(Color.White)
            .fontSize(12)
          Radio({ value: 'DualGrainy', group: 'radioGroup2' })
            .height(50)
            .width(50)
            .backgroundColor(Color.White)
            .onClick((): void => {
              this.blurMethod = BlurMethod.DualGrainy
            })
        }
      }
    }
  }
}