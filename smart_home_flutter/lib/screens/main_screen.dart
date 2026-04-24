import 'package:flutter/material.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:smart_home_flutter/screens/device/device_screen.dart';
import 'package:smart_home_flutter/screens/home/home_screen.dart';
import 'package:smart_home_flutter/screens/settings/settings_screen.dart';
import 'package:smart_home_flutter/screens/warning/warning_screen.dart';

class MainScreen extends StatefulWidget {
  const MainScreen({super.key});

  @override
  State<MainScreen> createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> {
  int _currentIndex = 0;

  final List<Widget> _screens = const [
    HomeScreen(),
    WarningScreen(),
    DeviceScreen(),
    SettingsScreen(),
  ];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: IndexedStack(index: _currentIndex, children: _screens),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex,
        onDestinationSelected: (index) {
          setState(() {
            _currentIndex = index;
          });
        },
        destinations: [
          NavigationDestination(
            icon: Icon(Icons.home_outlined, size: 24.sp),
            selectedIcon: Icon(Icons.home, size: 24.sp),
            label: '首页',
          ),
          NavigationDestination(
            icon: Icon(Icons.warning_amber_outlined, size: 24.sp),
            selectedIcon: Icon(Icons.warning, size: 24.sp),
            label: '报警',
          ),
          NavigationDestination(
            icon: Icon(Icons.devices_outlined, size: 24.sp),
            selectedIcon: Icon(Icons.devices, size: 24.sp),
            label: '设备',
          ),
          NavigationDestination(
            icon: Icon(Icons.settings_outlined, size: 24.sp),
            selectedIcon: Icon(Icons.settings, size: 24.sp),
            label: '设置',
          ),
        ],
      ),
    );
  }
}
