import 'package:flutter/material.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:provider/provider.dart';
import 'package:smart_home_flutter/providers/sensor_provider.dart';
import 'package:smart_home_flutter/services/mqtt_service.dart';
import 'package:smart_home_flutter/widgets/sensor_card.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  @override
  void initState() {
    super.initState();
    _connectMqtt();
  }

  Future<void> _connectMqtt() async {
    final provider = context.read<SensorProvider>();
    if (!provider.isConnected) {
      await provider.connect();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('智能家居监控'),
        actions: [
          Consumer<SensorProvider>(
            builder: (context, provider, child) {
              return Padding(
                padding: EdgeInsets.symmetric(horizontal: 16.w),
                child: Row(
                  children: [
                    Icon(
                      provider.isConnected ? Icons.cloud_done : Icons.cloud_off,
                      color: provider.isConnected ? Colors.green : Colors.grey,
                      size: 20.sp,
                    ),
                    SizedBox(width: 4.w),
                    Text(
                      provider.isConnected ? '已连接' : '未连接',
                      style: TextStyle(
                        fontSize: 12.sp,
                        color: provider.isConnected
                            ? Colors.green
                            : Colors.grey,
                      ),
                    ),
                  ],
                ),
              );
            },
          ),
        ],
      ),
      body: Consumer<SensorProvider>(
        builder: (context, provider, child) {
          if (provider.connectionState == AppMqttState.connecting) {
            return const Center(child: CircularProgressIndicator());
          }

          final data = provider.currentData;

          if (data == null) {
            return Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(Icons.sensors_off, size: 64.sp, color: Colors.grey),
                  SizedBox(height: 16.h),
                  Text(
                    '等待传感器数据...',
                    style: TextStyle(fontSize: 16.sp, color: Colors.grey),
                  ),
                  SizedBox(height: 24.h),
                  ElevatedButton.icon(
                    onPressed: () => provider.connect(),
                    icon: const Icon(Icons.refresh),
                    label: const Text('重新连接'),
                  ),
                ],
              ),
            );
          }

          return RefreshIndicator(
            onRefresh: () async {
              await provider.connect();
            },
            child: SingleChildScrollView(
              physics: const AlwaysScrollableScrollPhysics(),
              padding: EdgeInsets.all(16.w),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  if (data.isWarning)
                    Container(
                      width: double.infinity,
                      padding: EdgeInsets.all(16.w),
                      margin: EdgeInsets.only(bottom: 16.h),
                      decoration: BoxDecoration(
                        color: Colors.red.shade100,
                        borderRadius: BorderRadius.circular(12.r),
                        border: Border.all(color: Colors.red.shade300),
                      ),
                      child: Row(
                        children: [
                          Icon(Icons.warning, color: Colors.red, size: 24.sp),
                          SizedBox(width: 12.w),
                          Expanded(
                            child: Text(
                              '检测到异常状态，请注意查看！',
                              style: TextStyle(
                                color: Colors.red.shade900,
                                fontSize: 14.sp,
                                fontWeight: FontWeight.w500,
                              ),
                            ),
                          ),
                        ],
                      ),
                    ),
                  Text(
                    '环境监测',
                    style: TextStyle(
                      fontSize: 18.sp,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: 12.h),
                  Row(
                    children: [
                      Expanded(
                        child: SensorCard(
                          title: '温度',
                          value: '${data.temp}°C',
                          status: data.tempStatus,
                          icon: Icons.thermostat,
                          color: _getTempColor(data.temp),
                        ),
                      ),
                      SizedBox(width: 12.w),
                      Expanded(
                        child: SensorCard(
                          title: '湿度',
                          value: '${data.humi}%',
                          status: data.humiStatus,
                          icon: Icons.water_drop,
                          color: _getHumiColor(data.humi),
                        ),
                      ),
                    ],
                  ),
                  SizedBox(height: 12.h),
                  SensorCard(
                    title: 'CO浓度',
                    value: '${data.mq7}',
                    status: data.coStatus,
                    icon: Icons.air,
                    color: _getCoColor(data.mq7),
                    subtitle: '估算PPM: ${data.ppm}',
                  ),
                  SizedBox(height: 24.h),
                  Text(
                    '安防状态',
                    style: TextStyle(
                      fontSize: 18.sp,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: 12.h),
                  Container(
                    width: double.infinity,
                    padding: EdgeInsets.all(16.w),
                    decoration: BoxDecoration(
                      color: data.hasPeople
                          ? Colors.orange.shade100
                          : Colors.green.shade100,
                      borderRadius: BorderRadius.circular(12.r),
                      border: Border.all(
                        color: data.hasPeople
                            ? Colors.orange.shade300
                            : Colors.green.shade300,
                      ),
                    ),
                    child: Row(
                      children: [
                        Icon(
                          data.hasPeople ? Icons.person : Icons.person_off,
                          size: 32.sp,
                          color: data.hasPeople ? Colors.orange : Colors.green,
                        ),
                        SizedBox(width: 16.w),
                        Expanded(
                          child: Column(
                            crossAxisAlignment: CrossAxisAlignment.start,
                            children: [
                              Text(
                                data.people,
                                style: TextStyle(
                                  fontSize: 18.sp,
                                  fontWeight: FontWeight.bold,
                                  color: data.hasPeople
                                      ? Colors.orange.shade900
                                      : Colors.green.shade900,
                                ),
                              ),
                              SizedBox(height: 4.h),
                              Text(
                                '人体红外检测',
                                style: TextStyle(
                                  fontSize: 12.sp,
                                  color: Colors.grey.shade600,
                                ),
                              ),
                            ],
                          ),
                        ),
                      ],
                    ),
                  ),
                  SizedBox(height: 24.h),
                  Text(
                    '设备状态',
                    style: TextStyle(
                      fontSize: 18.sp,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: 12.h),
                  Container(
                    width: double.infinity,
                    padding: EdgeInsets.all(16.w),
                    decoration: BoxDecoration(
                      color: Theme.of(
                        context,
                      ).colorScheme.surfaceContainerHighest,
                      borderRadius: BorderRadius.circular(12.r),
                    ),
                    child: Row(
                      mainAxisAlignment: MainAxisAlignment.spaceAround,
                      children: [
                        _buildStatusItem(
                          '报警状态',
                          data.warning,
                          data.warning == '正常' ? Colors.green : Colors.red,
                        ),
                        Container(
                          width: 1,
                          height: 40.h,
                          color: Colors.grey.shade300,
                        ),
                        _buildStatusItem(
                          '蜂鸣器',
                          data.beep ? '开启' : '关闭',
                          data.beep ? Colors.orange : Colors.grey,
                        ),
                      ],
                    ),
                  ),
                ],
              ),
            ),
          );
        },
      ),
    );
  }

  Color _getTempColor(int temp) {
    if (temp < 18) return Colors.blue;
    if (temp > 28) return Colors.red;
    return Colors.green;
  }

  Color _getHumiColor(int humi) {
    if (humi < 30) return Colors.orange;
    if (humi > 70) return Colors.blue;
    return Colors.green;
  }

  Color _getCoColor(int mq7) {
    if (mq7 > 2000) return Colors.red;
    if (mq7 > 1500) return Colors.orange;
    return Colors.green;
  }

  Widget _buildStatusItem(String label, String value, Color color) {
    return Column(
      children: [
        Text(
          value,
          style: TextStyle(
            fontSize: 16.sp,
            fontWeight: FontWeight.bold,
            color: color,
          ),
        ),
        SizedBox(height: 4.h),
        Text(
          label,
          style: TextStyle(fontSize: 12.sp, color: Colors.grey.shade600),
        ),
      ],
    );
  }
}
