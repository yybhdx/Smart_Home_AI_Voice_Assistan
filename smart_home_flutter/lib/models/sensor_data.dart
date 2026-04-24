class SensorData {
  final int temp;
  final int humi;
  final int mq7;
  final int ppm;
  final bool hcSr501;
  final String people;
  final String warning;
  final bool beep;
  final DateTime timestamp;

  SensorData({
    required this.temp,
    required this.humi,
    required this.mq7,
    required this.ppm,
    required this.hcSr501,
    required this.people,
    required this.warning,
    required this.beep,
    DateTime? timestamp,
  }) : timestamp = timestamp ?? DateTime.now();

  factory SensorData.fromJson(Map<String, dynamic> json) {
    try {
      final services = json['services'] as List<dynamic>?;
      if (services == null || services.isEmpty) {
        return SensorData.defaultData();
      }

      final properties = services[0]['properties'] as Map<String, dynamic>?;
      if (properties == null) {
        return SensorData.defaultData();
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
      return SensorData.defaultData();
    }
  }

  static SensorData defaultData() {
    return SensorData(
      temp: 0,
      humi: 0,
      mq7: 0,
      ppm: 0,
      hcSr501: false,
      people: '无人',
      warning: '正常',
      beep: false,
    );
  }

  Map<String, dynamic> toJson() {
    return {
      'services': [
        {
          'service_id': 'Smart_Home',
          'properties': {
            'temp': temp,
            'humi': humi,
            'mq-7': mq7,
            'ppm': ppm,
            'hc_sr_501': hcSr501,
            'people': people,
            'warning': warning,
            'beep': beep,
          }
        }
      ]
    };
  }

  String get tempStatus {
    if (temp < 18) return '偏冷';
    if (temp > 28) return '偏热';
    return '舒适';
  }

  String get humiStatus {
    if (humi < 30) return '干燥';
    if (humi > 70) return '潮湿';
    return '适宜';
  }

  String get coStatus {
    if (mq7 > 2000) return '危险';
    if (mq7 > 1500) return '偏高';
    return '正常';
  }

  bool get isWarning => warning == '超标' || mq7 > 2000;

  bool get hasPeople => hcSr501 || people == '有人';

  SensorData copyWith({
    int? temp,
    int? humi,
    int? mq7,
    int? ppm,
    bool? hcSr501,
    String? people,
    String? warning,
    bool? beep,
    DateTime? timestamp,
  }) {
    return SensorData(
      temp: temp ?? this.temp,
      humi: humi ?? this.humi,
      mq7: mq7 ?? this.mq7,
      ppm: ppm ?? this.ppm,
      hcSr501: hcSr501 ?? this.hcSr501,
      people: people ?? this.people,
      warning: warning ?? this.warning,
      beep: beep ?? this.beep,
      timestamp: timestamp ?? this.timestamp,
    );
  }

  @override
  String toString() {
    return 'SensorData(temp: $temp, humi: $humi, mq7: $mq7, ppm: $ppm, people: $people, warning: $warning)';
  }
}
