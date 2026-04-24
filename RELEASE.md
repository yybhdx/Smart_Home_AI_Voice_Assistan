# 智能家居 AI 语音助手 v1.0.0-beta

> 预发布版本 (Pre-release)

基于 ESP32-S3 + STM32F103 的智能家居 AI 语音助手系统。融合小智 AI 语音交互、多传感器数据采集与华为云 IoT 平台数据上传。

## 系统架构

系统采用双 MCU 协作架构：

- **STM32F103** — 传感器采集终端：实时采集温湿度、人体红外、CO 气体浓度等环境数据，通过 UART 串口发送给 ESP32-S3
- **ESP32-S3** — AI 语音 + 云端枢纽：运行小智 AI 实现语音交互，同时接收 STM32 数据上传至华为云 IoT 平台

ESP32-S3 利用双核特性（Xtensa LX7，240MHz），在 Core 0 运行 AI 语音主逻辑与云端通信，Core 1 处理麦克风采集与音频编解码，实现语音交互与传感器数据上传的真正并行。

## 功能特性

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

### 显示与交互

- OLED 显示屏实时显示传感器数据
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
- STM32CubeMX + Keil MDK-ARM（STM32 端）
- USB 转 TTL 串口线 x2

### 编译烧录

**ESP32-S3 端：**

```bash
cd xiaozhi-esp32-1.8.4-2026.4.20
idf.py build
idf.py -p COMx flash monitor
```

**STM32 端：**

使用 Keil MDK-ARM 打开 `Smart_home_STM32/MDK-ARM/Smart home.uvprojx`，编译并下载。

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
| 云平台 | 华为云 IoT 设备接入服务 |
| 通信协议 | UART (MCU 间) / MQTT (设备→云) |
| 音频编解码 | Opus |
| 构建系统 | CMake (ESP32) / Makefile (STM32) |

## 已知问题

- [ ] WiFi 配置信息硬编码，需通过 Web 页面手动配置
- [ ] 华为云设备证书信息为硬编码，后续考虑改为动态配置
- [ ] CO 浓度 PPM 换算算法为简化模型，精度有限
- [ ] OLED 显示内容固定，未实现动态 UI

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
├── README.md
└── LICENSE                            ← MIT License
```

## 致谢

- [小智 AI (XiaoZhi)](https://github.com/78/xiaozhi-esp32) — ESP32 AI 语音助手开源项目
- [华为云 IoT](https://www.huaweicloud.com/product/iotda.html) — 设备接入服务
- [ESP-IDF](https://github.com/espressif/esp-idf) — ESP32 官方开发框架

## 开源协议

本项目基于 [MIT License](LICENSE) 开源。
