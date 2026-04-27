# 智能家居华为云监控APP

基于GitHub开源项目（Smoge_alarm）改造的Android APP，用于连接华为云IoT平台获取智能家居传感器数据。

## 📋 项目特点

✅ **经过验证的方案** - 基于已成功运行的GitHub项目  
✅ **完全适配您的配置** - 使用您的APP配置参数  
✅ **实时数据展示** - 每2秒自动刷新传感器数据  
✅ **美观的UI界面** - 使用Jetpack Compose Material Design  
✅ **详细的日志输出** - 方便调试和问题排查  

## 🔧 技术栈

- **语言**: Kotlin
- **UI框架**: Jetpack Compose (Material 2)
- **网络请求**: HttpURLConnection (原生)
- **JSON解析**: Jackson Library
- **异步处理**: Kotlin Coroutines
- **最低版本**: Android 7.0 (API 24)
- **目标版本**: Android 14 (API 34)

## 📱 配置信息（已内置）

所有华为云配置参数已经硬编码在 `HuaweiIOT.kt` 中：

```kotlin
// 华为账号相关配置
HUAWEINAME = "hw005226973"     // 华为账号名
IAMINAME = "hw005226973"       // IAM账户名
IAMPASSWORD = "a8pswys108"      // IAM账户密码

// IoT相关配置
PROJECT_ID = "dd5978aa446b4690884b89027c186d6e"
DEVICE_ID = "69ce6bd8e094d615922d9e08_Smart_Home"
SERVICE_ID = "Smart_Home"
ENDPOINT = "iotda.cn-east-3.myhuaweicloud.com"  // cn-east-3区域
```

## 🚀 快速开始

### 1. 用Android Studio打开项目

1. 打开 Android Studio
2. 选择 File → Open
2. 导航到: `D:\Project\Final_year_project\Smart_Home_AI_Voice_Assistan-2.0.0\smart_home_android`
4. 点击 OK，等待Gradle同步完成

### 2. 连接手机或模拟器

- **真机推荐**: USB连接Android手机（需开启开发者选项和USB调试）
- **模拟器**: 使用Android Studio自带的模拟器

### 3. 运行APP

点击工具栏的 ▶️ 运行按钮，或按 Shift+F10

### 4. 查看日志

在Android Studio底部的 **Logcat** 窗口查看运行日志：
```
过滤标签: HuaweiIOT, MainActivity
```

## 📊 预期输出日志

成功连接后，您应该看到以下日志：

```
D/HuaweiIOT: 🔐 正在获取IAM Token...
D/HuaweiIOT: ✅ Token获取成功: eyJhbGciOiJSUzI1NiIs...
D/HuaweiIOT: ✅ Token获取成功
D/MainActivity: ✅ 已连接
D/HuaweiIOT: 📡 请求URL: https://iotda.cn-east-3.myhuaweicloud.com/v5/iot/dd5978aa.../devices/69ce6bd8.../shadow
D/HuaweiIOT: 📥 API响应: {"shadow":[{"reported":{"properties":{"temp":25,"humi":60,...}}}]}
D/HuaweiIOT: ✅ temp = 25
D/MainActivity: temp = 25
```

## 🖥️ APP界面说明

### 主界面布局：

1. **顶部栏**: 显示应用标题"🏠 智能家居监控"
2. **连接状态卡片**: 显示华为云连接状态
   - 🟢 "✅ 已连接" - 成功
   - 🟠 "正在连接..." - 进行中
   - 🔴 "❌ 连接失败" - 错误
3. **传感器数据卡片**: 实时显示8个属性值：
   - 🌡️ 温度 (°C) - 颜色根据温度变化：蓝(冷)/绿(正常)/红(热)
   - 💧 湿度 (%RH) - 颜色根据湿度变化：橙(干)/绿(正常)/蓝(湿)
   - 💨 CO浓度(ADC) - 红色表示超标
   - ☠️ CO浓度(PPM) - 红色表示危险
   - 🚨 人体红外 - 橙色表示检测到人
   - 👤 人员状态 - 显示"有人"/"无人"
   - ⚠️ 警报状态 - 红色表示"超标"
   - 🔔 蜂鸣器 - 显示"开启"/"关闭"

## 📁 项目结构

