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

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import hilog from '@ohos.hilog';
import UIAbility from '@ohos.app.ability.UIAbility';
import Want from '@ohos.app.ability.Want';
import window from '@ohos.window';
import Logger from 'ets/util/Logger';
import fs from '@ohos.file.fs';

export default class EntryAbility extends UIAbility {

  image2sandbox(filename: string): void {
    try {
      Logger.info("copying image ")
      let dir: string = this.context.filesDir
      let filePath = dir + "/" + filename;
      Logger.info("copy filePath: " + filePath)
      this.context.resourceManager.getRawFileContent("rawfile/" + filename, (error, value) => {
        if (error != null) { //失败
          Logger.debug("copy fail: " + error.message)
        } else {
          let file = fs.openSync(filePath, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE)
          let writeLen = fs.writeSync(file.fd, value.buffer);
          Logger.debug("testTag-write data to file succeed and size is:" + writeLen)
          fs.closeSync(file)
        }
      })
    } catch (error) {
      Logger.debug("load error:")
    }
  }

  onWindowStageCreate(windowStage: window.WindowStage): void {
    // Main window is created, set main page for this ability
    Logger.info('testTag', '%{public}s', 'Ability onWindowStageCreate');

    this.image2sandbox("image.jpg");
    windowStage.loadContent('pages/Index', (err, data) => {
      if (err.code) {
        Logger.error('testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
        return;
      }
    });
  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
  }

  onForeground(): void {
    // Ability has brought to foreground
  }

  onBackground(): void {
    // Ability has back to background
  }
};
