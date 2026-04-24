import 'package:flutter_test/flutter_test.dart';
import 'package:smart_home_flutter/main.dart';
import 'package:smart_home_flutter/services/mqtt_service.dart';
import 'package:smart_home_flutter/services/notification_service.dart';

void main() {
  testWidgets('App smoke test', (WidgetTester tester) async {
    final mqttService = MqttService();
    final notificationService = NotificationService();

    await tester.pumpWidget(
      MyApp(mqttService: mqttService, notificationService: notificationService),
    );

    expect(find.text('智能家居监控'), findsOneWidget);
  });
}
