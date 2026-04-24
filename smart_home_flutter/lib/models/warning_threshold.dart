class WarningThreshold {
  final double tempMin;
  final double tempMax;
  final double humiMin;
  final double humiMax;
  final int coMax;
  final bool intrusionEnabled;

  WarningThreshold({
    this.tempMin = 10.0,
    this.tempMax = 35.0,
    this.humiMin = 20.0,
    this.humiMax = 90.0,
    this.coMax = 2000,
    this.intrusionEnabled = true,
  });

  factory WarningThreshold.defaultThreshold() {
    return WarningThreshold(
      tempMin: 10.0,
      tempMax: 35.0,
      humiMin: 20.0,
      humiMax: 90.0,
      coMax: 2000,
      intrusionEnabled: true,
    );
  }

  bool isTempWarning(double temp) {
    return temp < tempMin || temp > tempMax;
  }

  bool isHumiWarning(double humi) {
    return humi < humiMin || humi > humiMax;
  }

  bool isCoWarning(int co) {
    return co > coMax;
  }

  Map<String, dynamic> toJson() {
    return {
      'tempMin': tempMin,
      'tempMax': tempMax,
      'humiMin': humiMin,
      'humiMax': humiMax,
      'coMax': coMax,
      'intrusionEnabled': intrusionEnabled,
    };
  }

  factory WarningThreshold.fromJson(Map<String, dynamic> json) {
    return WarningThreshold(
      tempMin: (json['tempMin'] as num?)?.toDouble() ?? 10.0,
      tempMax: (json['tempMax'] as num?)?.toDouble() ?? 35.0,
      humiMin: (json['humiMin'] as num?)?.toDouble() ?? 20.0,
      humiMax: (json['humiMax'] as num?)?.toDouble() ?? 90.0,
      coMax: json['coMax'] as int? ?? 2000,
      intrusionEnabled: json['intrusionEnabled'] as bool? ?? true,
    );
  }

  WarningThreshold copyWith({
    double? tempMin,
    double? tempMax,
    double? humiMin,
    double? humiMax,
    int? coMax,
    bool? intrusionEnabled,
  }) {
    return WarningThreshold(
      tempMin: tempMin ?? this.tempMin,
      tempMax: tempMax ?? this.tempMax,
      humiMin: humiMin ?? this.humiMin,
      humiMax: humiMax ?? this.humiMax,
      coMax: coMax ?? this.coMax,
      intrusionEnabled: intrusionEnabled ?? this.intrusionEnabled,
    );
  }

  @override
  String toString() {
    return 'WarningThreshold(temp: $tempMin-$tempMax, humi: $humiMin-$humiMax, co: $coMax)';
  }
}
