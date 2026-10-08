# c011f4p6_uart-firm
## 概要
このプロジェクトは、c011f4p6というチップに対し、pa4, pa5, pa6の入力の値をuartで送信するファームです。

## 制御について
### 通信
uartは、usart2を使用しており、PA2をtx, PA3をrxに設定しております。ちなみに書き込む際は、stlinkケーブルを使用し、pa14をdebug_swclk, pa13をdebug_swdioです。

### cubemxの設定
- Pinout ViewでPC14をRCC_OSCX_IN, PC15をRCC_OSCX_OUT, PA2をUSART2_TX, PA3をUSART2_RX, PA14をDEBUG_SWCLK, PA13をDEBUG_SWDIO, PA4,5,6をGPIO_Inputに設定
- ConnectivityのUSART2で、ModeをAsynchronousに設定、Parameter SettingsのBaud Rateを9600bpsに設定
- Trace and DebugでSerial Wireを有効にする
- Clock ConfigurationでInput frequencyを16Mhz, System Clock MuxをHSE, HLCKを16Mhzに設定してReslove Clock Issues

## 別branch(feature/gpio-or-output)について
このbranchでは、送信方式と送る値を変更したものとなっております。pa4, pa5, pa6の入力値を受け取り、その値からどれか1つでもfalseであればpa1から信号を送信するようになっています。

### cubemxの設定
元branchから追加でpa1のgpio設定のみ行います。
- Pinout ViewでPA1をGPIO_Ouputに設定
- System CoreのGPIOでGPIOタブを開き、PA1を選択し、GPIO output levelがlowになっていることを確認する
