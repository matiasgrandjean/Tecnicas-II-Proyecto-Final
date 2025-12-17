#include <SoftwareSerial.h>

#define BAUDRATE 38400

SoftwareSerial SerialRaspi(10, 11); // pin 10 (RX) - pin 11 (TX)

void setup() {
  Serial.begin(BAUDRATE);
  SerialRaspi.begin(BAUDRATE);     // UART hacia Raspberry

  Serial.println("Puente serie Arduino <-> Raspberry listo.");
}

void loop() {
  // PC -> Raspberry
  if (Serial.available()) {
    char c = (char)Serial.read();
    SerialRaspi.write(c);
  }

  // Raspberry -> PC
  if (SerialRaspi.available()) {
    char c = (char)SerialRaspi.read();
    Serial.write(c);
  }
}
