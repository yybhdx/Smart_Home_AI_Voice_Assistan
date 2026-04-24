import 'package:flutter/material.dart';
import 'package:flutter_screenutil/flutter_screenutil.dart';
import 'package:provider/provider.dart';
import 'package:smart_home_flutter/models/warning_record.dart';
import 'package:smart_home_flutter/providers/sensor_provider.dart';
import 'package:intl/intl.dart';

class WarningScreen extends StatelessWidget {
  const WarningScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('报警记录'),
        actions: [
          Consumer<SensorProvider>(
            builder: (context, provider, child) {
              if (provider.warningRecords.isEmpty) {
                return const SizedBox.shrink();
              }
              return TextButton(
                onPressed: () => _showClearDialog(context, provider),
                child: Text(
                  '清空',
                  style: TextStyle(fontSize: 14.sp),
                ),
              );
            },
          ),
        ],
      ),
      body: Consumer<SensorProvider>(
        builder: (context, provider, child) {
          final records = provider.warningRecords;

          if (records.isEmpty) {
            return Center(
              child: Column(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  Icon(
                    Icons.check_circle_outline,
                    size: 64.sp,
                    color: Colors.green,
                  ),
                  SizedBox(height: 16.h),
                  Text(
                    '暂无报警记录',
                    style: TextStyle(
                      fontSize: 16.sp,
                      color: Colors.grey,
                    ),
                  ),
                ],
              ),
            );
          }

          return ListView.builder(
            padding: EdgeInsets.all(16.w),
            itemCount: records.length,
            itemBuilder: (context, index) {
              final record = records[index];
              return _buildWarningCard(context, record, provider);
            },
          );
        },
      ),
    );
  }

  Widget _buildWarningCard(
    BuildContext context,
    WarningRecord record,
    SensorProvider provider,
  ) {
    final dateFormat = DateFormat('MM-dd HH:mm');

    return Card(
      margin: EdgeInsets.only(bottom: 12.h),
      child: ListTile(
        leading: Container(
          width: 48.w,
          height: 48.w,
          decoration: BoxDecoration(
            color: _getWarningColor(record.type).withValues(alpha: 0.1),
            borderRadius: BorderRadius.circular(8.r),
          ),
          child: Center(
            child: Text(
              record.iconText,
              style: TextStyle(fontSize: 24.sp),
            ),
          ),
        ),
        title: Row(
          children: [
            Expanded(
              child: Text(
                record.title,
                style: TextStyle(
                  fontSize: 16.sp,
                  fontWeight: FontWeight.w500,
                ),
              ),
            ),
            Container(
              padding: EdgeInsets.symmetric(horizontal: 8.w, vertical: 2.h),
              decoration: BoxDecoration(
                color: record.levelText == '高危' 
                    ? Colors.red.shade100 
                    : Colors.orange.shade100,
                borderRadius: BorderRadius.circular(4.r),
              ),
              child: Text(
                record.levelText,
                style: TextStyle(
                  fontSize: 10.sp,
                  color: record.levelText == '高危' 
                      ? Colors.red 
                      : Colors.orange,
                ),
              ),
            ),
          ],
        ),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            SizedBox(height: 4.h),
            Text(
              record.message,
              style: TextStyle(fontSize: 13.sp),
            ),
            SizedBox(height: 4.h),
            Text(
              dateFormat.format(record.timestamp),
              style: TextStyle(
                fontSize: 12.sp,
                color: Colors.grey,
              ),
            ),
          ],
        ),
        isThreeLine: true,
        trailing: record.isRead
            ? null
            : Container(
                width: 8.w,
                height: 8.w,
                decoration: const BoxDecoration(
                  color: Colors.red,
                  shape: BoxShape.circle,
                ),
              ),
        onTap: () {
          if (!record.isRead) {
            provider.markWarningAsRead(record.id);
          }
          _showWarningDetail(context, record);
        },
      ),
    );
  }

  Color _getWarningColor(WarningType type) {
    switch (type) {
      case WarningType.temperature:
        return Colors.red;
      case WarningType.humidity:
        return Colors.blue;
      case WarningType.carbonMonoxide:
        return Colors.orange;
      case WarningType.intrusion:
        return Colors.purple;
    }
  }

  void _showWarningDetail(BuildContext context, WarningRecord record) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Row(
          children: [
            Text(record.iconText, style: TextStyle(fontSize: 24.sp)),
            SizedBox(width: 8.w),
            Text(record.title),
          ],
        ),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('类型: ${record.typeText}'),
            SizedBox(height: 8.h),
            Text('等级: ${record.levelText}'),
            SizedBox(height: 8.h),
            Text('详情: ${record.message}'),
            SizedBox(height: 8.h),
            Text('当前值: ${record.value}'),
            SizedBox(height: 8.h),
            Text('阈值: ${record.threshold}'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('确定'),
          ),
        ],
      ),
    );
  }

  void _showClearDialog(BuildContext context, SensorProvider provider) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('清空记录'),
        content: const Text('确定要清空所有报警记录吗？'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('取消'),
          ),
          TextButton(
            onPressed: () {
              provider.clearWarningRecords();
              Navigator.pop(context);
            },
            child: const Text('确定'),
          ),
        ],
      ),
    );
  }
}
