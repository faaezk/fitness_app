import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

void main() => runApp(const MyApp());

class MyApp extends StatelessWidget {
  const MyApp({super.key});
  @override
  Widget build(BuildContext context) => MaterialApp(
    title: 'Fitness App',
    theme: ThemeData(
      primarySwatch: Colors.blue,
    ),
    home: const MyHomePage(title: 'Fitness App'),
  );
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  @override
  MyHomePageState createState() => MyHomePageState();
}

class MyHomePageState extends State<MyHomePage> {
  List<BluetoothDevice> devicesList = [];
  List<BluetoothService> _services = [];
  Map<Guid, List<int>> readValues = {};
  int stepCount = 0;
  List<String> activity = ["Idle", "Moving"];
  int activityIndex = 0;

  final String serviceUuid = "19b10010-e8f2-537e-4f6c-d104768a1214";
  final String stepCharacteristicUuid = "19b10011-e8f2-537e-4f6c-d104768a1214";
  final String activityCharacteristicUuid = "19b10012-e8f2-537e-4f6c-d104768a1214";
  BluetoothCharacteristic? stepCharacteristic;
  BluetoothCharacteristic? activityCharacteristic;

  @override
  void initState() {
    super.initState();
    _initBluetooth();
  }

  void _initBluetooth() async {
    print("Initializing Bluetooth...");
    devicesList.clear();
    var subscription = FlutterBluePlus.onScanResults.listen(
          (results) {
        if (results.isNotEmpty) {
          for (ScanResult result in results) {
            if (result.device.platformName == "Arduino") {
              _connectToDevice(result.device);
              break;
            }
          }
        }
      },
      onError: (e) {
        print("Scan Error: $e");
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text(e.toString())),
        );
      },
    );

    FlutterBluePlus.cancelWhenScanComplete(subscription);
    await FlutterBluePlus.adapterState.where((val) => val == BluetoothAdapterState.on).first;

    print("Starting BLE scan...");
    await FlutterBluePlus.startScan();
    await FlutterBluePlus.isScanning.where((val) => val == false).first;
    print("Scan Complete");
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    FlutterBluePlus.stopScan();
    try {
      await device.connect();
    } on PlatformException catch (e) {
      if (e.code != 'already_connected') {
        rethrow;
      }
    }
    _services = await device.discoverServices();
    for (var service in _services) {
      if (service.uuid.toString() == serviceUuid) {
        for (var characteristic in service.characteristics) {
          if (characteristic.uuid.toString() == stepCharacteristicUuid) {
            stepCharacteristic = characteristic;
          } else if (characteristic.uuid.toString() == activityCharacteristicUuid) {
            activityCharacteristic = characteristic;
          }
        }
      }
    }

    _subscribeToServices();
  }

  void _subscribeToServices() {
    stepCharacteristic?.setNotifyValue(true);
    stepCharacteristic?.lastValueStream.listen((value) {
      if (value.isNotEmpty) {
        setState(() {
          stepCount = value[0];
        });
      }
    });
    
    activityCharacteristic?.setNotifyValue(true);
    activityCharacteristic?.lastValueStream.listen((value) {
      if (value.isNotEmpty) {
        setState(() {
          activityIndex = value[0];
        });
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.grey[300],
      appBar: AppBar(
        backgroundColor: Colors.transparent,
        elevation: 0,
        title: const Text(
          'Activity Dashboard',
          style: TextStyle(color: Colors.black, fontSize: 32),
        ),
        centerTitle: true,
      ),
      body: Padding(
        padding: const EdgeInsets.all(24.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [Expanded(child: _buildStatCard("Steps Taken", stepCount.toString()))],
            ),
            const SizedBox(height: 16),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [Expanded(child: _buildStatCard("Activity Status", activity[activityIndex]))],
            ),
            const SizedBox(height: 256),
          ],
        ),
      ),
    );
  }

  Widget _buildStatCard(String title, String value) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(10),
      ),
      padding: const EdgeInsets.all(16.0),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        crossAxisAlignment: CrossAxisAlignment.center,
        children: [
          Text(
            title,
            style: const TextStyle(fontSize: 14, fontWeight: FontWeight.w500),
            textAlign: TextAlign.center,
          ),
          const SizedBox(height: 20),
          Text(
            value,
            style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
          ),
        ],
      ),
    );
  }
}