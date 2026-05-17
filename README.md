# 智能家居 AI 语音助手

**当前版本：v1.0.1 (论文正式版本)**

基于 ESP32-S3 双核单片机的智能家居系统，融合了小智 AI 语音交互、传感器数据采集上传、以及 Android 移动端 APP 远程监控功能。

## 项目概述

本项目由硬件采集、AI 语音交互和移动端 APP 三部分协作完成：

| 模块 | 主控/平台 | 功能 |
|------|----------|------|
| **传感器采集** | STM32F103 | 读取温湿度、人体红外、CO 气体传感器数据，通过串口发送给 ESP32-S3 |
| **AI 语音 + 云端上传** | ESP32-S3 | 运行小智 AI 实现语音交互，同时接收 STM32 数据并上传至华为云 IoT 平台 |
| **移动端监控 APP** | Android (Kotlin + Jetpack Compose) | 通过华为云 IoTDA REST API 实时读取传感器数据，远程监控设备状态 |

> **说明**：移动端 APP 最初使用 Flutter 开发，后因 Flutter 的 MQTT 客户端无法正常连接华为云 IoT 平台，改为使用 Android Studio 原生开发（Kotlin + Jetpack Compose），通过 REST API 方式获取设备数据。

## 项目目录结构

```
Smart_Home_AI_Voice_Assistan/
├── README.md                          ← 项目说明文档（本文件）
├── APP配置参数.txt                     ← Android APP 所需的华为云配置参数（使用前必读）
├── 云平台配置参数.txt                   ← 华为云 IoT 平台配置参数
├── 华为云数据格式.txt                   ← 华为云物模型数据格式定义
├── ESP32-S3配置文件/                   ← ESP32-S3 编译所需的 sdkconfig 配置文件
│   └── sdkconfig                      ← 需在编译前复制到 xiaozhi-esp32 工程根目录
├── Smart_home_STM32/                  ← STM32F103 传感器采集工程
│   ├── Core/                          ← STM32 HAL 库核心代码
│   ├── MyApp/                         ← 传感器驱动（DHT11、HC-SR501、MQ-7、OLED、蜂鸣器）
│   ├── MDK-ARM/                       ← Keil 工程文件
│   └── Smart home.ioc                 ← STM32CubeMX 配置文件
├── xiaozhi-esp32-1.8.4-2026.4.20/    ← ESP32-S3 主工程（小智AI + 传感器 + 华为云）
│   └── main/
│       ├── stm32_uart.c/h            ← STM32 串口通信 + 数据上传任务
│       ├── huawei_cloud.c/h          ← 华为云 MQTT 客户端
│       ├── main.cc                   ← 启动 stm32_task
│       └── boards/bread-compact-wifi/
│           └── config.h              ← 板级引脚配置
└── smart_home_android/                ← Android 移动端 APP（智能家居监控）
    ├── app/src/main/java/com/smarthome/huawei/
    │   ├── MainActivity.kt            ← APP 主界面（Jetpack Compose）
    │   └── HuaweiIOT.kt              ← 华为云 IoTDA REST API 封装
    └── build.gradle                   ← 构建配置
```

## 系统总体架构

```
┌──────────────┐   UART   ┌──────────────────┐   MQTT   ┌──────────┐  REST API  ┌──────────────────┐
│  STM32F103   │ ───────→ │    ESP32-S3      │ ───────→ │  华为云   │ ←────────── │  Android APP     │
│  传感器采集   │  JSON    │  AI语音+云端上传  │  上传    │  IoT平台  │  读取数据   │  移动端监控      │
│  温湿度/CO/  │          │                  │          │  IoTDA   │            │  实时数据展示     │
│  人体红外     │          └──────────────────┘          └──────────┘            └──────────────────┘
└──────────────┘
```

系统工作流程：

1. **STM32F103** 每秒采集传感器数据并通过串口发送 JSON 给 ESP32-S3
2. **ESP32-S3** 通过 MQTT 将数据上传至华为云 IoT 平台，同时运行小智 AI 语音交互
3. **Android APP** 通过华为云 IoTDA REST API 读取设备影子数据，实时展示传感器状态

## 双核并行架构

ESP32-S3 是一颗**双核单片机**（Xtensa LX7，240MHz），搭载 FreeRTOS 实时操作系统。本项目充分利用双核特性，实现语音交互与传感器数据上传的**真正并行处理**：

