import 'dart:convert';
import 'package:dio/dio.dart';
import 'package:flutter/foundation.dart';
import 'package:smart_home_flutter/models/sensor_data.dart';

class HuaweiCloudApi {
  static const String _baseUrl =
      'https://iotda.cn-east-3.myhuaweicloud.com/v5/iot';
  
  final Dio _dio = Dio(BaseOptions(
    baseUrl: _baseUrl,
    connectTimeout: const Duration(seconds: 10),
    receiveTimeout: const Duration(seconds: 10),
    headers: {
      'Content-Type': 'application/json',
    },
  ));

  final String projectId = '69ce6bd8e094d615922d9e08';
  final String deviceId = '69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213';

  Future<SensorData?> fetchDeviceProperties() async {
    try {
      final response = await _dio.get(
        '/$projectId/devices/$deviceId',
        queryParameters: {
          'app_id': _appId,
        },
      );

      debugPrint('📥 API响应: ${response.data}');

      if (response.statusCode == 200) {
        return _parseResponse(response.data);
      }
      return null;
    } on DioException catch (e) {
      debugPrint('❌ API请求失败: $e');
      return null;
    } catch (e) {
      debugPrint('❌ 解析失败: $e');
      return null;
    }
  }

  SensorData? _parseResponse(dynamic data) {
    try {
      if (data == null) return null;

      final services = data['services'];
      if (services == null || services is! List || services.isEmpty) {
        return null;
      }

      final properties = services[0]['properties'];
      if (properties == null) {
        return null;
      }

      return SensorData(
        temp: properties['temp'] as int? ?? 0,
        humi: properties['humi'] as int? ?? 0,
        mq7: properties['mq-7'] as int? ?? 0,
        ppm: properties['ppm'] as int? ?? 0,
        hcSr501: properties['hc_sr_501'] as bool? ?? false,
        people: properties['people'] as String? ?? '无人',
        warning: properties['warning'] as String? ?? '正常',
        beep: properties['beep'] as bool? ?? false,
      );
    } catch (e) {
      debugPrint('解析设备属性失败: $e');
      return null;
    }
  }

  String get _appId => 'default';

  Future<List<Map<String, dynamic>>> fetchDeviceHistory({
    int limit = 20,
  }) async {
    try {
      final now = DateTime.now();
      final oneHourAgo = now.subtract(const Duration(hours: 1));

      final response = await _dio.get(
        '/$projectId/device-messages',
        queryParameters: {
          'device_id': deviceId,
          'start_time': oneHourAgo.millisecondsSinceEpoch,
          'end_time': now.millisecondsSinceEpoch,
          'limit': limit,
        },
      );

      if (response.statusCode == 200 && response.data is List) {
        return List<Map<String, dynamic>>.from(response.data);
      }
      return [];
    } catch (e) {
      debugPrint('获取历史数据失败: $e');
      return [];
    }
  }

  void dispose() {
    _dio.close();
  }
}
