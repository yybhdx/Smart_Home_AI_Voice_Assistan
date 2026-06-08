# 智能家居 STM32 终端

基于 STM32F103C8T6 + FreeRTOS 的智能家居数据采集终端，通过 ESP32-S3 连接华为云物联网平台。

## 硬件平台

| 硬件 | 型号 | 说明 |
|------|------|------|
| MCU | STM32F103C8T6 (Blue Pill) | 72MHz, 64KB Flash, 20KB SRAM |
| 温湿度传感器 | DHT11 | 单总线协议，PA8 |
| 一氧化碳传感器 | MQ-7 | ADC采集，PA1 (ADC1_IN1) |
| 人体红外传感器 | HC-SR501 | GPIO电平检测，PA0 |
| OLED显示屏 | SSD1306 (128x64) | 软件I2C，PB8(SCL)/PB9(SDA) |
| 蜂鸣器 | 有源蜂鸣器 | 低电平触发，PB12 |
| WiFi模块 | ESP32-S3 | UART3通信，PB10(TX)/PB11(RX) |
| 板载LED | PC13 | 系统心跳指示 |

## 软件架构

```
┌─────────────────────────────────────────────┐
│              FreeRTOS (CMSIS-RTOS V2)        │
│            抢占式调度, Tick=1000Hz            │
│            堆大小=10240字节, heap_4           │
├─────────────────────────────────────────────┤
│  defaultTask  │  dht11   │   OLED   │  MQ7   │
│  LED翻转      │  温湿度  │  显示    │ CO检测  │
│  512B/Normal  │1024B/Norm│1024B/Norm│1024B/N │
├───────────────┼──────────┼──────────┼────────┤
│  HC_SR_501    │  Buzzer  │  ESP32   │        │
│  人体检测      │  蜂鸣器  │  数据上报 │        │
│ 1024B/Above   │1024B/Abov│1024B/Abov│        │
└───────────────┴──────────┴──────────┴────────┘
│           HAL 库 + STM32CubeMX 生成          │
└─────────────────────────────────────────────┘
```

## FreeRTOS 任务列表

| 任务名 | 函数 | 功能 | 栈大小 | 优先级 |
|--------|------|------|--------|--------|
| defaultTask | StartDefaultTask | 板载LED每500ms翻转 | 512B | Normal |
| dht11 | dht11_task | 读取温湿度，挂起调度器保护时序 | 1024B | Normal |
| OLED | oled_task | 显示温湿度/ADC/PPM/人体检测/报警 | 1024B | Normal |
| MQ7 | mq7_task | ADC读取MQ-7，计算PPM浓度 | 1024B | Normal |
| HC_SR_501 | hc_sr501_task | 读取PA0电平检测是否有人 | 1024B | AboveNormal |
| buzzer | Buzzer_Task | CO超标或检测到人时蜂鸣器报警 | 1024B | AboveNormal |
| esp_report | esp_report | 构造JSON通过UART3上报华为云 | 1024B | AboveNormal |

## 引脚映射

| 引脚 | 功能 | 模式 |
|------|------|------|
| PA0 | HC-SR501 人体红外输出 | 输入，下拉 |
| PA1 | MQ-7 模拟输出 | 模拟输入 (ADC1_IN1) |
| PA8 | DHT11 数据线 | 输入/输出切换 (CRH寄存器) |
| PA9 | USART1_TX (调试串口) | 复用推挽输出 |
| PA10 | USART1_RX (调试串口) | 输入 |
| PB6 | OLED GND (供电控制) | 推挽输出 |
| PB7 | OLED VCC (供电控制) | 推挽输出 |
| PB8 | OLED SCL (软件I2C) | 开漏输出 |
| PB9 | OLED SDA (软件I2C) | 开漏输出 |
| PB10 | USART3_TX (ESP32通信) | 复用推挽输出 |
| PB11 | USART3_RX (ESP32通信) | 输入 |
| PB12 | 蜂鸣器 | 推挽输出 (低电平响) |
| PC13 | 板载LED | 推挽输出 |

## 时钟配置

