import 'package:flutter/material.dart';
import 'package:smart_home_flutter/models/device_info.dart';

class DeviceProvider extends ChangeNotifier {
  DeviceInfo _deviceInfo = DeviceInfo.fromConfig();

  DeviceInfo get deviceInfo => _deviceInfo;

  void updateOnlineStatus(bool isOnline) {
    _deviceInfo = _deviceInfo.copyWith(
      isOnline: isOnline,
      lastOnlineTime: isOnline ? DateTime.now() : _deviceInfo.lastOnlineTime,
    );
    notifyListeners();
  }

  void updateDeviceInfo({
    String? deviceName,
    String? firmwareVersion,
    int? signalStrength,
  }) {
    _deviceInfo = _deviceInfo.copyWith(
      deviceName: deviceName,
      firmwareVersion: firmwareVersion,
      signalStrength: signalStrength,
    );
    notifyListeners();
  }
}
