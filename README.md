# WaterRower Link+

## Description
WaterRower Link+ is a bluetooth module designed for any WaterRower with the S4 monitor.  It provides all the functionality of the official WaterRower ComModule as well as many additional features.

---

# 简化版搭建指南（USB OTG 直连方案）

本仓库是 [nickbits1024/WaterRowerLinkPlus](https://github.com/nickbits1024/WaterRowerLinkPlus) 的简化改造版。
利用 ESP32-S3 开发板自带的 USB OTG 口直连 S4 显示器，**去掉了 LM3525 电源开关芯片、USB-A 母座、面包板、按钮**，
整机只需焊接 2 处（USB-OTG 跳线 + 一根信号线），由一个 5V UPS 电源开关统一控制开关机，非常适合上手。

## 配件清单

| 配件 | 说明 |
|---|---|
| ESP32-S3 开发板 | DevKitC-1 兼容板（双 Type-C 口，RGB 灯在 GPIO48；若为 v1.1 版需改 `STATE_LED_GPIO` 为 38） |
| 5V/3A UPS 升压模块 | 双 18650 供电、带充电和欠压保护 |
| 18650 电池 ×2 | |
| 自锁/拨动开关 | 串在 UPS 输出正极，控制整机开关机 |
| USB-C 公转 USB-A 母转接头 | 手机 OTG 转接头即可（USB 2.0 / 3.x 均可） |
| 3.5mm 三段插头（TRS）×1 | 接 S4 心率输入口 |
| 10K 电阻 ×1 | 心率脉冲信号串联限流 |
| 杜邦线若干 | 供电 + 心率信号 |

## 接线方式

```
                    ┌──────────────── ESP32-S3 开发板 ────────────────┐
  UPS 5V(经开关) ──►│ 5V 脚                                          │
  UPS GND ────────►│ G 脚                                            │
                    │                                                │
   [焊桥: USB-OTG] │ "USB" Type-C 口 ──► C转A转接头 ──► S4 原装USB线 │
                    │                                 (供电 + D+/D-)  │
                    │ GPIO15 ──[10K]──► 3.5mm 中间段(Ring)           │
                    └────────────────────────────────────────────────┘
                                      3.5mm ──► S4 心率输入口（Tip/Sleeve 不接）
```

| 连接 | 说明 |
|---|---|
| UPS 5V/GND → 开发板 5V/G | 整机供电；开关控制开关机，S4 随开发板同时上电 |
| 开发板 "USB" Type-C 口 → C转A转接头 → S4 | **先焊桥 "USB-OTG" 跳线**（短接该口 VBUS 上的二极管），5V 才能对外输出给 S4；D+/D− 板内直连 GPIO20/19，无需飞线 |
| GPIO15 → 10K → 3.5mm Ring（中间段） | 心率脉冲输出到 S4 屏幕；地回路走 USB 线，尖端(Tip)和根部(Sleeve) 悬空不接 |

**注意事项**
- USB 的 D−/D+ 在 ESP32-S3 上**固定为 GPIO19/GPIO20**（芯片内部直连 USB PHY），不可改到其它引脚
- 焊桥后 "USB" 口**永远不要插电脑或充电器**（会与 UPS 5V 直连冲突）；刷机/看日志用另一个 "COM" 口
- S4 与 ESP32 通过 USB 线共地，3.5mm 只需一根信号线

## 代码改造（相对上游）

| 文件 | 改动 |
|---|---|
| `main/s4.c` | 开机自动启动 S4 会话（不再等按钮）；移除 30 分钟无活动关断（VBUS 常通）；USB 断线自动重连不重启；关停流程改为 best-effort |
| `main/s4_int.h` | 心率脉冲输出 GPIO14 → **GPIO15**；GPIO8（原 LM3525 控制）/ GPIO10（原按钮）闲置；新增枚举竞态兜底（S4 先于固件就绪时主动查询设备列表） |
| `main/main.c` + `main/CMakeLists.txt` | 移除 ANT+ 模块驱动（无此硬件，释放 GPIO15/16/17/18） |
| `main/ble.c` + `main/ble_int.h` | 广播名改为 **"S4 01"**（官方 ComModule 命名格式，新版官方 App 只认此格式）；FTMS(0x1826) 加入服务 UUID 列表；广播间隔 5s → 200ms |
| `main/state_manager.c` + `main/state_manager_int.h` | RGB 灯定为 GPIO48；新增开机彩色自检（红→绿→蓝→白） |
| `sdkconfig` | 适配 ESP-IDF v5.4.4 重新生成 |

## 烧录

```bash
# 安装 ESP-IDF v5.4.x 后
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3   # 仓库已含 sdkconfig，可跳过
idf.py -p /dev/cu.usbserial-XXXX build flash monitor
```

## 使用流程

1. 拨动开关开机 → RGB 灯红绿蓝白扫过（自检）→ S4 屏幕亮起 → 绿灯常亮（S4 数据通道就绪）
2. 手机打开 WaterRower Connect 搜索 **"S4 01"** 连接（Zwift 等 FTMS App 同样可用）
3. 佩戴蓝牙心率带 → S4 屏幕显示心率，RGB 灯随心跳闪红
4. 划船数据实时同步到 App
5. 用完拨开关关机（无任何 flash 写入，随时断电安全）

---

*以下为上游原始项目文档（原始面包板 / PCB 方案，供参考）：*

![Components](docs/S4.jpg)

## Features
- Replaces WaterRower ComModule and External ANT+ Module
  - External ANT+ connection Works on M1 when using recommended cable (unlike the official ANT+ Plugin-In module!)
- Bluetooth
  - Bluetooth Fitness Machine Profile
    - Rower Data
    - Indoor Bike Data
  - Supports BLE HRMs
- ANT+
  - Fitness Equipment Profile
  - Supports ANT+ HRMs
- S4 monitor heart rate provider
  - Sends captured ANT+/BLE HR data for display on S4 monitor
- ANT+ to BLE HRM bridge
  - WRL+ will broadcast ANT+ HR data over BLE for use in apps that only support BLE. e.g. SuperNatural

## Tested Software
- WaterRower Connect
- Holofit
- VZFit
- ZWift
- Garmin Fenix 6
- SuperNatural (HR only)

![Components](docs/Components.png)

## Hardware
Hardware design is available in the KiCad projects in this repo.  There are two versions, [breadboard](hardware/breadboard/) and [PCB](hardware/pcb/).  With the breadboard version, everything can be assembled using dev boards and jumper wires. [^1]  Both solderless and soldered breadboards were tested and work.  The PCB version requires the design be sent to a PCB manufacturer.

### Soldered Breadboard Version
![Components](docs/BB_Assembled.jpg)
![Components](docs/BB_Disassembled.jpg)

### PCB Version
![Components](docs/PCB.jpg)

## Breadboard Bill of Materials
|Qty|Reference|Part|Foot Print|Supplier|
|---|---------|----|----------|--------|
|1|C1|1uF|THT||[Amazon](https://www.amazon.com/gp/product/B085RDTCCV)
|1|C2|0.1uF|THT||(See above)
|1|J1|AudioPlug3|2.54mm|[Amazon](https://www.amazon.com/gp/product/B07L3P93ZD)
|1|J4|USB_A|2.54mm|[Amazon](https://www.amazon.com/gp/product/B07MBSTZYG)
|2|R1, R2|10k|THT|[Amazon](https://www.amazon.com/gp/product/B072BL2VX1)
|1|SW2|Button|12mm|[Amazon](https://www.amazon.com/gp/product/B07F8GBWGG)
|1|U1|ESP32-S3-DevKitC-1|DIP|[Amazon](https://www.amazon.com/gp/product/B09CPNRDND)
|1|U2|LM3525M-H|SOIC-8|[Mouser](https://www.mouser.com/ProductDetail/926-LM3525MX-H-NOPB)
|1|U2|SOIC-8 TO DIP||[Amazon](https://www.amazon.com/gp/product/B07CJ96ZPW)
|1|U3|D52QD2M6IA[^2]|2x10_2.54mm|[Digi-Key](https://www.digikey.com/en/products/detail/garmin-canada-inc/D52QD2M6IA-A-TRAY/6149453)
|1||PCB Breadboard|50x100|[Amazon](https://www.amazon.com/gp/product/B07ZP4FYVJ)
|1|Project Box|B07W78LN1V|100 x 60 x 25|[Amazon](https://www.amazon.com/gp/product/B07W78LN1V)
|1|TRS Cable|CC0377|3.5mm|[Amazon](https://www.amazon.com/gp/product/B01K3WXQDW)
|1|USB Cable|3896|Mini USB|[Monoprice](https://www.monoprice.com/product?p_id=3896)


## PCB Bill of Materials
|Qty|Reference|Part|Foot Print|Supplier|
|---|---------|----|----------|--------|
|2|C1, C3|1uF cap|0603|[Amazon](https://www.amazon.com/gp/product/B06XDG3WQX)
|8|C2, C4, C5, C6, C9, C10, C14, C16|0.1uF cap|0603|(See above)
|2|C7, C11|C 10uF cap|0603|(See above)
|4|C8, C12, C13, C15|4.7uF cap|0603|(See above)
|1|D1|SK6812|5x5mm|[Digi-Key](https://www.digikey.com/en/products/detail/adafruit-industries-llc/1655/5154679)
|1|D2|1N5819|SOD-323|[Digi-Key](https://www.digikey.com/en/products/detail/micro-commercial-co/B5819WS-TP/2213555)
|1|D3|RED LED|0603|[Amazon](https://www.amazon.com/gp/product/B01CUGA380)
|1|J1|Molex_67643|USB-A|[Mouser](https://www.mouser.com/ProductDetail/538-67643-3910)
|1|J2|CUI_SJ-3523|3.5mm|[Mouser](https://www.mouser.com/ProductDetail/490-SJ-3523-SMT-TR)
|1|J3|Molex-105017-0001|Micro USB|[Mouser](https://www.mouser.com/ProductDetail/538-105017-0001)
|1|Q1|BSS138|SOT-23|[Mouser](https://www.mouser.com/ProductDetail/512-BSS138W)
|2|Q2, Q3|S8050|SOT-23|[Amazon](https://www.amazon.com/gp/product/B07KPCBP24)
|7|R1, R3, R4, R5, R6, R8, R11|10K res|0603|[Amazon](https://www.amazon.com/gp/product/B095P6L5PB)
|2|R2, R7|1K res|0603|(See above)
|1|R9|47K res|0603|(See above)
|1|R10|22K res|0603|(See above)
|1|R12|5.1K res|0603|(See above)
|1|SW2|Header|2x2.54mm|[Amazon](https://www.amazon.com/gp/product/B01461DQ6S) 
|1|SW2|Button|12mm|[Amazon](https://www.amazon.com/gp/product/B07F8GBWGG)
|1|SW4|RESET|2x2.54mm|(See above)
|1|SW1|RESET|6x6mm|[Amazon](https://www.amazon.com/gp/product/B077N3D7BD)
|1|SW3|BOOT|6x6mm|(See RESET)
|1|U1|ESP32-S3-WROOM-1|ESP32-S3-WROOM-1|[Mouser](https://www.mouser.com/ProductDetail/356-ESP32-S3WROOM1N8)
|1|U2|LM3525M-H|SOIC-8|[Mouser](https://www.mouser.com/ProductDetail/926-LM3525MX-H-NOPB)
|1|U3|D52Q|D52QD2M4IA|[Digi-Key](https://www.digikey.com/en/products/detail/garmin-canada-inc/D52QD2M4IA-TRAY/6149451)
|1|U4|CP2102N-Axx-xQFN24|QFN-24|[Digi-Key](https://www.digikey.com/en/products/detail/silicon-labs/CP2102N-A02-GQFN24/9863476)
|1|U5|AP7361C-33E|SOT-223|[Digi-Key](https://www.digikey.com/en/products/detail/diodes-incorporated/AP7361-33E-13/4249788)
|1|U6|SP0506BAATG|MSOP-8|[Mouser](https://www.mouser.com/ProductDetail/576-SP0506BAATG)
|1|Project Box|B07W78LN1V|100 x 60 x 25|[Amazon](https://www.amazon.com/gp/product/B07W78LN1V)
|1|TRS Cable|CC0377|3.5mm|[Amazon](https://www.amazon.com/gp/product/B01K3WXQDW)
|1|USB Cable|3896|Mini USB|[Monoprice](https://www.monoprice.com/product?p_id=3896)
|1|Light Pipe||RA|[Digi-Key](https://www.digikey.com/en/products/detail/dialight/51513620375F/9742144)

## Cable Connections
1. Attach mini USB cable to S4 monitor and WaterRower Link+
2. Attach 3.5mm cable to S4 monitor and WaterRower Link+.  The recommended cable will fit in the narrow gap of an M1 rower and the monitor.
2. Attach micro USB cable to USB power supply

![Components](docs/Connections.jpg)

### Required Software
- [Visual Studio Code](https://code.visualstudio.com/download)
- [Visual Studio Code Espressif IDF extension](https://marketplace.visualstudio.com/items?itemName=espressif.esp-idf-extension)
- [ESP-IDF](https://dl.espressif.com/dl/esp-idf/) Built using commit 0132079fa51f2c130513dd634e97ef9726f00e85

### Equipment Used
- Soldering Iron: [YIHUA 995D+](https://www.amazon.com/gp/product/B0919MS875)
- PCB Hot Plate: [MINIWARE MHP30M Mini Hot Plate](https://www.amazon.com/gp/product/B08R6XFPKR)
- PCB Holder: [Stickvise](https://www.stickvise.com/)
- Microscope: [AMScope SM745NTP-B](https://amscope.com/products/sm745ntp-b) + [DAB](https://amscope.com/products/DAB)

## Usage
Power is provided by a 5V source on the breadboard power rail (e.g. a 5V UPS/power bank with the USB-A VBUS hardwired to the rail).  Once power is provided, a red LED will illuminate on the board and should be detectable in apps.  The S4 monitor is powered directly over the USB connection and a session starts as soon as it enumerates.  Turning the main power switch off powers everything down.  When WRL+ is receiving data from the S4, the LED will turn cyan. When it detects a heart rate over BLE or ANT+, it will beat red.
Note the software only supports one HRM.  If multiple are detected, only the first one seen will be connected.

## Known Issues
- The last update to VZfit displays HR in the HUD however the value does not appear correct.  It may be a bug with WRL+ or VZfit.

## Credits
This project was inspired by [PiRowFlow](https://github.com/inonoob/pirowflo)

[^1]: The LM3525M must be soldered to the SOIC-8 TO DIP first
[^2]: This part is EOL at Digi-Key.  I do not know if there is a compatible replacement.