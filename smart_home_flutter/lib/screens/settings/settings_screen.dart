import 'package:flutter/material.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:provider/provider.dart';
import 'package:smart_home_flutter/models/warning_threshold.dart';
import 'package:smart_home_flutter/providers/sensor_provider.dart';
import 'package:smart_home_flutter/providers/theme_provider.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final _tempMinController = TextEditingController();
  final _tempMaxController = TextEditingController();
  final _humiMinController = TextEditingController();
  final _humiMaxController = TextEditingController();
  final _coMaxController = TextEditingController();

  @override
  void dispose() {
    _tempMinController.dispose();
    _tempMaxController.dispose();
    _humiMinController.dispose();
    _humiMaxController.dispose();
    _coMaxController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('设置'),
      ),
      body: Consumer2<SensorProvider, ThemeProvider>(
        builder: (context, sensorProvider, themeProvider, child) {
          return SingleChildScrollView(
            padding: EdgeInsets.all(16.w),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  '外观设置',
                  style: TextStyle(
                    fontSize: 18.sp,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 12.h),
                Card(
                  child: ListTile(
                    leading: const Icon(Icons.palette),
                    title: const Text('主题模式'),
                    subtitle: Text(_getThemeModeText(themeProvider.themeMode)),
                    trailing: const Icon(Icons.chevron_right),
                    onTap: () => _showThemeDialog(context, themeProvider),
                  ),
                ),
                SizedBox(height: 24.h),
                Text(
                  '报警阈值设置',
                  style: TextStyle(
                    fontSize: 18.sp,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 12.h),
                Card(
                  child: Padding(
                    padding: EdgeInsets.all(16.w),
                    child: Column(
                      children: [
                        _buildThresholdRow(
                          '温度范围',
                          '°C',
                          _tempMinController,
                          _tempMaxController,
                          sensorProvider.threshold.tempMin,
                          sensorProvider.threshold.tempMax,
                        ),
                        SizedBox(height: 16.h),
                        _buildThresholdRow(
                          '湿度范围',
                          '%',
                          _humiMinController,
                          _humiMaxController,
                          sensorProvider.threshold.humiMin,
                          sensorProvider.threshold.humiMax,
                        ),
                        SizedBox(height: 16.h),
                        Row(
                          children: [
                            Expanded(
                              child: Text(
                                'CO浓度上限',
                                style: TextStyle(fontSize: 14.sp),
                              ),
                            ),
                            SizedBox(
                              width: 100.w,
                              child: TextField(
                                controller: _coMaxController,
                                keyboardType: TextInputType.number,
                                textAlign: TextAlign.center,
                                decoration: InputDecoration(
                                  hintText: sensorProvider.threshold.coMax.toString(),
                                  suffixText: '',
                                  isDense: true,
                                  contentPadding: EdgeInsets.symmetric(
                                    horizontal: 12.w,
                                    vertical: 8.h,
                                  ),
                                  border: const OutlineInputBorder(),
                                ),
                              ),
                            ),
                          ],
                        ),
                        SizedBox(height: 16.h),
                        SizedBox(
                          width: double.infinity,
                          child: ElevatedButton(
                            onPressed: () => _saveThreshold(sensorProvider),
                            child: const Text('保存设置'),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                SizedBox(height: 24.h),
                Text(
                  '关于',
                  style: TextStyle(
                    fontSize: 18.sp,
                    fontWeight: FontWeight.bold,
                  ),
                ),
                SizedBox(height: 12.h),
                Card(
                  child: Column(
                    children: [
                      ListTile(
                        leading: const Icon(Icons.info),
                        title: const Text('应用版本'),
                        trailing: const Text('1.0.0'),
                      ),
                      const Divider(height: 1),
                      ListTile(
                        leading: const Icon(Icons.code),
                        title: const Text('开源协议'),
                        trailing: const Icon(Icons.chevron_right),
                        onTap: () {
                          showLicensePage(
                            context: context,
                            applicationName: '智能家居助手',
                            applicationVersion: '1.0.0',
                          );
                        },
                      ),
                    ],
                  ),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildThresholdRow(
    String label,
    String unit,
    TextEditingController minController,
    TextEditingController maxController,
    double minValue,
    double maxValue,
  ) {
    return Row(
      children: [
        Expanded(
          child: Text(
            label,
            style: TextStyle(fontSize: 14.sp),
          ),
        ),
        SizedBox(
          width: 60.w,
          child: TextField(
            controller: minController,
            keyboardType: TextInputType.number,
            textAlign: TextAlign.center,
            decoration: InputDecoration(
              hintText: minValue.toString(),
              isDense: true,
              contentPadding: EdgeInsets.symmetric(
                horizontal: 8.w,
                vertical: 8.h,
              ),
              border: const OutlineInputBorder(),
            ),
          ),
        ),
        Padding(
          padding: EdgeInsets.symmetric(horizontal: 8.w),
          child: Text('-', style: TextStyle(fontSize: 14.sp)),
        ),
        SizedBox(
          width: 60.w,
          child: TextField(
            controller: maxController,
            keyboardType: TextInputType.number,
            textAlign: TextAlign.center,
            decoration: InputDecoration(
              hintText: maxValue.toString(),
              suffixText: unit,
              isDense: true,
              contentPadding: EdgeInsets.symmetric(
                horizontal: 8.w,
                vertical: 8.h,
              ),
              border: const OutlineInputBorder(),
            ),
          ),
        ),
      ],
    );
  }

  String _getThemeModeText(ThemeMode mode) {
    switch (mode) {
      case ThemeMode.light:
        return '浅色模式';
      case ThemeMode.dark:
        return '深色模式';
      default:
        return '跟随系统';
    }
  }

  void _showThemeDialog(BuildContext context, ThemeProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('选择主题'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            RadioListTile<ThemeMode>(
              title: const Text('跟随系统'),
              value: ThemeMode.system,
              groupValue: provider.themeMode,
              onChanged: (value) {
                provider.setThemeMode(value!);
                Navigator.pop(context);
              },
            ),
            RadioListTile<ThemeMode>(
              title: const Text('浅色模式'),
              value: ThemeMode.light,
              groupValue: provider.themeMode,
              onChanged: (value) {
                provider.setThemeMode(value!);
                Navigator.pop(context);
              },
            ),
            RadioListTile<ThemeMode>(
              title: const Text('深色模式'),
              value: ThemeMode.dark,
              groupValue: provider.themeMode,
              onChanged: (value) {
                provider.setThemeMode(value!);
                Navigator.pop(context);
              },
            ),
          ],
        ),
      ),
    );
  }

  void _saveThreshold(SensorProvider provider) {
    final threshold = WarningThreshold(
      tempMin: double.tryParse(_tempMinController.text) ?? provider.threshold.tempMin,
      tempMax: double.tryParse(_tempMaxController.text) ?? provider.threshold.tempMax,
      humiMin: double.tryParse(_humiMinController.text) ?? provider.threshold.humiMin,
      humiMax: double.tryParse(_humiMaxController.text) ?? provider.threshold.humiMax,
      coMax: int.tryParse(_coMaxController.text) ?? provider.threshold.coMax,
      intrusionEnabled: provider.threshold.intrusionEnabled,
    );

    provider.updateThreshold(threshold);

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('设置已保存')),
    );
  }
}
