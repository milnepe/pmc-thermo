#include "Arduino.h"
#include "RPC.h"
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Arduino_MachineControl.h>
using namespace machinecontrol;
using namespace rtos;

#define IOCH01 1
#define TRIP 25.0f  // Temperature threshold

float T1, T2, T3;

// Update these with values suitable for your network.
byte mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 61);  // PMC IP
IPAddress server(192, 168, 1, 60);  // Broker IP

EthernetClient ethClient;
PubSubClient client(server, 1883, ethClient);

// RPC callback on M7 - copies temperature readings into buffers
// Publish results over MQTT
float copyTemperatureOnM7(float temp_ch0, float temp_ch1, float temp_ch2) {
  T1 = temp_ch0;
  T2 = temp_ch1;
  T3 = temp_ch2;
  Serial.println("M7: TemperatureOnM7 T1 " + String(T1) + " T2 " + String(T2) + " T3 " + String(T3));
  client.publish("test/thermos", ("PMC0," + String(T1) + "," + String(T2) + "," + String(T3)).c_str());
  return 1;  // Note: must return a value!
}

void process_readings() {
  if ((T1 > TRIP) || (T2 > TRIP) || (T3 > TRIP)) {
    digital_outputs.set(IOCH01, HIGH);
  } else {
    digital_outputs.set(IOCH01, LOW);
  }
}

void setup() {
  RPC.begin();  // Init RPC, also boots M4 core
  Serial.begin(115200);
  // Binds RPC "copyTemperature" call on M4 to copyTemperatureOnM7 on M7
  RPC.bind("copyTemperature", copyTemperatureOnM7);

  Ethernet.begin(mac, ip);
  digital_outputs.setLatch();
  digital_outputs.set(IOCH01, LOW);
  client.connect("pmcClient0");
}

void loop() {
  client.loop();

  // Process temperature readings and do some work
  process_readings();
}