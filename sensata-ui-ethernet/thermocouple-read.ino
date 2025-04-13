#include <iostream>
#include <vector>

std::vector<std::tuple<std::string, int>> thermocouples = {
    std::make_tuple("T0", A7),
    std::make_tuple("T1", A8),
    std::make_tuple("T2", A9),
};

std::vector<float> thermocoupleReadings(thermocouples.size());

float voltToTemp(float v) {
    float temperature =
        ((6.71061387 * pow(0.1, 6) * pow(v, 4)) +
         (8.99614883 * pow(0.1, 4) * pow(v, 3)) +
         (-7.62504049 * pow(0.1, 2) * pow(v, 2)) +
         (2.53919375 * pow(10, 1) * pow(v, 1)) + (-1.29329394));
    return temperature;
}

float analogToTemperature(int input) {
    float voltage     = 1000 * (((0.003225) * input) / 47);
    float temperature = voltToTemp(voltage);
    return temperature;
}

void readThermocouples() {
    for (size_t i = 0; i < thermocouples.size(); i++) {
        std::tuple<std::string, int> thermocouple = thermocouples[i];

        uint8_t readPin         = std::get<1>(thermocouple);
        float analogReading     = (float)analogRead(readPin);
        float temperature       = analogToTemperature(analogReading);
        thermocoupleReadings[i] = temperature;
    }
}

void thermocoupleToString(std::string &message) {
    for (size_t i = 0; i < thermocoupleReadings.size(); i++) {
        std::tuple<std::string, int> thermocouple = thermocouples[i];

        std::string name = std::get<0>(thermocouple);
        char buffer[20];
        sprintf(buffer, "%.2f", thermocoupleReadings[i]);
        message += name;
        message += buffer;
    }
}
