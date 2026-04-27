package com.smarthome.huawei

import android.util.Log
import com.fasterxml.jackson.databind.JsonNode
import com.fasterxml.jackson.databind.ObjectMapper
import java.io.BufferedReader
import java.io.BufferedWriter
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL

class HuaweiIOT {

    companion object {
        private const val TAG = "HuaweiIOT"

        // 从APP配置参数.txt获取的配置
        private const val HUAWEINAME = "hw005226973"  // 华为账号名/IAM用户名
        private const val IAMINAME = "jifei"    // IAM账户名
        private const val IAMPASSWORD = "a8pswys108"  // IAM账户密码

        // IoT相关配置
        private const val PROJECT_ID = "dd5978aa446b4690884b89027c186d6e"
        private const val DEVICE_ID = "69ce6bd8e094d615922d9e08_Smart_Home"
        private const val SERVICE_ID = "Smart_Home"  // 服务ID

        // IoT 接入终端节点（cn-east-3区域）
        private const val ENDPOINT = "52e4e17470.st1.iotda-app.cn-east-3.myhuaweicloud.com"

        // IAM接口地址（cn-east-3区域）
        private const val IAM_URL = "https://iam.cn-east-3.myhuaweicloud.com/v3/auth/tokens?nocatalog=false"
    }

    var token: String = ""
        private set

    init {
        try {
            token = fetchToken()
            Log.d(TAG, "✅ Token获取成功")
        } catch (e: Exception) {
            Log.e(TAG, "❌ Token获取失败: ${e.message}")
            throw e
        }
    }

    /**
     * 获取设备属性（shadow模式）或状态（status模式）
     * @param attribute 属性名称（如：temp, humi, mq-7, hc_sr_501等）
     * @param mode 查询模式："shadow"或"status"
     * @return 属性值字符串
     */
    fun getAttribute(attribute: String, mode: String): String {
        return try {
            var urlString = when (mode) {
                "shadow" -> "https://$ENDPOINT/v5/iot/$PROJECT_ID/devices/$DEVICE_ID/shadow"
                "status" -> "https://$ENDPOINT/v5/iot/$PROJECT_ID/devices/$DEVICE_ID"
                else -> throw IllegalArgumentException("无效的mode参数: $mode")
            }

            Log.d(TAG, "📡 请求URL: $urlString")

            val url = URL(urlString)
            val urlCon = url.openConnection() as HttpURLConnection
            urlCon.setRequestProperty("Content-Type", "application/json")
            urlCon.setRequestProperty("X-Auth-Token", token)
            urlCon.connect()

            val reader = BufferedReader(InputStreamReader(urlCon.inputStream))
            val result = StringBuilder()
            var line: String?
            while (reader.readLine().also { line = it } != null) {
                result.append(line)
            }
            reader.close()
            urlCon.disconnect()

            val response = result.toString()
            Log.d(TAG, "📥 API响应: $response")

            if (mode == "shadow") {
                parseShadowAttribute(response, attribute)
            } else if (mode == "status") {
                parseStatus(response)
            } else {
                "error"
            }
        } catch (e: Exception) {
            Log.e(TAG, "❌ 获取属性失败 [attribute=$attribute, mode=$mode]: ${e.message}")
            e.printStackTrace()
            "error"
        }
    }

    /**
     * 解析设备影子数据中的属性值
     */
    private fun parseShadowAttribute(jsonStr: String, attribute: String): String {
        return try {
            val objectMapper = ObjectMapper()
            val jsonNode: JsonNode = objectMapper.readValue(jsonStr, JsonNode::class.java)

            // 解析路径: shadow[0].reported.properties.{attribute}
            val valueNode = jsonNode
                .get("shadow")?.get(0)?.get("reported")?.get("properties")?.get(attribute)

            if (valueNode != null) {
                val value = valueNode.asText()
                Log.d(TAG, "✅ $attribute = $value")
                value
            } else {
                Log.w(TAG, "⚠️ 未找到属性: $attribute")
                "N/A"
            }
        } catch (e: Exception) {
            Log.e(TAG, "❌ JSON解析失败: ${e.message}")
            "error"
        }
    }

    /**
     * 解析设备状态
     */
    private fun parseStatus(jsonStr: String): String {
        return try {
            val objectMapper = ObjectMapper()
            val jsonNode: JsonNode = objectMapper.readValue(jsonStr, JsonNode::class.java)
            val statusNode = jsonNode.get("status")
            statusNode?.asText() ?: "unknown"
        } catch (e: Exception) {
            "error"
        }
    }

    /**
     * 下发命令到设备
     */
    fun setCommand(command: String, value: String): String {
        return try {
            val urlString = "https://$ENDPOINT/v5/iot/$PROJECT_ID/devices/$DEVICE_ID/commands"
            val url = URL(urlString)
            val urlCon = url.openConnection() as HttpURLConnection

            urlCon.requestMethod = "POST"
            urlCon.setRequestProperty("Content-Type", "application/json")
            urlCon.setRequestProperty("X-Auth-Token", token)
            urlCon.doOutput = true
            urlCon.useCaches = false
            urlCon.instanceFollowRedirects = true
            urlCon.connect()

            val body = """{"paras":{"$command":$value},"service_id":"$SERVICE_ID","command_name":"control"}"""

            BufferedWriter(OutputStreamWriter(urlCon.outputStream, "UTF-8")).use { writer ->
                writer.write(body)
                writer.flush()
            }

            val reader = BufferedReader(InputStreamReader(urlCon.inputStream))
            val result = StringBuilder()
            var line: String?
            while (reader.readLine().also { line = it } != null) {
                result.append(line)
            }
            reader.close()
            urlCon.disconnect()

            val response = result.toString()
            Log.d(TAG, "📤 命令下发响应: $response")
            response
        } catch (e: Exception) {
            Log.e(TAG, "❌ 命令下发失败: ${e.message}")
            e.printStackTrace()
            "error"
        }
    }

    /**
     * 通过华为云IAM接口获取Token
     */
    private fun fetchToken(): String {
        val tokenJson = """
            {
                "auth": {
                    "identity": {
                        "methods": ["password"],
                        "password": {
                            "user": {
                                "domain": {"name": "$HUAWEINAME"},
                                "name": "$IAMINAME",
                                "password": "$IAMPASSWORD"
                            }
                        }
                    },
                    "scope": {"project": {"id": "$PROJECT_ID"}}
                }
            }
        """.trimIndent()

        Log.d(TAG, "🔐 正在获取IAM Token...")

        val url = URL(IAM_URL)
        val urlCon = url.openConnection() as HttpURLConnection
        urlCon.requestMethod = "POST"
        urlCon.setRequestProperty("Content-Type", "application/json;charset=utf8")
        urlCon.doOutput = true
        urlCon.useCaches = false
        urlCon.instanceFollowRedirects = true
        urlCon.connect()

        BufferedWriter(OutputStreamWriter(urlCon.outputStream, "UTF-8")).use { writer ->
            writer.write(tokenJson)
            writer.flush()
        }

        val tokenValue = urlCon.getHeaderField("X-Subject-Token")
        urlCon.disconnect()

        if (tokenValue == null || tokenValue.isEmpty()) {
            throw RuntimeException("❌ 获取Token失败：响应中未包含X-Subject-Token头")
        }

        Log.d(TAG, "✅ Token获取成功: ${tokenValue.take(20)}...")
        return tokenValue
    }
}
