package com.smarthome.huawei

import android.os.Bundle
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.animateColorAsState
import androidx.compose.animation.core.*
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.lifecycleScope
import android.content.Intent
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.text.SimpleDateFormat
import java.util.*

class MainActivity : ComponentActivity() {

    private val attributeData = mutableStateOf<Map<String, String>>(emptyMap())
    private val connectionStatus = mutableStateOf("正在连接...")
    private val lastUpdateTime = mutableStateOf("-")
    // 控制是否正在进行轮询连接
    private val isMonitoring = mutableStateOf(true)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            MaterialTheme(colorScheme = lightColorScheme(
                primary = Color(0xFF1976D2),
                secondary = Color(0xFF03A9F4),
                tertiary = Color(0xFF4CAF50)
            )) {
                SmartHomeDashboard(
                    attributeData = attributeData.value,
                    connectionStatus = connectionStatus.value,
                    lastUpdate = lastUpdateTime.value,
                    isMonitoring = isMonitoring.value,
                    onToggleMonitoring = { active -> 
                        isMonitoring.value = active
                        if (active) {
                            connectionStatus.value = "正在重新连接..."
                        } else {
                            connectionStatus.value = "⏸ 已断开连接"
                        }
                    },
                    onLogout = {
                        startActivity(Intent(this, LoginActivity::class.java))
                        finish()
                    }
                )
            }
        }

        startMonitoringLoop()
    }

    private fun startMonitoringLoop() {
        lifecycleScope.launch(Dispatchers.IO) {
            try {
                val huaweiIOT = HuaweiIOT()
                val sdf = SimpleDateFormat("HH:mm:ss", Locale.getDefault())

                while (true) {
                    if (isMonitoring.value) {
                        try {
                            val newData = huaweiIOT.getAllAttributes()
                            if (newData.isNotEmpty()) {
                                withContext(Dispatchers.Main) {
                                    attributeData.value = newData
                                    lastUpdateTime.value = sdf.format(Date())
                                    connectionStatus.value = "✅ 已连接"
                                }
                            }
                        } catch (e: Exception) {
                            withContext(Dispatchers.Main) {
                                connectionStatus.value = "❌ 连接异常"
                            }
                        }
                    }
                    // 无论是否在监控，都维持循环，但如果不监控则不发请求
                    delay(2000L)
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) {
                    connectionStatus.value = "❌ 初始化失败"
                    Toast.makeText(this@MainActivity, "初始化失败: ${e.message}", Toast.LENGTH_LONG).show()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SmartHomeDashboard(
    attributeData: Map<String, String>,
    connectionStatus: String,
    lastUpdate: String,
    isMonitoring: Boolean,
    onToggleMonitoring: (Boolean) -> Unit,
    onLogout: () -> Unit
) {
    Scaffold(
        topBar = {
            CenterAlignedTopAppBar(
                title = {
                    Text("智能家居控制台", fontWeight = FontWeight.Bold)
                },
                actions = {
                    IconButton(onClick = onLogout) {
                        Icon(Icons.Default.ExitToApp, contentDescription = "登出")
                    }
                },
                colors = TopAppBarDefaults.centerAlignedTopAppBarColors(
                    containerColor = Color.White,
                    titleContentColor = Color(0xFF1A1A1A)
                )
            )
        },
        containerColor = Color(0xFFF8F9FA)
    ) { innerPadding ->
        Column(
            modifier = Modifier
                .padding(innerPadding)
                .fillMaxSize()
                .padding(horizontal = 16.dp)
        ) {
            // 状态概览区域
            SummaryHeader(attributeData, connectionStatus, lastUpdate, isMonitoring)

            // 连接控制按钮组
            ConnectionControls(isMonitoring, onToggleMonitoring)

            Spacer(modifier = Modifier.height(16.dp))

            Text(
                "环境监测",
                fontSize = 18.sp,
                fontWeight = FontWeight.Bold,
                color = Color(0xFF333333),
                modifier = Modifier.padding(bottom = 12.dp)
            )

            // 环境数据网格
            LazyVerticalGrid(
                columns = GridCells.Fixed(2),
                horizontalArrangement = Arrangement.spacedBy(12.dp),
                verticalArrangement = Arrangement.spacedBy(12.dp),
                modifier = Modifier.weight(1f)
            ) {
                item {
                    SensorCard(
                        title = "温度",
                        value = attributeData["temp"] ?: "--",
                        unit = "°C",
                        icon = Icons.Default.Thermostat,
                        baseColor = Color(0xFFFF5252),
                        status = getTempStatus(attributeData["temp"]),
                        isMonitoring = isMonitoring
                    )
                }
                item {
                    SensorCard(
                        title = "湿度",
                        value = attributeData["humi"] ?: "--",
                        unit = "%RH",
                        icon = Icons.Default.WaterDrop,
                        baseColor = Color(0xFF448AFF),
                        status = getHumiStatus(attributeData["humi"]),
                        isMonitoring = isMonitoring
                    )
                }
                item {
                    SensorCard(
                        title = "CO浓度(PPM)",
                        value = attributeData["ppm"] ?: "--",
                        unit = "ppm",
                        icon = Icons.Default.Air,
                        baseColor = Color(0xFF7E57C2),
                        status = if ((attributeData["ppm"]?.toIntOrNull() ?: 0) > 50) "超标" else "正常",
                        isMonitoring = isMonitoring
                    )
                }
                item {
                    val isPresent = attributeData["hc_sr_501"] == "true" || attributeData["people"] == "有人"
                    SensorCard(
                        title = "安全监测",
                        value = if (isPresent) "有人" else "无人",
                        unit = "",
                        icon = if (isPresent) Icons.Default.DirectionsRun else Icons.Default.PersonOutline,
                        baseColor = if (isPresent) Color(0xFFFF9800) else Color(0xFF9E9E9E),
                        status = if (isPresent) "探测到活动" else "环境安全",
                        isMonitoring = isMonitoring
                    )
                }
                item {
                    val isWarning = attributeData["warning"] == "超标"
                    SensorCard(
                        title = "系统警报",
                        value = attributeData["warning"] ?: "正常",
                        unit = "",
                        icon = if (isWarning) Icons.Default.Warning else Icons.Default.CheckCircle,
                        baseColor = if (isWarning) Color(0xFFF44336) else Color(0xFF4CAF50),
                        status = if (isWarning) "请立即检查" else "运行良好",
                        isMonitoring = isMonitoring
                    )
                }
                item {
                    val isBeep = attributeData["beep"] == "true" || attributeData["beep"] == "1"
                    SensorCard(
                        title = "蜂鸣器",
                        value = if (isBeep) "鸣叫中" else "静音",
                        unit = "",
                        icon = if (isBeep) Icons.Default.NotificationsActive else Icons.Default.NotificationsOff,
                        baseColor = if (isBeep) Color(0xFFFFC107) else Color(0xFF607D8B),
                        status = "物理反馈状态",
                        isMonitoring = isMonitoring
                    )
                }
            }
            
            Spacer(modifier = Modifier.height(16.dp))
        }
    }
}

@Composable
fun ConnectionControls(isMonitoring: Boolean, onToggleMonitoring: (Boolean) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        Button(
            onClick = { onToggleMonitoring(true) },
            enabled = !isMonitoring,
            modifier = Modifier.weight(1f),
            shape = RoundedCornerShape(12.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFF4CAF50),
                disabledContainerColor = Color(0xFFE0E0E0)
            )
        ) {
            Icon(Icons.Default.PlayArrow, contentDescription = null)
            Spacer(modifier = Modifier.width(8.dp))
            Text("开启连接")
        }

        Button(
            onClick = { onToggleMonitoring(false) },
            enabled = isMonitoring,
            modifier = Modifier.weight(1f),
            shape = RoundedCornerShape(12.dp),
            colors = ButtonDefaults.buttonColors(
                containerColor = Color(0xFFF44336),
                disabledContainerColor = Color(0xFFE0E0E0)
            )
        ) {
            Icon(Icons.Default.Stop, contentDescription = null)
            Spacer(modifier = Modifier.width(8.dp))
            Text("断开连接")
        }
    }
}

@Composable
fun SummaryHeader(attributeData: Map<String, String>, connectionStatus: String, lastUpdate: String, isMonitoring: Boolean) {
    val isWarning = (attributeData["warning"] == "超标" || (attributeData["ppm"]?.toIntOrNull() ?: 0) > 50) && isMonitoring
    val backgroundColor by animateColorAsState(
        when {
            !isMonitoring -> Color(0xFFEEEEEE)
            isWarning -> Color(0xFFFFEBEE)
            else -> Color(0xFFE3F2FD)
        },
        animationSpec = tween(durationMillis = 500)
    )

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        colors = CardDefaults.cardColors(containerColor = backgroundColor),
        shape = RoundedCornerShape(24.dp)
    ) {
        Row(
            modifier = Modifier
                .padding(20.dp)
                .fillMaxWidth(),
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(56.dp)
                    .background(
                        when {
                            !isMonitoring -> Color(0xFF9E9E9E)
                            isWarning -> Color(0xFFFF5252)
                            else -> Color(0xFF2196F3)
                        },
                        CircleShape
                    ),
                contentAlignment = Alignment.Center
            ) {
                Icon(
                    imageVector = when {
                        !isMonitoring -> Icons.Default.CloudOff
                        isWarning -> Icons.Default.ReportProblem
                        else -> Icons.Default.Home
                    },
                    contentDescription = null,
                    tint = Color.White,
                    modifier = Modifier.size(32.dp)
                )
            }

            Spacer(modifier = Modifier.width(16.dp))

            Column {
                Text(
                    text = when {
                        !isMonitoring -> "监控已暂停"
                        isWarning -> "环境异常：请检查！"
                        else -> "室内环境：良好"
                    },
                    fontSize = 20.sp,
                    fontWeight = FontWeight.Bold,
                    color = when {
                        !isMonitoring -> Color(0xFF616161)
                        isWarning -> Color(0xFFD32F2F)
                        else -> Color(0xFF1976D2)
                    }
                )
                Text(
                    text = if (isMonitoring) "华为云已联机 | 更新于 $lastUpdate" else "点击开启连接以接收数据",
                    fontSize = 12.sp,
                    color = Color.Gray
                )
            }
        }
    }
}

