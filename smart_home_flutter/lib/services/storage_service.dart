import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';
import 'package:smart_home_flutter/models/sensor_data.dart';
import 'package:smart_home_flutter/models/warning_record.dart';
import 'package:smart_home_flutter/models/warning_threshold.dart';

class StorageService {
  static late SharedPreferences _prefs;

  static Future<void> init() async {
    _prefs = await SharedPreferences.getInstance();
  }

  static const String _keyThreshold = 'warning_threshold';
  static const String _keyWarningRecords = 'warning_records';
  static const String _keyThemeMode = 'theme_mode';
  static const String _keyNotificationEnabled = 'notification_enabled';
  static const String _keySoundEnabled = 'sound_enabled';
  static const String _keyVibrationEnabled = 'vibration_enabled';
  static const String _keyLastSensorData = 'last_sensor_data';

  static WarningThreshold getThreshold() {
    final json = _prefs.getString(_keyThreshold);
    if (json == null) {
      return WarningThreshold.defaultThreshold();
    }
    return WarningThreshold.fromJson(jsonDecode(json));
  }

  static Future<void> saveThreshold(WarningThreshold threshold) async {
    await _prefs.setString(_keyThreshold, jsonEncode(threshold.toJson()));
  }

  static List<WarningRecord> getWarningRecords() {
    final jsonList = _prefs.getStringList(_keyWarningRecords) ?? [];
    return jsonList
        .map((json) => WarningRecord.fromJson(jsonDecode(json)))
        .toList();
  }

  static Future<void> saveWarningRecords(List<WarningRecord> records) async {
    final jsonList = records.map((r) => jsonEncode(r.toJson())).toList();
    await _prefs.setStringList(_keyWarningRecords, jsonList);
  }

  static Future<void> addWarningRecord(WarningRecord record) async {
    final records = getWarningRecords();
    records.insert(0, record);
    if (records.length > 100) {
      records.removeRange(100, records.length);
    }
    await saveWarningRecords(records);
  }

  static Future<void> clearWarningRecords() async {
    await _prefs.remove(_keyWarningRecords);
  }

  static SensorData? getLastSensorData() {
    final json = _prefs.getString(_keyLastSensorData);
    if (json == null) return null;
    try {
      final map = jsonDecode(json);
      return SensorData.fromJson(map);
    } catch (e) {
      return null;
    }
  }

  static Future<void> saveLastSensorData(SensorData data) async {
    await _prefs.setString(_keyLastSensorData, jsonEncode(data.toJson()));
  }

  static String getThemeMode() {
    return _prefs.getString(_keyThemeMode) ?? 'system';
  }

  static Future<void> saveThemeMode(String mode) async {
    await _prefs.setString(_keyThemeMode, mode);
  }

  static bool isNotificationEnabled() {
    return _prefs.getBool(_keyNotificationEnabled) ?? true;
  }

  static Future<void> setNotificationEnabled(bool enabled) async {
    await _prefs.setBool(_keyNotificationEnabled, enabled);
  }

  static bool isSoundEnabled() {
    return _prefs.getBool(_keySoundEnabled) ?? true;
  }

  static Future<void> setSoundEnabled(bool enabled) async {
    await _prefs.setBool(_keySoundEnabled, enabled);
  }

  static bool isVibrationEnabled() {
    return _prefs.getBool(_keyVibrationEnabled) ?? true;
  }

  static Future<void> setVibrationEnabled(bool enabled) async {
    await _prefs.setBool(_keyVibrationEnabled, enabled);
  }
}
