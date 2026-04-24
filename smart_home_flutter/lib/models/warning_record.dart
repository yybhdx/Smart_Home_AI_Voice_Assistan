enum WarningType {
  temperature,
  humidity,
  carbonMonoxide,
  intrusion,
}

class WarningRecord {
  final String id;
  final WarningType type;
  final String title;
  final String message;
  final double value;
  final double threshold;
  final DateTime timestamp;
  final bool isRead;
  final bool isHandled;

  WarningRecord({
    required this.id,
    required this.type,
    required this.title,
    required this.message,
    required this.value,
    required this.threshold,
    DateTime? timestamp,
    this.isRead = false,
    this.isHandled = false,
  }) : timestamp = timestamp ?? DateTime.now();

  String get typeText {
    switch (type) {
      case WarningType.temperature:
        return '温度报警';
      case WarningType.humidity:
        return '湿度报警';
      case WarningType.carbonMonoxide:
        return 'CO浓度报警';
      case WarningType.intrusion:
        return '入侵报警';
    }
  }

  String get levelText {
    switch (type) {
      case WarningType.carbonMonoxide:
      case WarningType.intrusion:
        return '高危';
      case WarningType.temperature:
      case WarningType.humidity:
        return '警告';
    }
  }

  String get iconText {
    switch (type) {
      case WarningType.temperature:
        return '🌡️';
      case WarningType.humidity:
        return '💧';
      case WarningType.carbonMonoxide:
        return '⚠️';
      case WarningType.intrusion:
        return '🚨';
    }
  }

  Map<String, dynamic> toJson() {
    return {
      'id': id,
      'type': type.index,
      'title': title,
      'message': message,
      'value': value,
      'threshold': threshold,
      'timestamp': timestamp.toIso8601String(),
      'isRead': isRead,
      'isHandled': isHandled,
    };
  }

  factory WarningRecord.fromJson(Map<String, dynamic> json) {
    return WarningRecord(
      id: json['id'] as String,
      type: WarningType.values[json['type'] as int],
      title: json['title'] as String,
      message: json['message'] as String,
      value: (json['value'] as num).toDouble(),
      threshold: (json['threshold'] as num).toDouble(),
      timestamp: DateTime.parse(json['timestamp'] as String),
      isRead: json['isRead'] as bool? ?? false,
      isHandled: json['isHandled'] as bool? ?? false,
    );
  }

  WarningRecord copyWith({
    String? id,
    WarningType? type,
    String? title,
    String? message,
    double? value,
    double? threshold,
    DateTime? timestamp,
    bool? isRead,
    bool? isHandled,
  }) {
    return WarningRecord(
      id: id ?? this.id,
      type: type ?? this.type,
      title: title ?? this.title,
      message: message ?? this.message,
      value: value ?? this.value,
      threshold: threshold ?? this.threshold,
      timestamp: timestamp ?? this.timestamp,
      isRead: isRead ?? this.isRead,
      isHandled: isHandled ?? this.isHandled,
    );
  }

  static String generateId() {
    return DateTime.now().millisecondsSinceEpoch.toString();
  }

  @override
  String toString() {
    return 'WarningRecord(type: $typeText, title: $title, value: $value, timestamp: $timestamp)';
  }
}
