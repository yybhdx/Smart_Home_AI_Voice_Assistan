# 智能家居 AI 语音助手

基于 ESP32-S3 双核单片机的智能家居系统，融合了小智 AI 语音交互、传感器数据采集上传、以及 Flutter 移动端 APP 远程监控功能。

## 项目概述

本项目由硬件采集、AI 语音交互和移动端 APP 三部分协作完成：

| 模块 | 主控/平台 | 功能 |
|------|----------|------|
| **传感器采集** | STM32F103 | 读取温湿度、人体红外、CO 气体传感器数据，通过串口发送给 ESP32-S3 |
| **AI 语音 + 云端上传** | ESP32-S3 | 运行小智 AI 实现语音交互，同时接收 STM32 数据并上传至华为云 IoT 平台 |
| **移动端监控 APP** | Flutter (Android/iOS) | 通过 MQTT 协议从华为云实时读取传感器数据，远程监控与设备控制 |

## 项目目录结构

```
Graduation-Project/
├── README.md                          ← 项目说明文档（本文件）
├── Smart_home_STM32/                  ← STM32F103 传感器采集工程
│   ├── Core/                          ← STM32 HAL 库核心代码
│   ├── MyApp/                         ← 传感器驱动（DHT11、HC-SR501、MQ-7、OLED、蜂鸣器）
│   ├── MDK-ARM/                       ← Keil 工程文件
│   └── Smart home.ioc                 ← STM32CubeMX 配置文件
├── Smart_home_ESP32-S3/               ← ESP32-S3 独立版（仅串口+MQTT，未合并小智AI）
│   └── main/
│       ├── main.c                     ← UART 读取 + MQTT 发布
│       ├── wifi_mqtt.c/h             ← WiFi 连接 + 华为云 MQTT
│       └── audio_manager.c/h         ← I2S 音频回环（独立版用）
├── xiaozhi-esp32-1.8.4-2026.4.20/    ← 最终合并工程（小智AI + 传感器 + 华为云）
│   └── main/
│       ├── stm32_uart.c/h            ← 新增：STM32 串口通信 + 数据上传任务
│       ├── huawei_cloud.c/h          ← 新增：华为云 MQTT 客户端
│       ├── main.cc                   ← 修改：启动 stm32_task
│       └── boards/bread-compact-wifi/
│           └── config.h              ← 板级引脚配置（小智AI引脚不变）
└── smart_home_flutter/                ← Flutter 移动端 APP（智能家居助手）
    ├── pubspec.yaml                   ← 依赖配置
    └── lib/
        ├── main.dart                  ← APP 入口
        ├── config/                    ← 路由配置、华为云连接常量
        ├── models/                    ← 数据模型（传感器、设备、报警记录、阈值）
        ├── services/                  ← 服务层（MQTT、REST API、本地存储、通知）
        ├── providers/                 ← 状态管理（Provider 模式）
        ├── screens/                   ← 页面（首页、报警、设备、设置）
        └── widgets/                   ← 通用组件（传感器卡片等）
```

> **说明**：`Smart_home_ESP32-S3` 是合并前的独立版本，功能已整合到 `xiaozhi-esp32` 中，仅作存档保留。实际烧录使用 `xiaozhi-esp32-1.8.4-2026.4.20` 目录。

## 系统总体架构

```
┌──────────────┐   UART   ┌──────────────────┐   MQTT   ┌──────────┐   MQTT   ┌──────────────────┐
│  STM32F103   │ ───────→ │    ESP32-S3      │ ───────→ │  华为云   │ ←─────── │  Flutter APP     │
│  传感器采集   │  JSON    │  AI语音+云端上传  │  上传    │  IoT平台  │  订阅    │  移动端监控      │
│  温湿度/CO/  │          │                  │          │  IoTDA   │          │  实时数据展示     │
│  人体红外     │          └──────────────────┘          └──────────┘          │  报警通知        │
└──────────────┘                                                              │  远程控制        │
                                                                              └──────────────────┘
```

系统工作流程：

1. **STM32F103** 每秒采集传感器数据并通过串口发送 JSON 给 ESP32-S3
2. **ESP32-S3** 通过 MQTT 将数据上传至华为云 IoT 平台，同时运行小智 AI 语音交互
3. **Flutter APP** 通过 MQTT 订阅华为云设备属性上报主题，实时获取传感器数据，提供数据展示、报警通知和远程设备控制

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

### ESP32-S3 外设

