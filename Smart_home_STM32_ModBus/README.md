# 智能家居 STM32 终端 (Modbus 升级版)

基于 STM32F103C8T6 + FreeRTOS 的智能家居数据采集终端。本项目已从传统的 DHT11 升级为工控级 Modbus 温湿度传感器，并通过 ESP32-S3 将数据实时上报至华为云 IoT 平台。

## 硬件平台

| 硬件 | 型号 | 说明 |
|------|------|------|
| MCU | STM32F103C8T6 (Blue Pill) | 72MHz, 64KB Flash, 20KB SRAM |
| 温湿度传感器 | 工控 Modbus 传感器 | RS485 接口, Modbus RTU 协议, USART1 (9600bps) |
| RS485 转换器 | MAX3485 | 自动收发控制引脚 PA2 (RE/DE) |
| 一氧化碳传感器 | MQ-7 | ADC 采集, PA1 (ADC1_IN1) |
| 人体红外传感器 | HC-SR501 | GPIO 电平检测, PA0 |
| OLED 显示屏 | SSD1306 (128x64) | 软件 I2C, PB8(SCL)/PB9(SDA), 电源控制 PB6(GND)/PB7(VCC) |
| 蜂鸣器 | 有源蜂鸣器 | 低电平触发, PB12 |
| WiFi 模块 | ESP32-S3 | USART3 通信 (115200bps), PB10(TX)/PB11(RX) |
| 板载 LED | PC13 | 系统心跳指示 (500ms 翻转) |

## 软件架构

```
┌─────────────────────────────────────────────┐
│              FreeRTOS (CMSIS-RTOS V2)        │
│            抢占式调度, Tick=1000Hz            │
│            堆大小=10240字节, heap_4           │
├─────────────────────────────────────────────┤
│  defaultTask  │  ModBus任务  │   OLED任务   │
│  LED翻转      │  温湿度读取  │   UI刷新     │
│  512B/Normal  │ 2048B/Above  │ 1024B/Normal │
├───────────────┼──────────┼──────────┼────────┤
│  MQ7任务      │ HC_SR_501 │  Buzzer  │ ESP上报 │
│  CO浓度计算   │ 人体检测  │ 报警逻辑  │ 云端通信 │
│ 1024B/Normal  │ 1024B/Abov│1024B/Abov│1024B/Abov│
└───────────────┴──────────┴──────────┴────────┘
│           HAL 库 + STM32CubeMX 生成          │
└─────────────────────────────────────────────┘
```

## FreeRTOS 任务列表

| 任务名 | 函数 | 功能 | 栈大小 | 优先级 |
|--------|------|------|--------|--------|
| defaultTask | StartDefaultTask | 板载 LED 每 500ms 翻转 | 512B | Normal |
| RS485_ModBus_humi_temp | Temp_Humi_Read | Modbus RTU 轮询读取温湿度 | 2048B | AboveNormal |
| OLED | oled_task | 显示温湿度、ADC、PPM、人体状态及报警 | 1024B | Normal |
| MQ7 | mq7_task | ADC 读取 MQ-7，计算 PPM 浓度 (阈值 50ppm) | 1024B | Normal |
| HC_SR_501 | hc_sr501_task | 读取 PA0 检测是否有人 | 1024B | AboveNormal |
| buzzer | Buzzer_Task | 环境异常(CO/有人)时触发报警 | 1024B | AboveNormal |
| esp_report | esp_report | 构造 JSON 报文通过 UART3 发送至 ESP32 | 1024B | AboveNormal |

## 引脚映射 (核心外设)

| 引脚 | 功能 | 模式 | 说明 |
|------|------|------|------|
| PA0 | HC-SR501 输出 | 输入 | 检测人体红外信号 (1:有人, 0:无人) |
| PA1 | MQ-7 模拟输出 | 模拟输入 (ADC1_IN1) | 采集 CO 浓度模拟量 |
| PA2 | RS485 RE/DE | 推挽输出 | 控制 MAX3485 收发切换 (H:TX, L:RX) |
| PA9 | USART1_TX | 复用推挽 | 连接 RS485 模块 |
| PA10 | USART1_RX | 浮空输入 | 连接 RS485 模块 |
| PB6 | OLED GND | 推挽输出 | 供电负极控制 (置低电平) |
| PB7 | OLED VCC | 推挽输出 | 供电正极控制 (置高电平) |
| PB8 | OLED SCL | 开漏输出 | 软件 I2C 时钟线 |
| PB9 | OLED SDA | 开漏输出 | 软件 I2C 数据线 |
| PB10 | USART3_TX | 复用推挽 | 发送 JSON 到 ESP32-S3 |
| PB11 | USART3_RX | 浮空输入 | 接收 ESP32-S3 指令 |
| PB12 | 蜂鸣器 | 推挽输出 | 低电平报警 |
| PC13 | 板载 LED | 推挽输出 | 心跳指示灯 (低电平亮) |

## 通信协议配置

### 1. Modbus RTU (温湿度传感器)
- **串口**: USART1 (9600, 8N1)
- **控制**: 通过 PA2 实现半双工收发切换
- **功能码**: `03H` (读保持寄存器)
- **起始地址**: `0000H`
- **解析方式**:
  - 温度 = 寄存器[0] / 10.0 (°C)
  - 湿度 = 寄存器[1] / 10.0 (%RH)
- **校验**: CRC16 (Modbus 专用)

### 2. ESP32-S3 (云端/上报)
- **串口**: USART3 (115200, 8N1)
- **数据周期**: 每 2 秒上报一次
- **JSON 格式**:
  ```json
  {
    "temp": 25,
    "humi": 60,
    "mq7_raw": 1200,
    "ppm": 12.5,
    "people": "有人",
    "warning": "正常"
  }
  ```
  - `people`: "有人" 或 "无人"
  - `warning`: "报警" (ADC > 2500) 或 "正常"

## 目录结构 (MyApp)

- `modbus.c/h`: Modbus RTU 核心协议实现及 RS485 切换逻辑
- `temp_humi.c/h`: 温湿度传感器读取任务与数据解析
- `crc.c/h`: CRC16 循环冗余校验算法
- `esp32-s3.c/h`: 云端数据打包 (JSON) 与上报任务
- `myoled.c/h`: SSD1306 驱动及 4 行 UI 显示逻辑
- `OLED_Font.h`: OLED 字符点阵字库
- `mymq-7.c/h`: MQ-7 采样、电压转换及 PPM 浓度计算
- `hc-sr501.c/h`: 人体红外检测任务
- `buzzer.c/h`: 报警器控制逻辑 (基于标志位)

## 报警与预警逻辑

1. **蜂鸣器报警**:
   - 当 `ppm > 50.0` (CO 浓度) 或 `hc_sr501_value == 1` (有人) 时，蜂鸣器开启。
2. **云端预警**:
   - 当 `mq7_adc_value > 2500` 时，JSON 报文中的 `warning` 字段显示为 "报警"。
3. **OLED 警示**:
   - 当检测到有人时，屏幕第四行显示 "People!"。
   - 当 `mq7_adc_value > 4000` 且 `ppm > 4000` 时，屏幕显示 "Warning!"。

## 编译与运行

1. 使用 Keil uVision 5 打开 `MDK-ARM/Smart home.uvprojx`。
2. 编译器建议使用 **ARMCC V5**。
3. 确保已安装 **CMSIS-FreeRTOS** 支持包。
4. 编译并烧录至 STM32F103C8T6 开发板。
