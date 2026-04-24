import 'package:flutter/material.dart';
import 'package:smart_home_flutter/screens/home/home_screen.dart';
import 'package:smart_home_flutter/screens/main_screen.dart';
import 'package:smart_home_flutter/screens/warning/warning_screen.dart';
import 'package:smart_home_flutter/screens/device/device_screen.dart';
import 'package:smart_home_flutter/screens/settings/settings_screen.dart';

class Routes {
  static const String main = '/';
  static const String home = '/home';
  static const String warning = '/warning';
  static const String device = '/device';
  static const String settings = '/settings';
  
  static Map<String, WidgetBuilder> get routes => {
    main: (context) => const MainScreen(),
    home: (context) => const HomeScreen(),
    warning: (context) => const WarningScreen(),
    device: (context) => const DeviceScreen(),
    settings: (context) => const SettingsScreen(),
  };
  
  static void pushNamed(BuildContext context, String routeName) {
    Navigator.pushNamed(context, routeName);
  }
  
  static void pushReplacementNamed(BuildContext context, String routeName) {
    Navigator.pushReplacementNamed(context, routeName);
  }
  
  static void pop(BuildContext context) {
    Navigator.pop(context);
  }
}
