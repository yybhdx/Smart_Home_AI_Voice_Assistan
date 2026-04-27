package com.smarthome.huawei

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class MainActivity : ComponentActivity() {

    private val attributeData = mutableStateOf<Map<String, String>>(emptyMap())
    private val connectionStatus = mutableStateOf("正在连接...")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme(colorScheme = lightColorScheme(primary = Color(0xFF1976D2))) {
                SmartHomeScreen(
                    attributeData = attributeData.value,
                    connectionStatus = connectionStatus.value
                )
            }
        }

        Toast.makeText(this, "智能家居监控系统启动中...", Toast.LENGTH_SHORT).show()

        lifecycleScope.launch(Dispatchers.IO) {
            try {
                connectionStatus.value = "正在连接华为云..."
                val huaweiIOT = HuaweiIOT()
                connectionStatus.value = "✅ 已连接"

                val attributes = listOf(
                    "temp",
                    "humi",
                    "mq-7",
                    "ppm",
                    "hc_sr_501",
                    "people",
                    "warning",
                    "beep"
                )

                while (true) {
                    val result = mutableMapOf<String, String>()

                    for (attr in attributes) {
                        try {
                            val value = huaweiIOT.getAttribute(attr, "shadow")
                            android.util.Log.d("MainActivity", "$attr = $value")
                            result[attr] = value
                        } catch (e: Exception) {
                            android.util.Log.e("MainActivity", "获取 $attr 失败: ${e.message}")
                            result[attr] = "error"
                        }
                    }

                    withContext(Dispatchers.Main) {
                        attributeData.value = result
                    }

                    delay(2000L)
                }
            } catch (e: Exception) {
                android.util.Log.e("MainActivity", "❌ 连接失败: ${e.message}")
                e.printStackTrace()
                withContext(Dispatchers.Main) {
                    connectionStatus.value = "❌ 连接失败: ${e.message}"
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SmartHomeScreen(
    attributeData: Map<String, String>,
    connectionStatus: String
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Text(
                        text = "🏠 智能家居监控",
                        fontSize = 20.sp,
                        fontWeight = FontWeight.Bold
                    )
                },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = Color(0xFF1976D2),
                    titleContentColor = Color.White
                )
            )
        },
        containerColor = Color(0xFFF5F5F5)
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize()
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            ConnectionStatusCard(status = connectionStatus)
            SensorDataCard(attributeData = attributeData)
        }
    }
}

@Composable
fun ConnectionStatusCard(status: String) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp),
        shape = RoundedCornerShape(8.dp)
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = "连接状态：",
                fontWeight = FontWeight.Bold,
                fontSize = 16.sp
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = status,
                color = when {
                    status.contains("✅") || status.contains("已连接") -> Color(0xFF4CAF50)
                    status.contains("❌") || status.contains("失败") -> Color(0xFFF44336)
                    else -> Color(0xFFFF9800)
                },
                fontWeight = FontWeight.Medium
            )
        }
    }
}

@Composable
fun SensorDataCard(attributeData: Map<String, String>) {
    Card(
        modifier = Modifier.fillMaxSize(),
        elevation = CardDefaults.cardElevation(defaultElevation = 6.dp),
        shape = RoundedCornerShape(8.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Text(
                text = "📊 实时传感器数据",
                fontSize = 18.sp,
                fontWeight = FontWeight.Bold,
                color = Color(0xFF1976D2)
            )

            Spacer(modifier = Modifier.height(16.dp))

            if (attributeData.isEmpty()) {
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier.padding(8.dp)
                ) {
                    CircularProgressIndicator(
                        modifier = Modifier.size(24.dp),
                        color = Color(0xFF1976D2)
                    )
                    Spacer(modifier = Modifier.width(12.dp))
                    Text(text = "正在加载数据...", fontSize = 14.sp)
                }
            } else {
                LazyColumn(
                    verticalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    items(attributeData.toList()) { (attr, value) ->
                        SensorDataRow(attribute = attr, value = value)
                    }
                }
            }
        }
    }
}

@Composable
fun SensorDataRow(attribute: String, value: String) {
    val displayName = when (attribute) {
        "temp" -> "🌡️ 温度"
        "humi" -> "💧 湿度"
        "mq-7" -> "💨 CO浓度(ADC)"
        "ppm" -> "☠️ CO浓度(PPM)"
        "hc_sr_501" -> "🚨 人体红外"
        "people" -> "👤 人员状态"
        "warning" -> "⚠️ 警报状态"
        "beep" -> "🔔 蜂鸣器"
        else -> attribute
    }

    val displayValue = formatValue(attribute, value)
    val valueColor = getTextColor(attribute, value)

    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(color = valueColor.copy(alpha = 0.1f), shape = RoundedCornerShape(4.dp))
            .padding(12.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = displayName,
            fontSize = 15.sp,
            fontWeight = FontWeight.Medium,
            modifier = Modifier.weight(1f)
        )
        Spacer(modifier = Modifier.width(16.dp))
        Text(
            text = displayValue,
            fontSize = 15.sp,
            fontWeight = FontWeight.Bold,
            color = valueColor
        )
    }

    Divider(
        modifier = Modifier.padding(top = 8.dp),
        color = Color.LightGray.copy(alpha = 0.3f)
    )
}

private fun formatValue(attribute: String, value: String): String {
    return when (attribute) {
        "temp" -> "$value °C"
        "humi" -> "$value %RH"
        "mq-7", "ppm" -> value
        "hc_sr_501", "beep" -> if (value == "true" || value == "1") "开启" else "关闭"
        "people" -> value
        "warning" -> if (value == "超标") "⚠️ $value" else "✓ $value"
        else -> value
    }
}

private fun getTextColor(attribute: String, value: String): Color {
    return when (attribute) {
        "temp" -> {
            val temp = value.toIntOrNull() ?: 0
            when {
                temp > 30 -> Color(0xFFF44336)
                temp < 18 -> Color(0xFF2196F3)
                else -> Color(0xFF4CAF50)
            }
        }
        "humi" -> {
            val humi = value.toIntOrNull() ?: 0
            when {
                humi > 70 -> Color(0xFF2196F3)
                humi < 30 -> Color(0xFFFF9800)
                else -> Color(0xFF4CAF50)
            }
        }
        "mq-7", "ppm" -> {
            val coValue = value.toIntOrNull() ?: 0
            if (coValue > 2000) Color(0xFFF44336) else Color(0xFF4CAF50)
        }
        "warning" -> if (value == "超标") Color(0xFFF44336) else Color(0xFF4CAF50)
        "hc_sr_501", "people" -> if (value == "true" || value == "有人") Color(0xFFFF9800) else Color.Gray
        else -> Color.Gray
    }
}
