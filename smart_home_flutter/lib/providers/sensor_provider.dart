import 'dart:async';
import 'dart:math';
import 'package:flutter/foundation.dart';
import 'package:smart_home_flutter/models/sensor_data.dart';
import 'package:smart_home_flutter/models/warning_record.dart';
import 'package:smart_home_flutter/models/warning_threshold.dart';
import 'package:smart_home_flutter/services/mqtt_service.dart';
import 'package:smart_home_flutter/services/notification_service.dart';
import 'package:smart_home_flutter/services/storage_service.dart';

class SensorProvider extends ChangeNotifier {
  final MqttService _mqttService;
  final NotificationService _notificationService;

  SensorData? _currentData;
  final List<SensorData> _historyData = [];
  WarningThreshold _threshold = WarningThreshold.defaultThreshold();
  List<WarningRecord> _warningRecords = [];
  bool _notificationEnabled = true;
  bool _useSimulation = false;

  StreamSubscription<SensorData>? _dataSubscription;
  Timer? _refreshTimer;
  Timer? _simulationTimer;

  SensorProvider(this._mqttService, this._notificationService) {
    _loadSettings();
    _listenToMqttData();
    _startRefreshTimer();
  }

  SensorData? get currentData => _currentData;
  List<SensorData> get historyData => _historyData;
  WarningThreshold get threshold => _threshold;
  List<WarningRecord> get warningRecords => _warningRecords;
  bool get notificationEnabled => _notificationEnabled;

  AppMqttState get connectionState => _mqttService.connectionState;
  bool get isConnected => _mqttService.isConnected;

  void _loadSettings() {
    _threshold = StorageService.getThreshold();
    _warningRecords = StorageService.getWarningRecords();
    _notificationEnabled = StorageService.isNotificationEnabled();

    final lastData = StorageService.getLastSensorData();
    if (lastData != null) {
      _currentData = lastData;
    }
  }

  void _listenToMqttData() {
    _dataSubscription = _mqttService.dataStream.listen((data) {
      debugPrint('📥 收到传感器数据: $data');
      _updateData(data);
    });
  }

  void _startRefreshTimer() {
    _refreshTimer?.cancel();
    
    _refreshTimer = Timer.periodic(const Duration(seconds: 5), (timer) {
      if (_currentData == null || _useSimulation) {
        _generateSimulatedData();
      }
    });

    Future.delayed(const Duration(seconds: 2), () {
      if (_currentData == null) {
        debugPrint('📡 未收到真实数据，启动模拟模式');
        _useSimulation = true;
        _generateSimulatedData();
      }
    });
  }

  void _generateSimulatedData() {
    final random = Random();
    final now = DateTime.now();

    final data = SensorData(
      temp: 20 + random.nextInt(15),
      humi: 40 + random.nextInt(40),
      mq7: random.nextInt(500),
      ppm: random.nextInt(100),
      hcSr501: random.nextBool(),
      people: random.nextBool() ? '有人' : '无人',
      warning: random.nextDouble() > 0.8 ? '超标' : '正常',
      beep: random.nextBool(),
      timestamp: now,
    );

    debugPrint('🎲 生成模拟数据: $data');
    _updateData(data);
  }

  void _updateData(SensorData data) {
    _currentData = data;
    _historyData.insert(0, data);
    if (_historyData.length > 3600) {
      _historyData.removeRange(3600, _historyData.length);
    }
    StorageService.saveLastSensorData(data);
    _checkWarnings(data);
    notifyListeners();
  }

  void _checkWarnings(SensorData data) {
    if (_threshold.isTempWarning(data.temp.toDouble())) {
      _addWarning(WarningRecord(
        id: WarningRecord.generateId(),
        type: WarningType.temperature,
        title: '温度异常',
        message: '当前温度 ${data.temp}°C，超出正常范围',
        value: data.temp.toDouble(),
        threshold:
            data.temp < _threshold.tempMin ? _threshold.tempMin : _threshold.tempMax,
      ));
    }

    if (_threshold.isHumiWarning(data.humi.toDouble())) {
      _addWarning(WarningRecord(
        id: WarningRecord.generateId(),
        type: WarningType.humidity,
        title: '湿度异常',
        message: '当前湿度 ${data.humi}%，超出正常范围',
        value: data.humi.toDouble(),
        threshold: data.humi < _threshold.humiMin
            ? _threshold.humiMin
            : _threshold.humiMax,
      ));
    }

    if (_threshold.isCoWarning(data.mq7)) {
      _addWarning(WarningRecord(
        id: WarningRecord.generateId(),
        type: WarningType.carbonMonoxide,
        title: 'CO浓度超标',
        message: '当前CO浓度 ${data.mq7}，超过安全阈值',
        value: data.mq7.toDouble(),
        threshold: _threshold.coMax.toDouble(),
      ));
    }

    if (_threshold.intrusionEnabled && data.hasPeople) {
      _addWarning(WarningRecord(
        id: WarningRecord.generateId(),
        type: WarningType.intrusion,
        title: '检测到人员',
        message: '检测到有人进入监控区域',
        value: 1,
        threshold: 0,
      ));
    }
  }

  void _addWarning(WarningRecord record) {
    if (_warningRecords.isNotEmpty) {
      final last = _warningRecords.first;
      if (last.type == record.type &&
          DateTime.now().difference(last.timestamp).inMinutes < 5) {
        return;
      }
    }

    _warningRecords.insert(0, record);
    if (_warningRecords.length > 100) {
      _warningRecords.removeRange(100, _warningRecords.length);
    }
    StorageService.addWarningRecord(record);

    if (_notificationEnabled) {
      _notificationService.showWarning(record);
    }
  }

  Future<void> connect() async {
    await _mqttService.connect();
    notifyListeners();
  }

  Future<void> disconnect() async {
    await _mqttService.disconnect();
    notifyListeners();
  }

  Future<void> updateThreshold(WarningThreshold newThreshold) async {
    _threshold = newThreshold;
    await StorageService.saveThreshold(newThreshold);
    notifyListeners();
  }

  Future<void> setNotificationEnabled(bool enabled) async {
    _notificationEnabled = enabled;
    await StorageService.setNotificationEnabled(enabled);
    notifyListeners();
  }

  Future<void> clearWarningRecords() async {
    _warningRecords.clear();
    await StorageService.clearWarningRecords();
    notifyListeners();
  }

  void markWarningAsRead(String id) {
    final index = _warningRecords.indexWhere((r) => r.id == id);
    if (index != -1) {
      _warningRecords[index] =
          _warningRecords[index].copyWith(isRead: true);
      StorageService.saveWarningRecords(_warningRecords);
      notifyListeners();
    }
  }

  void toggleSimulation(bool enabled) {
    _useSimulation = enabled;
    if (enabled) {
      _generateSimulatedData();
    }
    notifyListeners();
  }

  Future<void> sendCommand(Map<String, dynamic> command) async {
    await _mqttService.publishProperties(command);
  }

  @override
  void dispose() {
    _dataSubscription?.cancel();
    _refreshTimer?.cancel();
    _simulationTimer?.cancel();
    super.dispose();
  }
}
