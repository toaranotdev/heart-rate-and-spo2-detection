#include <PeakDetection.h>

#include "MAX30105.h"

MAX30105 particleSensor;
PeakDetection peakDetector;

void setup() {
  Serial.begin(9600);

  particleSensor.begin(Wire, I2C_SPEED_FAST);
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);

  peakDetector.begin(32, 1, 0.5);
}

void loop() {
  int irValue = particleSensor.getIR();
  Serial.println(irValue);
}