```
┌─────────────────────────────────────────────────┐
│                  ESP32-S3 双核                    │
│                                                   │
│   Core 0                          Core 1         │
│  ┌──────────────────┐        ┌──────────────────┐ │
│  │ 小智 AI 主逻辑    │        │ 麦克风音频采集    │ │
│  │ WiFi/网络通信     │        │ 语音唤醒词检测    │ │
│  │ 串口读取 STM32    │        │ Opus 音频编解码   │ │
│  │ 华为云 MQTT 上传  │        │ 扬声器音频输出    │ │
│  └──────────────────┘        └──────────────────┘ │
│                                                   │
│   ┌─────────────────────────────────────────┐     │
│   │           FreeRTOS 任务调度              │     │
│   │  任务名        优先级  绑核     功能      │     │
│   │  audio_input    8     Core 1  麦克风采集  │     │
│   │  stm32_task     5     Core 0  串口+MQTT  │     │
│   │  audio_output   3     -       扬声器输出  │     │
│   │  opus_codec     2     -       音频编解码  │     │
│   │  主事件循环     3     -       AI状态机    │     │
│   └─────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────┘
```

- 语音任务优先级高，确保实时响应
- 传感器上传任务大部分时间处于阻塞等待状态，不占用 CPU 资源
- 两个核心各司其职，互不干扰

## 硬件清单

### ESP32-S3 外设与接线

#### ESP32-S3 引脚汇总表

| GPIO | 方向 | 功能 | 连接目标 | 供电 |
|------|------|------|---------|------|
| GPIO0 | 输入 | BOOT 按键 | 按键（低电平有效） | - |
| GPIO4 | 输出 | I2S 麦克风 WS | 麦克风 Word Select | - |
| GPIO5 | 输出 | I2S 麦克风 SCK | 麦克风 Serial Clock | - |
| GPIO6 | 输入 | I2S 麦克风 SD | 麦克风 Data Out | - |
| GPIO7 | 输出 | I2S 功放 DIN | 功放 Data In | - |
| GPIO8 | 输出 | UART1 TX | → STM32 PB11 (USART3 RX) | - |
| GPIO9 | 输入 | UART1 RX | ← STM32 PB10 (USART3 TX) | - |
| GPIO15 | 输出 | I2S 功放 BCLK | 功放 Bit Clock | - |
| GPIO16 | 输出 | I2S 功放 LRCK | 功放 Left/Right Clock | - |
| GPIO18 | 输出 | 灯控输出 | 外接灯/继电器 | - |
| GPIO39 | 输入 | 音量减按键 | 按键 | - |
| GPIO40 | 输入 | 音量加按键 | 按键 | - |
| GPIO41 | I2C SDA | OLED 显示屏 | SSD1306 SDA | 3.3V |
| GPIO42 | I2C SCL | OLED 显示屏 | SSD1306 SCL | 3.3V |
| GPIO47 | 输入 | 触摸按键 | 触摸传感器 | - |
| GPIO48 | 输出 | WS2812 RGB LED | 板载 RGB LED | - |

#### I2S 数字麦克风（I2S Simplex 输入）

| 麦克风引脚 | ESP32-S3 GPIO | 说明 |
|-----------|---------------|------|
| WS（字选择） | GPIO4 | I2S Word Select |
| SCK（时钟） | GPIO5 | I2S Serial Clock |
| SD（数据） | GPIO6 | I2S Data In（DIN） |
| L/R（声道选择） | 接 GND | 选择右声道（低电平有效） |
| VDD | 接 3.3V | 电源 |
| GND | 接 GND | 地 |

> 采样率：16000 Hz，单声道输入

#### I2S 功放模块（I2S Simplex 输出）

| 功放引脚 | ESP32-S3 GPIO | 说明 |
|---------|---------------|------|
| DIN（数据输入） | GPIO7 | I2S Data Out（DOUT） |
| BCLK（位时钟） | GPIO15 | I2S Bit Clock |
| LRCK（左右声道） | GPIO16 | I2S Left/Right Clock |
| SD_MODE | 接 3.3V | 芯片使能 + 选择左声道（高电平=正常工作） |
| GAIN | 接 GND | 增益选择（9dB） |
| VIN | 接 3.3V | 功放电源（音量较 5V 偏小，语音播放够用） |
| GND | 接 GND | 地 |

