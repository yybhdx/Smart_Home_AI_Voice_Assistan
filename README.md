

---

# 基于 STM32 与 ESP32-S3 的智能家居环境监测与语音系统

## 项目简介

本项目是一款集成环境监测、安全告警、物联网云端接入及语音处理功能的智能家居终端。系统采用双处理器架构：
*   **STM32F103C8T6**：负责底层传感器数据（温湿度、有害气体、人体感应）的高频率采集、本地 OLED 显示及蜂鸣器异常告警。
*   **ESP32-S3**：作为物联网网关，通过串口接收 STM32 数据并上报至**华为云 IoT 平台**，同时利用其 I2S 接口实现了基础的音频采集与回放功能。

##  核心功能

*   **环境监测**：实时采集温度、湿度及一氧化碳（CO）浓度。
*   **入侵检测**：通过红外热释电传感器监控人体活动。
*   **智能告警**：当有害气体超标或有人入侵时，本地蜂鸣器自动触发。
*   **云端接入**：基于 MQTT 协议接入华为云设备接入（IoTDA），支持 JSON 格式数据上报。
*   **音频处理**：实现 16bit/44.1kHz 的音频环回（Loopback），为后续语音识别奠定基础。
*   **本地交互**：通过 0.96 寸 OLED 屏实时展示各项环境参数与系统状态。

## 硬件架构与引脚连接

### 传感器与执行器 (STM32)

| 模块           | 引脚 (STM32)           | 说明                      |
| :------------- | :--------------------- | :------------------------ |
| **DHT11**      | PA8                    | 温湿度数据读取            |
| **MQ-7**       | PA1 (ADC1_CH1)         | 模拟量采集，计算 PPM 浓度 |
| **HC-SR501**   | PA0                    | 人体感应信号 (高电平有效) |
| **OLED (I2C)** | PB8 (SCL), PB9 (SDA)   | 软件 I2C 驱动显示屏       |
| **Buzzer**     | PA5                    | 异常声光告警              |
| **ESP32 通信** | PB10 (TX3), PB11 (RX3) | UART3 对接 ESP32 串口     |

### 语音与通信 (ESP32-S3)

| 模块            | 引脚 (ESP32-S3)                 | 说明             |
| :-------------- | :------------------------------ | :--------------- |
| **I2S MIC**     | IO12(SCK), IO11(WS), IO10(SD)   | 数字麦克风输入   |
| **I2S Speaker** | IO16(BCLK), IO15(LRC), IO7(DIN) | 音频功率放大输出 |
| **STM32 UART**  | IO4 (TX), IO5 (RX)              | 对应 STM32 UART3 |

##  软件设计说明

###  数据上报协议 (JSON)

STM32 将传感器数据封装为华为云指定的标准 JSON 格式：
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

### ESP32-S3 音频逻辑

代码实现了 `audio_loopback_task`，通过 I2S 接口采集 32bit 原始数据并下采样为 16bit 后实时输出，实现了低延迟的音频环回。

###  关键算法

*   **MQ-7 PPM 换算**：基于 `pow(11.5428 * R0 / RS, 0.6549f)` 公式进行气体浓度线性化处理。
*   **DHT11 时序**：手动编写 `Delay_us` 实现单总线精确读写。

## 项目文件结构

```text
Graduation-Project
├── STM32_Firmware/             # STM32 工程目录
│   ├── MyApp/                  # 自定义驱动
│   │   ├── mydht11.c           # 温湿度驱动
│   │   ├── mymq-7.c            # 气体传感器 ADC+算法
│   │   ├── hc-sr501.c          # 人体感应逻辑
│   │   ├── myoled.c            # OLED 驱动
│   │   └── esp32-s3.c          # 串口 JSON 协议封装
│   └── main.c                  # 任务调度逻辑
├── ESP32_Firmware/             # ESP32-S3 工程目录 (IDF)
│   ├── main/
│   │   ├── wifi_mqtt.c         # WiFi 连接与华为云 MQTT 客户端
│   │   ├── audio_manager.c     # I2S 音频初始化与环回任务
│   │   └── main.c              # UART 转发与数据解析
└── README.md
```

## 如何运行

1.  **硬件连接**：按照第 3 章的引脚图连接传感器。
2.  **STM32 端**：使用 Keil MDK 开发环境编译并烧录。
3.  **ESP32 端**：
    *   配置 `wifi_mqtt.c` 中的 `WIFI_SSID` 和 `WIFI_PASS`。
    *   配置华为云 `CLIENT_ID`, `USERNAME`, `PASSWORD`（当前代码已填入，需确保云端设备在线）。
    *   使用 `idf.py build flash monitor` 进行编译部署。

## 注意事项

*   **MQ-7 预热**：该传感器初次使用需连续供电 24 小时以上以获得稳定读数。
*   **电源稳定**：由于 ESP32-S3 启动 WiFi 瞬间电流较大，建议为系统提供独立的 5V/2A 电源。

## 许可证

本项目采用[MIT License](https://github.com/yybhdx/Graduation-Project/blob/main/LICENSE)协议。