```
smart_home_android/
├── app/
│   ├── build.gradle                    # 应用级构建配置
│   └── src/main/
│       ├── AndroidManifest.xml         # 应用清单（含网络权限）
│       ├── java/com/smarthome/huawei/
│       │   ├── HuaweiIOT.kt            # ★ 核心类：华为云连接
│       │   └── MainActivity.kt         # ★ 主界面：数据展示
│       └── res/
│           ├── values/
│           │   ├── strings.xml         # 字符串资源
│           │   └── themes.xml          # 主题样式
│           └── ...                     # 其他资源
├── build.gradle                        # 项目级构建配置
├── settings.gradle                     # 项目设置
├── gradle.properties                   # Gradle属性
└── app/libs/
    ├── jackson-annotations-2.8.1.jar   # JSON解析库
    ├── jackson-core-2.8.1.jar
    └── jackson-databind-2.8.1.jar
```

## ⚙️ 核心代码解析

### HuaweiIOT.kt - 华为云连接核心类

#### 1. IAM认证获取Token
```kotlin
private fun getToken(): String {
    // 构建认证请求体
    val tokenJson = """{"auth":{"identity":{"methods":["password"],"password":{"user":{...}}},"scope":{"project":{"id":"$PROJECT_ID"}}}}"""
    
    // POST请求到IAM接口
    val urlCon = URL(IAM_URL).openConnection() as HttpURLConnection
    
    // 从响应头获取Token
    val token = urlCon.getHeaderField("X-Subject-Token")
    return token
}
```

#### 2. 查询设备影子
```kotlin
fun getAttribute(attribute: String, mode: String): String {
    // 构建查询URL
    val urlString = "https://$ENDPOINT/v5/iot/$PROJECT_ID/devices/$DEVICE_ID/shadow"
    
    // 设置请求头（包含Token）
    urlCon.requestProperty("X-Auth-Token", token)
    
    // 解析JSON响应
    // 路径: shadow[0].reported.properties.{attribute}
}
```

#### 3. 数据轮询机制（在MainActivity中）
```kotlin
lifecycleScope.launch(Dispatchers.IO) {
    val huaweiIOT = HuaweiIOT()  // 初始化并获取Token
    
    while (true) {
        for (attr in attributes) {
            val value = huaweiIOT.getAttribute(attr, "shadow")  // 查询每个属性
            result[attr] = value
        }
        
        attributeData.value = result  // 更新UI
        delay(2000L)  // 每2秒刷新一次
    }
}
```

## 🔍 与原GitHub项目的区别

| 对比项 | Smoge_alarm (原项目) | smart_home_android (本项目) |
|--------|---------------------|---------------------------|
| **用途** | 烟雾报警系统 | **智能家居监控系统** |
| **服务ID** | "Smoge" | **"Smart_Home"** |
| **设备ID** | 需手动填写 | **已内置** |
| **区域** | cn-north-4 | **cn-east-3** |
| **传感器** | Fire, Hum, Smg, Temp, Fee | **temp, humi, mq-7, ppm, hc_sr_501, people, warning, beep** |
| **UI风格** | 通用模板 | **定制化智能家居界面** |
| **数据展示** | 简单列表 | **彩色卡片 + 状态指示** |

## ❓ 常见问题

### Q1: 提示"获取Token失败"
**A**: 
- 检查网络连接
- 确认IAM账号密码是否正确
- 检查账号是否有IoTDA访问权限

### Q2: 提示"未找到属性: temp"
**A**:
- ESP32设备可能没有在线或没有上传数据
- 检查设备影子中是否有数据
- 在华为云控制台查看设备最新上报的数据

### Q3: 数据不更新
**A**:
- 查看Logcat是否有错误日志
- 确认ESP32每秒都在上传数据
- 检查华为云平台设备是否在线

### Q4: 编译报错"找不到Jackson库"
**A**:
- 确认`app/libs/`目录下有3个jar文件
- 执行 File → Sync Project with Gradle Files
- 清理并重建项目：Build → Clean Project → Rebuild Project

## 🎯 下一步优化建议

1. **添加下拉刷新功能**
2. **历史数据图表展示**
3. **报警阈值设置界面**
4. **远程控制功能**（通过setCommand方法）
5. **后台保活服务**
6. **数据导出功能**

## 📄 许可证

本项目基于 [Smoge_alarm](https://github.com/...) 开源项目改造，仅供学习交流使用。

---

**祝您毕设顺利！🎉**

如有问题，请检查Logcat日志并根据错误信息排查。
