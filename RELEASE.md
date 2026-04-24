# 智能家居 AI 语音助手 v2.0.0-beta

> 预发布版本 (Pre-release)

基于 ESP32-S3 + STM32F103 + Flutter 的智能家居系统。在 v1.0.0 基础上新增 **Flutter 移动端 APP**，实现端到云到手机的数据闭环，支持实时传感器数据查看、报警通知推送和远程设备控制。

## v2.0.0-beta 更新内容

### 新增：Flutter 移动端 APP（smart_home_flutter）

- **实时数据展示** — 通过 MQTT 订阅华为云 IoT 平台，实时显示温度、湿度、CO 浓度传感器数据
- **人体红外安防状态** — 实时查看 HC-SR501 人体红外检测状态
- **报警记录** — 传感器数据超过阈值时自动生成报警记录，支持本地通知推送，历史记录查看与一键清除
- **远程设备控制** — 通过 MQTT 命令远程控制蜂鸣器开关、安防模式、报警通知
- **可配置报警阈值** — 温度/湿度/CO 浓度阈值可自定义，设置持久化存储
- **主题切换** — 亮色/暗色/跟随系统三种主题
- **模拟模式** — 无硬件设备时自动生成模拟数据，方便演示和测试

### 技术栈新增

| 类别 | 技术 | 用途 |
|------|------|------|
| 移动端框架 | Flutter | 跨平台 APP 开发（Android/iOS） |
| 状态管理 | Provider | ChangeNotifier + Provider 模式 |
| 通信协议 | MQTT (mqtt_client) | 订阅华为云设备属性上报主题 |
| HTTP 客户端 | Dio | 华为云 IoTDA REST API |
| 本地存储 | SharedPreferences | 报警记录与设置持久化 |
| 通知推送 | flutter_local_notifications | 本地报警通知 |
| 屏幕适配 | flutter_screenutil | 多屏幕尺寸适配 |

### 数据流闭环

```
STM32F103 ──UART──→ ESP32-S3 ──MQTT上传──→ 华为云 IoTDA ──MQTT订阅──→ Flutter APP
                                                                 ↑                    │
                                                                 └── MQTT命令下发 ←────┘
                                                                     （蜂鸣器控制等）
```

---

## 完整功能特性

### AI 语音交互

- 语音唤醒检测
- 语音指令识别（基于小智 AI）
- 流式语音对话（ASR → LLM → TTS）
- 扬声器实时播报

### 环境监测

- **温湿度检测** — DHT11 传感器，实时采集温度与湿度
- **人体红外检测** — HC-SR501 传感器，检测是否有人活动
- **CO 气体检测** — MQ-7 传感器，检测一氧化碳浓度（ADC 采样 + PPM 换算）
- **超标报警** — CO 浓度超标时蜂鸣器自动报警

### 云端数据上传

- 通过 MQTT 协议将传感器数据实时上传至华为云 IoT 设备接入服务
- 每秒上传一次，JSON 格式数据包
- 设备影子同步

### 移动端远程监控（v2.0.0 新增）

- MQTT 实时订阅传感器数据，首页展示温度/湿度/CO 浓度卡片
- 安防状态面板显示人体红外检测结果
- 报警记录自动生成与本地通知推送
- 远程控制蜂鸣器开关
- 可配置报警阈值（温度/湿度/CO）
- 亮色/暗色主题切换

### 显示与交互

- OLED 显示屏实时显示传感器数据（STM32 与 ESP32 各一块）
- WS2812 RGB LED 状态指示
- 触摸按键与物理按键控制

## 硬件清单

| 模块 | 器件 | 说明 |
|------|------|------|
| 主控 (AI) | ESP32-S3 | 双核 240MHz，WiFi/BLE，I2S 音频 |
| 主控 (传感器) | STM32F103 | ARM Cortex-M3，多路 ADC |
| 温湿度 | DHT11 | 温度 0-50°C，湿度 20-90%RH |
| 人体红外 | HC-SR501 | 红外热释电，检测距离可调 |
| CO 气体 | MQ-7 | 一氧化碳传感器，模拟量输出 |
| 显示屏 x2 | SSD1306 128x32 | I2C OLED，分别连接两个 MCU |
| 音频输入 | I2S 数字麦克风 | 小智 AI 语音采集 |
| 音频输出 | I2S 功放模块 | 小智 AI 语音播报 |
| 蜂鸣器 | 有源蜂鸣器 | CO 超标报警 |
| LED | WS2812 RGB | 状态指示 |