```
HSE (8MHz) → PLL × 9 → SYSCLK (72MHz)
                ├── AHB  (72MHz)
                ├── APB1 (36MHz)  → USART3
                └── APB2 (72MHz)  → USART1, ADC1, TIM1
                                     ADC时钟 = 72/6 = 12MHz
```

## 外设配置

| 外设 | 配置 | 用途 |
|------|------|------|
| USART1 | 115200, 8N1 | 调试串口 (PA9/PA10) |
| USART3 | 115200, 8N1 | ESP32-S3通信 (PB10/PB11) |
| ADC1 | 单次转换, 12位, 通道1 | MQ-7模拟量采集 (PA1) |
| TIM1 | 预分频72, 1MHz计数 | DHT11微秒级延时 |

## 报警逻辑

- **CO超标报警**：MQ-7 ADC值 >= 2500 时 `buzzer_bit1 = 1`
- **人体检测报警**：HC-SR501 检测到人时 `buzzer_bit2 = 1`
- **蜂鸣器**：`buzzer_bit1` 或 `buzzer_bit2` 任一为1即触发蜂鸣器

## OLED 显示布局 (128x64, 4行)

```
第1行: temp:25 humi:60      ← 温度(整数) + 湿度(整数)
第2行: adc:1234              ← MQ-7 ADC原始值 (0-4095)
第3行: ppm:12                ← CO浓度 PPM值
第4行: People! Warning!      ← 人体检测 + CO超标警告
```

## 华为云 IoT 数据上报格式

通过 UART3 每1秒向 ESP32-S3 发送 JSON：

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

## 目录结构

```
Smart_home_STM32/
├── Core/
│   ├── Inc/                  # 头文件
│   │   ├── main.h            # 引脚定义
│   │   ├── FreeRTOSConfig.h  # FreeRTOS配置
│   │   ├── adc.h / gpio.h / tim.h / usart.h
│   │   └── stm32f1xx_it.h    # 中断处理声明
│   └── Src/                  # 源文件
│       ├── main.c            # 主函数 (初始化 + 启动调度器)
│       ├── freertos.c        # FreeRTOS任务定义与创建
│       ├── stm32f1xx_it.c    # 中断处理 (SysTick同时服务HAL和FreeRTOS)
│       ├── adc.c / gpio.c / tim.c / usart.c
│       └── stm32f1xx_hal_msp.c
├── MyApp/                    # 用户驱动代码
│   ├── mydht11.c/h           # DHT11温湿度传感器驱动
│   ├── myoled.c/h            # OLED显示驱动 (软件I2C)
│   ├── mymq-7.c/h            # MQ-7一氧化碳传感器驱动
│   ├── hc-sr501.c/h          # HC-SR501人体红外传感器驱动
│   ├── buzzer.c/h            # 蜂鸣器驱动
│   ├── esp32-s3.c/h          # ESP32-S3通信 + 华为云数据上报
│   └── OLED_Font.h           # OLED字模数据
├── Middlewares/              # FreeRTOS内核源码
├── MDK-ARM/                  # Keil工程文件
└── Smart home.ioc            # STM32CubeMX配置文件
```

## 编译与烧录

1. 使用 **Keil MDK-ARM V5** 打开 `MDK-ARM/Smart home.uvprojx`
2. 编译 (Build)
3. 通过 ST-Link / SWD 烧录

## FreeRTOS 关键配置说明

| 配置项 | 值 | 说明 |
|--------|-----|------|
| configUSE_PREEMPTION | 1 | 抢占式调度 |
| configTICK_RATE_HZ | 1000 | Tick频率 1ms |
| configTOTAL_HEAP_SIZE | 10240 | FreeRTOS堆大小 |
| configMAX_PRIORITIES | 56 | 最大优先级数 |
| configMINIMAL_STACK_SIZE | 128 | 最小栈大小 (字) |
| configUSE_TIMERS | 1 | 启用软件定时器 |
| configSUPPORT_STATIC_ALLOCATION | 1 | 支持静态分配 (Idle/Timer任务) |
| USE_FreeRTOS_HEAP_4 | - | 使用 heap_4 内存管理方案 |