| 外设 | 型号/类型 | 引脚 |
|------|----------|------|
| 麦克风 | I2S 数字麦克风 | WS=GPIO4, SCK=GPIO5, DIN=GPIO6 |
| 扬声器 | I2S 功放模块 | DOUT=GPIO7, BCLK=GPIO15, LRCK=GPIO16 |
| OLED 屏幕 | SSD1306 128x32 | SDA=GPIO41, SCL=GPIO42 |
| LED | WS2812 RGB | GPIO48 |
| BOOT 按键 | - | GPIO0 |
| 触摸按键 | - | GPIO47 |
| 音量+ | - | GPIO40 |
| 音量- | - | GPIO39 |
| **STM32 串口** | **UART1** | **TX=GPIO8, RX=GPIO9** |

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
STM32F103                          ESP32-S3                           华为云                        Flutter APP
┌─────────┐                     ┌──────────────────┐              ┌──────────┐                ┌──────────────┐
│ 传感器   │                     │                  │              │          │                │              │
│ 采集    │──UART3(PB10)────→  │ UART1(GPIO9)     │              │          │   MQTT 订阅    │  实时数据展示  │
│(每1秒)  │   JSON数据          │      ↓           │              │  IoT     │ ──────────────→│  温度/湿度/CO │
│         │                     │ stm32_task 读取   │              │  Device  │                │  人体红外状态  │
│ OLED    │                     │      ↓           │              │  Hub     │   属性上报      │              │
│ 显示    │                     │ MQTT 发布(每1秒)  │──MQTT(1883)─→│          │ ←──────────────│  MQTT 命令    │
│ 蜂鸣器  │                     │                  │   传感器数据  │          │                │  蜂鸣器控制   │
│ 报警    │                     │  ─ ─ ─ ─ ─ ─ ─  │              │          │                │  报警通知     │
│         │                     │                  │              │          │                │              │
│         │                     │ 小智 AI 语音交互  │              │          │                │              │
│         │                     │ 麦克风→ASR→LLM→  │              │          │                │              │
│         │                     │ TTS→扬声器       │              │          │                │              │
└─────────┘                     └──────────────────┘              └──────────┘                └──────────────┘
```

## Flutter 移动端 APP

### 功能概览

APP 通过 MQTT 协议连接华为云 IoT 平台，实时读取设备上报的传感器数据，提供以下功能：

| 功能模块 | 说明 |
|----------|------|
| **实时数据展示** | 首页展示温度、湿度、CO 浓度传感器卡片，人体红外安防状态，设备运行状态 |
| **报警记录** | 自动检测数据是否超过阈值，生成本地报警记录并推送通知，支持报警历史查看与一键清除 |
| **设备控制** | 远程控制蜂鸣器开关、安防模式开关、报警通知开关 |
| **设置** | 主题切换（亮色/暗色/跟随系统）、报警阈值自定义（温度/湿度/CO 浓度） |
| **模拟模式** | 无硬件时自动生成模拟数据用于演示和测试 |

### 页面结构

```
底部导航栏（4个页面）
├── 首页（Home）       ← 传感器数据卡片、安防状态面板、设备状态面板、报警横幅
├── 报警（Warning）    ← 报警记录列表、严重等级标签、详情弹窗、一键清除
├── 设备（Device）     ← 设备在线状态、蜂鸣器/安防/通知开关控制
└── 设置（Settings）   ← 主题选择、报警阈值配置、关于信息
```

### 技术架构

| 技术栈 | 选型 | 用途 |
|--------|------|------|
| 状态管理 | Provider | ChangeNotifier + Provider 模式管理全局状态 |
| 网络通信 | mqtt_client | MQTT v3.1.1 协议连接华为云 IoTDA |
| HTTP 客户端 | Dio | 华为云 IoTDA REST API 调用 |
| 本地存储 | SharedPreferences | 报警记录、阈值配置持久化 |
| 通知推送 | flutter_local_notifications | 本地报警通知 |
| 屏幕适配 | flutter_screenutil | 多屏幕尺寸适配（基准 375×812） |

### APP 连接华为云方式

APP 作为**数据消费端**通过 MQTT 连接华为云 IoT 平台，订阅设备属性上报主题获取实时数据：

| 配置项 | 值 |
|--------|-----|
| MQTT Broker | `52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com` |
| 端口 | 1883 |
| 设备ID | `69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213` |
| 订阅主题 | `$oc/devices/{device_id}/sys/properties/report` |
| 命令主题 | `$oc/devices/{device_id}/sys/commands/request/id/{timestamp}` |

### 项目代码结构

```
smart_home_flutter/lib/
├── main.dart                    ← APP 入口，初始化 Provider 和路由
├── config/
│   ├── app_config.dart          ← 华为云连接常量
│   └── routes.dart              ← 命名路由定义
├── models/
│   ├── sensor_data.dart         ← 传感器数据模型（temp/humi/mq-7/ppm/hc_sr_501 等）
│   ├── device_info.dart         ← 设备信息模型
│   ├── warning_record.dart      ← 报警记录模型
│   └── warning_threshold.dart   ← 报警阈值模型
├── services/
│   ├── mqtt_service.dart        ← MQTT 客户端（连接、订阅、发布）
│   ├── huawei_api_service.dart  ← 华为云 REST API 客户端
│   ├── storage_service.dart     ← 本地持久化存储
│   └── notification_service.dart← 本地通知推送
├── providers/
│   ├── sensor_provider.dart     ← 传感器数据状态管理（核心：MQTT 数据接收、报警检测、模拟数据）
│   ├── device_provider.dart     ← 设备在线/离线状态管理
│   └── theme_provider.dart      ← 主题切换管理
├── screens/
│   ├── main_screen.dart         ← 底部导航框架
│   ├── home/home_screen.dart    ← 首页（实时数据展示）
│   ├── warning/warning_screen.dart← 报警记录页
│   ├── device/device_screen.dart← 设备控制页
│   └── settings/settings_screen.dart← 设置页
└── widgets/
    └── sensor_card.dart         ← 传感器数据卡片组件
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
- Flutter SDK 3.x
- STM32CubeMX + Keil MDK-ARM（仅 STM32 端）