> 采样率：24000 Hz。注意：麦克风和功放使用独立的 I2S 实例（Simplex 模式），非全双工

#### OLED 显示屏（I2C）

| OLED 引脚 | ESP32-S3 GPIO | 说明 |
|-----------|---------------|------|
| SDA | GPIO41 | I2C 数据线 |
| SCL | GPIO42 | I2C 时钟线 |
| VDD | 接 3.3V | 电源 |
| GND | 接 GND | 地 |

> 型号：SSD1306，128x32（可通过 sdkconfig 切换 128x64），I2C 地址 0x3C，X/Y 轴镜像显示

#### 按键与 LED

| 外设 | 引脚 | 说明 |
|------|------|------|
| BOOT 按键 | GPIO0 | 低电平有效，首次烧录需按下 |
| 触摸按键 | GPIO47 | 电容触摸 |
| 音量+ | GPIO40 | 按键输入 |
| 音量- | GPIO39 | 按键输入 |
| WS2812 RGB LED | GPIO48 | 板载可编程 RGB 指示灯 |
| 灯控输出 | GPIO18 | 外接灯光控制 |

#### STM32 串口通信（UART1）

| ESP32-S3 | 方向 | STM32F103 | 说明 |
|----------|------|-----------|------|
| GPIO8 (TX) | → | PB11 (USART3 RX) | ESP32 发送，STM32 接收 |
| GPIO9 (RX) | ← | PB10 (USART3 TX) | STM32 发送，ESP32 接收 |
| GND | ─ | GND | 共地（必须连接） |

> 波特率 115200，8N1，缓冲区 1024 字节。GPIO8/9 避开了麦克风引脚 GPIO4/5/6

### STM32F103 外设

| 外设 | 型号 | 引脚 |
|------|------|------|
| 温湿度传感器 | DHT11 | PA8 |
| 人体红外传感器 | HC-SR501 | PA0 |
| CO 气体传感器 | MQ-7 | PA1 (ADC) |
| OLED 显示屏 | SSD1306 | PB8(SCL), PB9(SDA) |
| 蜂鸣器 | 有源蜂鸣器 | PB12 |
| ESP32 串口 | USART3 | PB10(TX), PB11(RX) |

## 接线方式

STM32 与 ESP32-S3 通过串口通信，接线如下：

```
STM32F103                ESP32-S3
PB10 (USART3 TX) ────→  GPIO9  (UART1 RX)
PB11 (USART3 RX)  ←──── GPIO8  (UART1 TX)
GND               ──────── GND
```

串口参数：波特率 115200，8 位数据位，无校验，1 位停止位。

## 传感器数据格式

STM32 每秒通过串口发送一次 JSON 数据，格式如下：