## 快速开始

### 环境要求

- ESP-IDF v5.5
- Python 3.11
- Flutter SDK 3.x（移动端 APP）
- STM32CubeMX + Keil MDK-ARM（STM32 端）
- USB 转 TTL 串口线 x2
- Android 手机或模拟器（运行 APP）

### 编译烧录

**ESP32-S3 端：**

```bash
cd xiaozhi-esp32-1.8.4-2026.4.20
idf.py build
idf.py -p COMx flash monitor
```

**STM32 端：**

使用 Keil MDK-ARM 打开 `Smart_home_STM32/MDK-ARM/Smart home.uvprojx`，编译并下载。

**Flutter APP：**

```bash
cd smart_home_flutter
flutter pub get
flutter run
```

构建 APK：

```bash
flutter build apk
```

### 接线

STM32F103 与 ESP32-S3 通过 UART 串口通信：

```
STM32 PB10 (TX) → ESP32 GPIO9 (RX)
STM32 PB11 (RX) ← ESP32 GPIO8 (TX)
STM32 GND       ─ ESP32 GND
```

串口参数：115200 波特率，8N1。

### WiFi 配置

首次烧录后通过小智 AI 的 Web 配置页面设置 WiFi 连接。

## 传感器数据格式

STM32 每秒通过串口发送 JSON 数据：

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

## 技术栈

| 类别 | 技术 |
|------|------|
| MCU (AI) | ESP32-S3 + ESP-IDF v5.5 + FreeRTOS |
| MCU (传感器) | STM32F103 + HAL 库 + Keil MDK-ARM |
| AI 语音 | 小智 AI (XiaoZhi) |
| 移动端 | Flutter + Provider + mqtt_client |
| 云平台 | 华为云 IoT 设备接入服务 |
| 通信协议 | UART (MCU 间) / MQTT (设备↔云↔APP) |
| 音频编解码 | Opus |
| 构建系统 | CMake (ESP32) / Makefile (STM32) / Gradle (Flutter) |

## 已知问题

- [ ] WiFi 配置信息硬编码，需通过 Web 页面手动配置
- [ ] 华为云设备证书信息为硬编码，后续考虑改为动态配置
- [ ] CO 浓度 PPM 换算算法为简化模型，精度有限
- [ ] OLED 显示内容固定，未实现动态 UI
- [ ] 华为云 REST API 服务已定义但未集成，APP 当前仅通过 MQTT 获取数据
- [ ] MQTT 连接凭据硬编码在 APP 中，后续应支持用户输入或扫码配置

## 项目结构

```
Graduation-Project/
├── Smart_home_STM32/                  ← STM32F103 传感器采集工程
│   ├── Core/                          ← HAL 库核心代码
│   ├── MyApp/                         ← 传感器驱动
│   ├── MDK-ARM/                       ← Keil 工程文件
│   └── Smart home.ioc                 ← CubeMX 配置
├── xiaozhi-esp32-1.8.4-2026.4.20/     ← ESP32-S3 主工程
│   └── main/
│       ├── stm32_uart.c/h             ← STM32 串口通信
│       ├── huawei_cloud.c/h           ← 华为云 MQTT 客户端
│       ├── main.cc                    ← 程序入口
│       └── boards/                    ← 板级配置
├── smart_home_flutter/                ← Flutter 移动端 APP (v2.0.0 新增)
│   └── lib/
│       ├── models/                    ← 数据模型
│       ├── services/                  ← MQTT、REST API、通知、存储服务
│       ├── providers/                 ← Provider 状态管理
│       ├── screens/                   ← 首页/报警/设备/设置 页面
│       └── widgets/                   ← 通用组件
├── README.md
└── LICENSE                            ← MIT License
```

## 致谢

- [小智 AI (XiaoZhi)](https://github.com/78/xiaozhi-esp32) — ESP32 AI 语音助手开源项目
- [华为云 IoT](https://www.huaweicloud.com/product/iotda.html) — 设备接入服务
- [ESP-IDF](https://github.com/espressif/esp-idf) — ESP32 官方开发框架
- [Flutter](https://flutter.dev) — Google 跨平台 UI 框架

## 开源协议

本项目基于 [MIT License](LICENSE) 开源。