### ESP32-S3 编译烧录

在 ESP-IDF CMD 终端中执行：

```bash
cd xiaozhi-esp32-1.8.4-2026.4.20
idf.py build
idf.py -p COM8 flash monitor
```

### STM32 编译烧录

使用 Keil MDK-ARM 打开 `Smart_home_STM32/MDK-ARM/Smart home.uvprojx`，编译并下载。

### Flutter APP 编译运行

```bash
cd smart_home_flutter

# 获取依赖
flutter pub get

# 运行（连接手机或模拟器）
flutter run

# 构建 APK
flutter build apk
```

## WiFi 配置

本项目连接 WiFi 热点 `jf`，首次烧录后通过小智 AI 的 Web 配置页面设置 WiFi。

## 技术要点

- **双核并行**：ESP32-S3 的两个核心分别处理语音交互和传感器数据上传，互不干扰
- **非阻塞设计**：stm32_task 大部分时间处于阻塞状态（等待串口数据/延时），不会影响语音任务的实时性
- **独立 MQTT 通道**：华为云 MQTT 与小智 AI 的通信协议互不干扰，各自维护独立的连接
- **串口引脚选择**：STM32 串口使用 GPIO8/9（UART1），避开了小智 AI 的麦克风引脚 GPIO4/5/6
- **APP 实时监控**：Flutter APP 通过 MQTT 订阅华为云属性上报主题，与 ESP32 端共享同一设备通道，实现端到云到手机的数据闭环
- **模拟模式**：APP 内置模拟数据生成，无硬件设备时也可正常运行和演示

## 华为云 IoT 平台

传感器数据通过 MQTT 协议上传至华为云 IoT 设备接入服务：

| 配置项 | 值 |
|--------|-----|
| MQTT Broker | `52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com` |
| 端口 | 1883 |
| 设备ID | `69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213` |
| 发布主题 | `$oc/devices/.../sys/properties/report` |
| 上传频率 | 每秒1次 |

## 实物接线总图

```
            ┌─────────────── STM32F103 ───────────────┐
            │                                          │
  DHT11 ────┤ PA8         温湿度                      │
  HC-SR501 ─┤ PA0         人体红外                    │
  MQ-7 ─────┤ PA1 (ADC)   CO 气体                     │
  OLED ─────┤ PB8/PB9      显示屏                     │
  蜂鸣器 ───┤ PB12         报警                        │
            │                                          │
            │ PB10 (TX) ────────────→ ESP32 GPIO9 (RX) │
            │ PB11 (RX) ←──────────── ESP32 GPIO8 (TX) │
            │ GND ──────────────────── GND             │
            └──────────────────────────────────────────┘
                            ↕
            ┌─────────────── ESP32-S3 ────────────────┐
            │                                          │
  麦克风 ───┤ GPIO4/5/6    I2S 数字输入               │
  扬声器 ───┤ GPIO7/15/16  I2S 功放输出               │
  OLED ─────┤ GPIO41/42    I2C 显示屏                 │
  LED ──────┤ GPIO48       WS2812 RGB                 │
  按键 ─────┤ GPIO0/47/39/40  控制                    │
            │                                          │
            │            WiFi ────→ 华为云 IoT          │
            │            WiFi ────→ 小智 AI 服务器      │
            └──────────────────────────────────────────┘
                            ↕
            ┌─────────────── 华为云 IoTDA ─────────────┐
            │                                          │
            │       MQTT 属性上报 ──→ 设备影子          │
            │       MQTT 命令下发 ←── APP 控制          │
            └──────────────────────────────────────────┘
                            ↕
            ┌─────────────── Flutter APP ──────────────┐
            │                                          │
            │  MQTT 订阅 ──→ 实时传感器数据展示         │
            │  MQTT 发布 ──→ 蜂鸣器远程控制             │
            │  阈值检测  ──→ 本地报警通知推送           │
            └──────────────────────────────────────────┘
```