```json
{
  "services": [{
    "service_id": "Smart_Home",
    "properties": {
      "temp": 25,
      "humi": 60,
      "mq-7": 1234,
      "ppm": 12,
      "hc_sr_501": true,
      "people": "有人",
      "warning": "正常",
      "beep": false
    }
  }]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `temp` | int | 温度（摄氏度） |
| `humi` | int | 湿度（%RH） |
| `mq-7` | int | CO 传感器 ADC 原始值（0-4095） |
| `ppm` | int | CO 浓度估算值（ppm） |
| `hc_sr_501` | bool | 人体红外检测（true=有人） |
| `people` | string | "有人" 或 "无人" |
| `warning` | string | "正常" 或 "超标"（ADC>2000） |
| `beep` | bool | 蜂鸣器状态 |

## 系统工作流程

```
STM32F103                          ESP32-S3                           华为云                        Android APP
┌─────────┐                     ┌──────────────────┐              ┌──────────┐                ┌──────────────┐
│ 传感器   │                     │                  │              │          │                │              │
│ 采集    │──UART3(PB10)────→  │ UART1(GPIO9)     │              │          │  REST API      │  实时数据展示  │
│(每1秒)  │   JSON数据          │      ↓           │              │  IoT     │ ──────────────→│  温度/湿度/CO │
│         │                     │ stm32_task 读取   │              │  Device  │  读取设备影子   │  人体红外状态  │
│ OLED    │                     │      ↓           │              │  Hub     │                │              │
│ 显示    │                     │ MQTT 发布(每1秒)  │──MQTT(1883)─→│          │                │  连接状态监控  │
│ 蜂鸣器  │                     │                  │   传感器数据  │          │                │              │
│ 报警    │                     │  ─ ─ ─ ─ ─ ─ ─  │              │          │                │              │
│         │                     │                  │              │          │                │              │
│         │                     │ 小智 AI 语音交互  │              │          │                │              │
│         │                     │ 麦克风→ASR→LLM→  │              │          │                │              │
│         │                     │ TTS→扬声器       │              │          │                │              │
└─────────┘                     └──────────────────┘              └──────────┘                └──────────────┘
```

## Android 移动端 APP

### 功能概览

APP 通过华为云 IoTDA REST API 读取设备影子数据，实时展示传感器状态：

| 功能模块 | 说明 |
|----------|------|
| **连接状态** | 显示与华为云的连接状态（连接中/已连接/连接失败） |
| **实时数据展示** | 展示温度、湿度、CO 浓度（ADC/PPM）、人体红外、人员状态、警报状态、蜂鸣器状态 |
| **数据颜色指示** | 传感器数值根据阈值自动变色（正常绿色/警告橙色/超标红色） |

### 技术架构

| 技术栈 | 选型 | 用途 |
|--------|------|------|
| 开发语言 | Kotlin | Android 原生开发 |
| UI 框架 | Jetpack Compose + Material 3 | 声明式 UI 界面 |
| 网络通信 | HttpURLConnection + 华为云 REST API | 通过设备影子读取传感器数据 |
| JSON 解析 | Jackson | 解析华为云 API 响应 |
| 异步处理 | Kotlin Coroutines | 网络请求异步处理 |
| 数据获取方式 | IoTDA 设备影子 API | 每 2 秒轮询一次获取最新数据 |

### APP 连接华为云方式

APP 作为**数据消费端**，通过华为云 IAM 鉴权获取 Token，然后调用 IoTDA REST API 读取设备影子数据：

| 配置项 | 值 |
|--------|-----|
| IAM 用户名 | `jifei` |
| 华为账号名 | `hw005226973` |
| 项目ID | `dd5978aa446b4690884b89027c186d6e` |
| 设备ID | `69ce6bd8e094d615922d9e08_Smart_Home` |
| IoT 应用端点 | `52e4e17470.st1.iotda-app.cn-east-3.myhuaweicloud.com` |
| 设备影子 API | `GET /v5/iot/{project_id}/devices/{device_id}/shadow` |
| 数据刷新频率 | 每 2 秒 |

### 项目代码结构

```
smart_home_android/app/src/main/java/com/smarthome/huawei/
├── MainActivity.kt            ← APP 主界面（Compose UI + 数据轮询逻辑）
└── HuaweiIOT.kt              ← 华为云 IoTDA API 封装（Token 获取、设备影子读取、命令下发）
```

## 软件结构

### ESP32 端（在 xiaozhi-esp32 项目基础上新增的文件）

```
main/
├── stm32_uart.c / .h      ← 新增：STM32 串口通信 + 数据上传任务
├── huawei_cloud.c / .h    ← 新增：华为云 MQTT 客户端
├── main.cc                ← 修改：启动 stm32_task
├── CMakeLists.txt         ← 修改：添加新源文件
└── boards/bread-compact-wifi/
    └── config.h           ← 未修改（小智AI引脚不变）
