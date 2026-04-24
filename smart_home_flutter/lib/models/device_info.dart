class DeviceInfo {
  final String deviceId;
  final String deviceName;
  final bool isOnline;
  final DateTime? lastOnlineTime;
  final String firmwareVersion;
  final int signalStrength;

  DeviceInfo({
    required this.deviceId,
    required this.deviceName,
    this.isOnline = false,
    this.lastOnlineTime,
    this.firmwareVersion = '1.0.0',
    this.signalStrength = 0,
  });

  factory DeviceInfo.fromConfig() {
    return DeviceInfo(
      deviceId: '69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213',
      deviceName: '智能家居终端',
      isOnline: false,
      firmwareVersion: '1.0.0',
    );
  }

  DeviceInfo copyWith({
    String? deviceId,
    String? deviceName,
    bool? isOnline,
    DateTime? lastOnlineTime,
    String? firmwareVersion,
    int? signalStrength,
  }) {
    return DeviceInfo(
      deviceId: deviceId ?? this.deviceId,
      deviceName: deviceName ?? this.deviceName,
      isOnline: isOnline ?? this.isOnline,
      lastOnlineTime: lastOnlineTime ?? this.lastOnlineTime,
      firmwareVersion: firmwareVersion ?? this.firmwareVersion,
      signalStrength: signalStrength ?? this.signalStrength,
    );
  }

  String get onlineStatusText => isOnline ? '在线' : '离线';

  String get lastOnlineText {
    if (lastOnlineTime == null) return '从未连接';
    final now = DateTime.now();
    final diff = now.difference(lastOnlineTime!);
    if (diff.inSeconds < 60) return '刚刚';
    if (diff.inMinutes < 60) return '${diff.inMinutes}分钟前';
    if (diff.inHours < 24) return '${diff.inHours}小时前';
    return '${diff.inDays}天前';
  }

  @override
  String toString() {
    return 'DeviceInfo(deviceId: $deviceId, isOnline: $isOnline, lastOnline: $lastOnlineText)';
  }
}