@Composable
fun SensorCard(
    title: String,
    value: String,
    unit: String,
    icon: ImageVector,
    baseColor: Color,
    status: String,
    isMonitoring: Boolean
) {
    val infiniteTransition = rememberInfiniteTransition()
    val alpha by infiniteTransition.animateFloat(
        initialValue = 0.6f,
        targetValue = 1f,
        animationSpec = infiniteRepeatable(
            animation = tween(1000, easing = LinearEasing),
            repeatMode = RepeatMode.Reverse
        )
    )

    val contentAlpha = if (isMonitoring) 1f else 0.4f

    Card(
        modifier = Modifier.fillMaxWidth(),
        shape = RoundedCornerShape(20.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Row(
                verticalAlignment = Alignment.CenterVertically,
                horizontalArrangement = Arrangement.SpaceBetween,
                modifier = Modifier.fillMaxWidth()
            ) {
                Box(
                    modifier = Modifier
                        .size(36.dp)
                        .background(baseColor.copy(alpha = 0.1f * contentAlpha), CircleShape),
                    contentAlignment = Alignment.Center
                ) {
                    Icon(
                        imageVector = icon,
                        contentDescription = null,
                        tint = baseColor.copy(alpha = contentAlpha),
                        modifier = Modifier.size(20.dp)
                    )
                }
                
                if (isMonitoring && (status == "超标" || status == "探测到活动" || status == "请立即检查")) {
                    Box(
                        modifier = Modifier
                            .size(8.dp)
                            .clip(CircleShape)
                            .background(Color.Red.copy(alpha = alpha))
                    )
                }
            }

            Spacer(modifier = Modifier.height(12.dp))

            Text(
                text = title,
                fontSize = 13.sp,
                color = Color.Gray.copy(alpha = contentAlpha),
                fontWeight = FontWeight.Medium
            )

            Row(verticalAlignment = Alignment.Bottom) {
                Text(
                    text = value,
                    fontSize = 24.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color(0xFF1A1A1A).copy(alpha = contentAlpha)
                )
                if (unit.isNotEmpty()) {
                    Text(
                        text = " $unit",
                        fontSize = 14.sp,
                        color = Color.Gray.copy(alpha = contentAlpha),
                        modifier = Modifier.padding(bottom = 4.dp)
                    )
                }
            }

            Spacer(modifier = Modifier.height(4.dp))

            Text(
                text = if (isMonitoring) status else "暂停中",
                fontSize = 11.sp,
                color = baseColor.copy(alpha = contentAlpha),
                fontWeight = FontWeight.Bold
            )
        }
    }
}

private fun getTempStatus(value: String?): String {
    val temp = value?.toIntOrNull() ?: return "未知"
    return when {
        temp > 30 -> "偏热"
        temp < 15 -> "偏冷"
        else -> "适宜"
    }
}

private fun getHumiStatus(value: String?): String {
    val humi = value?.toIntOrNull() ?: return "未知"
    return when {
        humi > 70 -> "潮湿"
        humi < 30 -> "干燥"
        else -> "舒适"
    }
}
