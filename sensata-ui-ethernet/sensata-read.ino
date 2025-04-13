#include <Arduino.h>
#include <vector>
#include <string>

#define P0_PIN 18
#define P0_IMIN 4.0f
#define P0_IMAX 20.0f
#define P0_PMIN 0.0f
#define P0_PMAX 1000.0f

#define P1_PIN 19
#define P1_IMIN 4.0f
#define P1_IMAX 20.0f
#define P1_PMIN 0.0f
#define P1_PMAX 1000.0f

#define P2_PIN 24
#define P2_IMIN 4.0f
#define P2_IMAX 20.0f
#define P2_PMIN 0.0f
#define P2_PMAX 1000.0f

#define P3_PIN 25
#define P3_IMIN 4.0f
#define P3_IMAX 20.0f
#define P3_PMIN 0.0f
#define P3_PMAX 1000.0f

#define P4_PIN 26
#define P4_IMIN 4.0f
#define P4_IMAX 20.0f
#define P4_PMIN 0.0f
#define P4_PMAX 1000.0f

#define CURRENT_SENSE_RESISTOR 220.0f
#define VREF 3.3f
#define COUNTS 1023.0f

typedef struct {
  const char* name;
  uint8_t     pin;
  float       i_min; // min current (mA)
  float       i_max; // max current (mA)
  float       p_min; // min psi
  float       p_max; // max psi
} AnalogCurrentSensor;

std::vector<AnalogCurrentSensor> sensatas = {
  { "P0", P0_PIN, P0_IMIN, P0_IMAX, P0_PMIN, P0_PMAX },
  { "P1", P1_PIN, P1_IMIN, P1_IMAX, P1_PMIN, P1_PMAX },
  { "P2", P2_PIN, P2_IMIN, P2_IMAX, P2_PMIN, P2_PMAX },
  { "P3", P3_PIN, P3_IMIN, P3_IMAX, P3_PMIN, P3_PMAX },
  { "P4", P4_PIN, P4_IMIN, P4_IMAX, P4_PMIN, P4_PMAX }
};

std::vector<float> sensataReadings(sensatas.size());

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float correctPressure(float rawPressure) {
  return rawPressure;
}

// void setup() {
//   Serial.begin(115200);
//   analogReadAveraging(8);
// }

void readSensatas() {
  for (size_t i = 0; i < sensatas.size(); i++) {
    int raw = analogRead(sensatas[i].pin);
    float voltage = raw * (VREF / COUNTS);              // Convert ADC to voltage
    float current_mA = (voltage / CURRENT_SENSE_RESISTOR) * 1000.0f; // Voltage to current in mA
    float pressure = mapFloat(
      current_mA,
      sensatas[i].i_min,
      sensatas[i].i_max,
      sensatas[i].p_min,
      sensatas[i].p_max
    );
    pressure = correctPressure(pressure);
    sensataReadings[i] = pressure;
  }
}

void sensataToString(std::string &message) {
  for (size_t i = 0; i < sensataReadings.size(); i++) {
    const char* name = sensatas[i].name;
    char buffer[20];
    sprintf(buffer, "%.2f", sensataReadings[i]);
    message += name;
    message += buffer;
  }
}

void printReadings() {
  std::string msg;
  msg.reserve(sensatas.size() * 8);
  sensataToString(msg);
  Serial.println(msg.c_str());
}

// void loop() {
//   readSensatas();
//   printReadings();
//   delay(500);
// }

