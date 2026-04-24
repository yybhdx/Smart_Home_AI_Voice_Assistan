import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:provider/provider.dart';
import 'package:smart_home_flutter/providers/device_provider.dart';
import 'package:smart_home_flutter/providers/sensor_provider.dart';
import 'package:smart_home_flutter/providers/theme_provider.dart';
import 'package:smart_home_flutter/screens/main_screen.dart';
import 'package:smart_home_flutter/services/mqtt_service.dart';
import 'package:smart_home_flutter/services/notification_service.dart';
import 'package:smart_home_flutter/services/storage_service.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);

  await StorageService.init();

  final notificationService = NotificationService();
  await notificationService.initialize();

  final mqttService = MqttService();

  runApp(
    MyApp(mqttService: mqttService, notificationService: notificationService),
  );
}

class MyApp extends StatelessWidget {
  final MqttService mqttService;
  final NotificationService notificationService;

  const MyApp({
    super.key,
    required this.mqttService,
    required this.notificationService,
  });

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => ThemeProvider()),
        ChangeNotifierProvider(create: (_) => DeviceProvider()),
        ChangeNotifierProvider(
          create: (_) => SensorProvider(mqttService, notificationService),
        ),
      ],
      child: ScreenUtilInit(
        designSize: const Size(375, 812),
        minTextAdapt: true,
        splitScreenMode: true,
        builder: (context, child) {
          final themeProvider = Provider.of<ThemeProvider>(context);

          return MaterialApp(
            title: '智能家居助手',
            debugShowCheckedModeBanner: false,
            theme: themeProvider.lightTheme,
            darkTheme: themeProvider.darkTheme,
            themeMode: themeProvider.themeMode,
            home: const MainScreen(),
          );
        },
      ),
    );
  }
}