```

## 编译与烧录

### 前置条件

- ESP-IDF v5.5
- Python 3.11
- Android Studio（Flutter 已弃用，不再需要）
- STM32CubeMX + Keil MDK-ARM（仅 STM32 端）

### ESP32-S3 编译烧录

> **重要**：编译前，必须先将 `ESP32-S3配置文件/sdkconfig` 复制到 `xiaozhi-esp32-1.8.4-2026.4.20/` 目录下，覆盖原有的 sdkconfig 文件。

在 ESP-IDF CMD 终端中执行：

```bash
cd xiaozhi-esp32-1.8.4-2026.4.20
idf.py build
idf.py -p COM8 flash monitor
```

#### 启动 AI 功能

烧录完成后，**首次使用需要按一下 ESP32-S3 的 BOOT 键**，确保串口日志中出现以下信息：

```
wifi:Set ps type: 0, coexist: 0
```

两项值均为 `0` 时，AI 语音功能才能正常启动。

### STM32 编译烧录

使用 Keil MDK-ARM 打开 `Smart_home_STM32/MDK-ARM/Smart home.uvprojx`，编译并下载。

### Android APP 编译运行

> **重要**：编译前，必须将 `APP配置参数.txt` 中的配置参数填入 `HuaweiIOT.kt` 文件的对应常量中，包括华为账号名、IAM 用户名、IAM 密码、项目ID、设备ID 等。

使用 Android Studio 打开 `smart_home_android/` 目录，连接 Android 手机或使用模拟器，点击运行即可。

## WiFi 配置

本项目连接 WiFi 热点 `jf`，首次烧录后通过小智 AI 的 Web 配置页面设置 WiFi。

## 技术要点

- **双核并行**：ESP32-S3 的两个核心分别处理语音交互和传感器数据上传，互不干扰
- **非阻塞设计**：stm32_task 大部分时间处于阻塞状态（等待串口数据/延时），不会影响语音任务的实时性
- **独立 MQTT 通道**：华为云 MQTT 与小智 AI 的通信协议互不干扰，各自维护独立的连接
- **串口引脚选择**：STM32 串口使用 GPIO8/9（UART1），避开了小智 AI 的麦克风引脚 GPIO4/5/6
- **APP 数据获取方式**：Android APP 通过华为云 IoTDA 设备影子 REST API 获取数据（非 MQTT），解决了 Flutter MQTT 客户端兼容性问题
- **IAM 鉴权**：APP 通过华为云 IAM 接口获取 Token，再调用 IoTDA API，保证访问安全性

## 华为云 IoT 平台

传感器数据通过 MQTT 协议上传至华为云 IoT 设备接入服务：

| 配置项 | 值 |
|--------|-----|
| MQTT Broker | `52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com` |
| 端口 | 1883 |
| 设备ID | `69ce6bd8e094d615922d9e08_Smart_Home` |
| 发布主题 | `$oc/devices/.../sys/properties/report` |
| 上传频率 | 每秒1次 |

## 实物接线总图

```
            ┌─────────────── STM32F103 ───────────────┐
            │                                          │
  DHT11 ────┤ PA8         温湿度                      │
  HC-SR501 ─┤ PA0         人体红外                    │
  MQ-7 ─────┤ PA1 (ADC)   CO 气体                     │
  OLED ─────┤ PB8(SCL)/PB9(SDA)  显示屏(软件I2C)     │
  蜂鸣器 ───┤ PB12         报警(低电平触发)            │
            │                                          │
            │ PB10 (TX) ────────────→ ESP32 GPIO9 (RX) │
            │ PB11 (RX) ←──────────── ESP32 GPIO8 (TX) │
            │ GND ──────────────────── GND             │
            └──────────────────────────────────────────┘
                            ↕
            ┌─────────────── ESP32-S3 ────────────────────────────┐
            │                                                      │
  麦克风 ───┤ GPIO4(WS)/GPIO5(SCK)/GPIO6(SD)  I2S 数字输入 3.3V   │
  功放 ─────┤ GPIO7(DIN)/GPIO15(BCLK)/GPIO16(LRCK)  I2S 输出 5V  │
  OLED ─────┤ GPIO41(SDA)/GPIO42(SCL)  I2C 显示屏 3.3V          │
  RGB LED ──┤ GPIO48       WS2812 RGB 指示灯                     │
  灯控 ─────┤ GPIO18       外接灯光控制                          │
  按键 ─────┤ GPIO0(BOOT)/GPIO47(触摸)/GPIO39/40(音量)           │
  STM32 ────┤ GPIO8(TX→PB11)/GPIO9(RX←PB10)  UART1 115200      │
            │                                                      │
            │            WiFi ────→ 华为云 IoT (MQTT 1883)        │
            │            WiFi ────→ 小智 AI 服务器 (WebSocket)    │
            └──────────────────────────────────────────────────────┘
                            ↕
            ┌─────────────── 华为云 IoTDA ─────────────┐
            │                                          │
            │       MQTT 属性上报 ──→ 设备影子          │
            │       REST API 查询 ←── APP 读取         │
            └──────────────────────────────────────────┘
                            ↕
            ┌─────────────── Android APP ─────────────┐
            │                                          │
            │  REST API ──→ 读取设备影子实时数据        │
            │  IAM 鉴权 ──→ Token 获取与自动续期       │
            │  Compose UI ──→ 实时传感器数据展示        │
            └──────────────────────────────────────────┘
```
