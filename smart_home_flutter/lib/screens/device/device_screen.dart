import 'package:flutter/material.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:provider/provider.dart';
import 'package:smart_home_flutter/providers/device_provider.dart';
import 'package:smart_home_flutter/providers/sensor_provider.dart';

class DeviceScreen extends StatelessWidget {
  const DeviceScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('设备管理'),
      ),
      body: Consumer2<SensorProvider, DeviceProvider>(
        builder: (context, sensorProvider, deviceProvider, child) {
          final device = deviceProvider.deviceInfo;
          final isConnected = sensorProvider.isConnected;

          return SingleChildScrollView(
            padding: EdgeInsets.all(16.w),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                _buildDeviceCard(context, device, isConnected),
                SizedBox(height: 24.h),
                Text(
                  '设备控制',
                  style: TextStyle(
                    fontSize: 18.sp,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 12.h),
                _buildControlCard(context, sensorProvider),
                SizedBox(height: 24.h),
                Text(
                  '设备信息',
                  style: TextStyle(
                    fontSize: 18.sp,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 12.h),
                _buildInfoCard(context, device),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildDeviceCard(
    BuildContext context,
    dynamic device,
    bool isConnected,
  ) {
    return Card(
      child: Padding(
        padding: EdgeInsets.all(16.w),
        child: Row(
          children: [
            Container(
              width: 64.w,
              height: 64.w,
              decoration: BoxDecoration(
                color: isConnected 
                    ? Colors.green.shade100 
                    : Colors.grey.shade100,
                borderRadius: BorderRadius.circular(12.r),
              ),
              child: Icon(
                Icons.devices,
                size: 32.sp,
                color: isConnected ? Colors.green : Colors.grey,
              ),
            ),
            SizedBox(width: 16.w),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    device.deviceName,
                    style: TextStyle(
                      fontSize: 18.sp,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  SizedBox(height: 4.h),
                  Row(
                    children: [
                      Container(
                        width: 8.w,
                        height: 8.w,
                        decoration: BoxDecoration(
                          color: isConnected ? Colors.green : Colors.grey,
                          shape: BoxShape.circle,
                        ),
                      ),
                      SizedBox(width: 8.w),
                      Text(
                        isConnected ? '在线' : '离线',
                        style: TextStyle(
                          fontSize: 14.sp,
                          color: isConnected ? Colors.green : Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            IconButton(
              onPressed: () {},
              icon: const Icon(Icons.refresh),
              tooltip: '刷新',
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildControlCard(BuildContext context, SensorProvider provider) {
    return Card(
      child: Column(
        children: [
          ListTile(
            leading: const Icon(Icons.volume_up),
            title: const Text('蜂鸣器'),
            subtitle: const Text('控制设备蜂鸣器开关'),
            trailing: Switch(
              value: provider.currentData?.beep ?? false,
              onChanged: (value) {
                provider.sendCommand({'beep': value});
              },
            ),
          ),
          const Divider(height: 1),
          ListTile(
            leading: const Icon(Icons.security),
            title: const Text('安防模式'),
            subtitle: const Text('开启后将检测非法入侵'),
            trailing: Switch(
              value: provider.threshold.intrusionEnabled,
              onChanged: (value) {
                provider.updateThreshold(
                  provider.threshold.copyWith(intrusionEnabled: value),
                );
              },
            ),
          ),
          const Divider(height: 1),
          ListTile(
            leading: const Icon(Icons.notifications),
            title: const Text('报警通知'),
            subtitle: const Text('接收设备报警推送'),
            trailing: Switch(
              value: provider.notificationEnabled,
              onChanged: (value) {
                provider.setNotificationEnabled(value);
              },
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildInfoCard(BuildContext context, dynamic device) {
    return Card(
      child: Column(
        children: [
          _buildInfoTile('设备ID', device.deviceId),
          const Divider(height: 1),
          _buildInfoTile('固件版本', device.firmwareVersion),
          const Divider(height: 1),
          _buildInfoTile('最后在线', device.lastOnlineText),
        ],
      ),
    );
  }

  Widget _buildInfoTile(String label, String value) {
    return Padding(
      padding: EdgeInsets.symmetric(horizontal: 16.w, vertical: 12.h),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(
            label,
            style: TextStyle(
              fontSize: 14.sp,
              color: Colors.grey.shade600,
            ),
          ),
          Text(
            value,
            style: TextStyle(
              fontSize: 14.sp,
              fontWeight: FontWeight.w500,
            ),
          ),
        ],
      ),
    );
  }
}
