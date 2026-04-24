import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import 'package:mqtt_client/mqtt_client.dart';
import 'package:mqtt_client/mqtt_server_client.dart';
import 'package:smart_home_flutter/models/sensor_data.dart';

enum AppMqttState { disconnected, connecting, connected, error }

class MqttService extends ChangeNotifier {
  MqttServerClient? _client;
  AppMqttState _connectionState = AppMqttState.disconnected;
  String? _errorMessage;
  SensorData? _currentData;
  StreamSubscription<List<MqttReceivedMessage<MqttMessage>>>? _subscription;

  final String broker =
      '52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com';
  final int port = 1883;

  final String clientId = '69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213';
  final String username = '69ce6bd8e094d615922d9e08_Smart_Home';
  final String password =
      'b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19';

  AppMqttState get connectionState => _connectionState;
  String? get errorMessage => _errorMessage;
  SensorData? get currentData => _currentData;
  bool get isConnected => _connectionState == AppMqttState.connected;

  final StreamController<SensorData> _dataController =
      StreamController<SensorData>.broadcast();
  Stream<SensorData> get dataStream => _dataController.stream;

  Future<void> connect() async {
    if (_connectionState == AppMqttState.connecting) {
      return;
    }

    _connectionState = AppMqttState.connecting;
    _errorMessage = null;
    notifyListeners();

    try {
      _client = MqttServerClient.withPort(broker, clientId, port);

      _client!.logging(on: true);
      _client!.keepAlivePeriod = 60;
      _client!.connectTimeoutPeriod = 15000;
      _client!.autoReconnect = true;
      _client!.onDisconnected = _onDisconnected;
      _client!.onConnected = _onConnected;
      _client!.onSubscribeFail = _onSubscribeFail;

      final connMsg = MqttConnectMessage()
        ..withWillRetain()
        ..withWillQos(MqttQos.atMostOnce);

      _client!.connectionMessage = connMsg;

      debugPrint('========================================');
      debugPrint('🔌 MQTT连接参数:');
      debugPrint('Broker: $broker:$port');
      debugPrint('ClientId: $clientId');
      debugPrint('Username: $username');
      debugPrint('========================================');

      await _client!.connect(username, password);

      if (_client!.connectionStatus!.state != MqttConnectionState.connected) {
        throw Exception('连接失败: ${_client!.connectionStatus!.state}');
      }

      _connectionState = AppMqttState.connected;
      _subscribe();
      notifyListeners();
    } catch (e) {
      debugPrint('❌ MQTT连接异常: $e');
      _connectionState = AppMqttState.error;
      _errorMessage = e.toString();
      _client = null;
      notifyListeners();
    }
  }

  void _subscribe() {
    if (_client == null || !isConnected) return;

    final topic = '\$oc/devices/$username/sys/properties/report';
    debugPrint('📡 订阅主题: $topic');

    _client!.subscribe(topic, MqttQos.atLeastOnce);

    _subscription?.cancel();
    _subscription = _client!.updates!.listen((
      List<MqttReceivedMessage<MqttMessage?>> messages,
    ) {
      for (final message in messages) {
        final MqttPublishMessage recMess =
            message.payload as MqttPublishMessage;
        final payload = MqttPublishPayload.bytesToStringAsString(
          recMess.payload.message,
        );
        debugPrint('📥 收到原始数据:\n$payload');
        _handleMessage(payload);
      }
    });
  }

  void _handleMessage(String payload) {
    try {
      debugPrint('开始解析数据...');
      final json = jsonDecode(payload);
      debugPrint('JSON解析成功: $json');

      final data = SensorData.fromJson(json);
      debugPrint('传感器数据: $data');

      _currentData = data;
      _dataController.add(data);
      notifyListeners();
    } catch (e) {
      debugPrint('❌ 解析数据失败: $e');
      debugPrint('原始数据: $payload');
    }
  }

  Future<void> publishCommand(Map<String, dynamic> command) async {
    if (_client == null || !isConnected) {
      throw Exception('MQTT未连接');
    }

    final topic =
        '\$oc/devices/$clientId/sys/commands/request/id/${DateTime.now().millisecondsSinceEpoch}';
    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(command));
    _client!.publishMessage(topic, MqttQos.atMostOnce, builder.payload!);
  }

  Future<void> publishProperties(Map<String, dynamic> properties) async {
    if (_client == null || !isConnected) {
      throw Exception('MQTT未连接');
    }

    final topic = '\$oc/devices/$username/sys/properties/report';
    final payload = {
      'services': [
        {'service_id': 'Smart_Home', 'properties': properties},
      ],
    };

    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(payload));
    _client!.publishMessage(topic, MqttQos.atMostOnce, builder.payload!);
  }

  void _onConnected() {
    debugPrint('✅ MQTT已连接到华为云!');
    _connectionState = AppMqttState.connected;
    _errorMessage = null;
    notifyListeners();
  }

  void _onDisconnected() {
    debugPrint('❌ MQTT已断开连接');
    _connectionState = AppMqttState.disconnected;
    notifyListeners();
  }

  void _onSubscribeFail(String topic) {
    debugPrint('⚠️ 订阅失败: $topic');
  }

  Future<void> disconnect() async {
    await _subscription?.cancel();
    _client?.disconnect();
    _client = null;
    _connectionState = AppMqttState.disconnected;
    notifyListeners();
  }

  @override
  void dispose() {
    disconnect();
    _dataController.close();
    super.dispose();
  }
}
