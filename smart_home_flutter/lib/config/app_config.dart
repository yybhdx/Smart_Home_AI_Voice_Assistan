class AppConfig {
  static const String appName = '智能家居助手';
  static const String appVersion = '1.0.0';
  
  static const String huaweiCloudMqttBroker = '52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com';
  static const int mqttPort = 1883;
  static const String deviceId = '69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213';
  
  static const String mqttPublishTopic = '\$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213/sys/properties/report';
  static const String mqttSubscribeTopic = '\$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213/sys/properties/report';
  
  static const Duration dataRefreshInterval = Duration(seconds: 1);
  static const Duration connectionTimeout = Duration(seconds: 10);
  
  static const double tempWarningThreshold = 30.0;
  static const double humiWarningThreshold = 80.0;
  static const int coWarningThreshold = 2000;
}
