# Teensy Sensata Pressure Reader

Firmware for a Teensy-based sensor measurement system using pressure and temperature sensors, with extra code for Ethernet handling.

## Features
- Reads up to 5 digital/analog current‐output pressure sensors (P0–P4)  
- Converts raw ADC counts → voltage → current (mA) → pressure (psi)  
- Configurable sensor ranges via `#define` constants  
- Prints comma‐separated readings over USB serial  
- Easily extendable mapping and correction functions  

## Hardware
- **Board:** Teensy 4.1  
- **Sensors:** 5 digital/analog pressure sensors 
- **Wiring:**  
  - P0 → A18  
  - P1 → A19  
  - P2 → A24  
  - P3 → A25  
  - P4 → A26  
- **Sense resistor:** 220 Ω between sensor output and ADC  
- Look into other code files for wiring up temperature sensors and other configurations

## Build & Upload
Install the Arduino IDE with Teensyduino, then compile and load the code.

### Made for Yale Project Liquid
