#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include "udp.h"
#include <SD.h>

// Command name, time of command execution
std::vector<std::tuple<std::string, unsigned long>> commandSchedule;

// whether or not to transmit sensor data
bool sensataStream = true;
bool thermoStream = true;

unsigned long i2cTime = 0;
bool borked = false;

File udpCommsData;
const char filename[] = "udpCommsDataLog.txt";

// TODO: maybe only send sensor values on intervals

// all normally closed except ethane vent
/* V0 : Nitrous run
V1 : Nitrous vent //normally closed
V2 : Nitrous tank
V3 : Ethane run
V4 : Ethane vent // normally open
V5 : Ethane tank 
*/
int sparkPlugPin = 20;
const uint8_t numValves = 6;
int valvePins[numValves] = {0, 1, 2, 5, 6, 15};
int timeoutState[numValves] = {0, 0, 0, 0, 1, 0};
int valveStates[numValves] = {0, 0, 0, 0, 0, 0};
int sparkPlugState = 0;
int resetPin = 41;

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 4000)
    {
        // Wait for Serial
    }

    Serial.println("Teensy startup");

    udpSetup();

    // sensataSetup();

    for (size_t i = 0; i < numValves; i++) {
        pinMode(valvePins[i], OUTPUT);
    }
    
    pinMode(sparkPlugPin, OUTPUT);
    pinMode(resetPin, OUTPUT);

    digitalWrite(resetPin, HIGH);

    if (!SD.begin(BUILTIN_SDCARD))
    {
        error("SD card failed to initalize");
    } else { 
      udpCommsData = SD.open(filename, FILE_WRITE);
      if(udpCommsData) {
        udpCommsData.println("STARTING NEW LOG");
        Serial.println("No problems!");
      } else {
        error("No udpCommsData file was created :(");
      }
      udpCommsData.close();
    }
}

void loop()
{
    // Check if new commands received and insert into buffer
    // Execute immediately if necessary
    receivePacket();

    if(foundLaptop) {
      if((millis() - lastPacketTime > heartbeatTimeout) && !timedOut) {
        Serial.print("last packet time = ");
        Serial.println(lastPacketTime);
        Serial.print("current time = ");
        Serial.println(millis());
        lostConnectionSequence();
        timedOut = true;
      }
    }

    // if it's time to execute any commands , execute them
    executeScheduledCommands();

    if(i2cTime < 10) {
      if(sensataStream)
      {
        readSensatas();
        printReadings();
      }

      sendSensorValues();
    }
    else {
      if(!borked) {
        udpSend("BRK");
      }
      borked = true;
    }
}

void executeScheduledCommands()
{
    commandSchedule.erase(
        std::remove_if(
            commandSchedule.begin(), commandSchedule.end(),
            [](std::tuple<std::string, unsigned long> tuple)
            {
                unsigned long time = std::get<1>(tuple);
                if (time <= millis())
                {
                    std::string command = std::get<0>(tuple);
                    parseCommand(command);
                    return 1;
                }
                else
                {
                    return 0;
                }
            }),
        commandSchedule.end());
}

void parseCommand(std::string command)
{
    // Access and read the first command
    std::string code = command.substr(0, 3);
    std::string data = command.substr(3);
    // These are all the commands we want to execute immediately, even if we're
    // in a wait period
    bool success = false;
    if (code == "ECH")
    {
        //Serial.println("Alive!");
        udpSend(data);
        success = true;
    }
    else if (code == "CLR")
    {
        commandSchedule.clear();
        success = true;
    }
    else if (code == "TMP")
    {
        int start;
        if (!stringToInt(data, 0, 1, start) || (start != 0 && start != 1))
        {
            error("TMP stream command not valid: " + data);
        }
        else
        {
            thermoStream = start;
            success = true;
        }
    }
    else if (code == "SEN")
    {
        int start;
        if (!stringToInt(data, 0, 1, start) || (start != 0 && start != 1))
        {
            error("SEN stream command not valid: " + data);
        }
        else
        {
            sensataStream = start;
            success = true;
        }
    }
    else if (code == "PDW")
    {
        success = pinDigitalWrite(data);
    }
    else if (code == "VDW")
    {
        success = valveDigitalWrite(data);
    }
    else if (code == "SPK") // check to make sure command after spark is one character
    //TODO incorporate with sensor values
    {
        int val;
        if (!stringToInt(data, 0, 1, val) || (val != 0 && val != 1))
        {
            error("SPK on/off command not valid: " + data);
        }
        else
        {
            digitalWrite(sparkPlugPin, val);
            Serial.print("Spark plug to ");
            Serial.println(val);
            sparkPlugState = val;
            success = true;
        }
    }
    else if (code == "TIM") 
    {
      success = goToTimeoutState();
    }
    else if (code == "BUD") {
      Ethernet.begin(teensyIP, subnet, gateway);
      success = true;
    }
    else if (code == "SUD") {
      Ethernet.end();
      success = true;
    } else
    {
        error("Command includes unrecognized command sequence: " + code);
        success = false;
    }

    // Let laptop know that we're executing a command if it executed without error
    if (success)
    {
        udpSend(command);
    }
}

// Receives and prints chat packets
static void receivePacket()
{
    int size = udp.parsePacket();
    if (size < 0)
    {
        return;
    }

    // Get the packet data and set remote address
    const uint8_t *data = udp.data();
    remoteIP = udp.remoteIP();

    // reset heartbeat variables
    foundLaptop = true;
    if (timedOut) { // if this is a new connection 
      Serial.println("sending valve states");
      sendValveStates();
    }
    timedOut = false;
    lastPacketTime = millis();

    // Converting to stringstream
    const char *data_char_array = reinterpret_cast<const char *>(data);
    std::string data_str(
        data_char_array,
        std::min(strlen(data_char_array), static_cast<size_t>(size)));
    std::stringstream ss(data_str);
    std::string command;
    unsigned long executeTime = 0;
    while (std::getline(ss, command, ';'))
    {
        // if command = wait, increase executeTime
        std::string code = command.substr(0, 3);
        if (code == "WAI")
        {
            std::string message = command.substr(3);
            unsigned long duration = std::stoul(message);
            executeTime += duration * 10;
        }
        else
        {
            if (executeTime == 0)
            {
                parseCommand(command);
            }
            else
            {
                std::tuple<std::string, unsigned long> commandTime(
                    command, executeTime + millis());
                commandSchedule.push_back(commandTime);
            }
        }
    }
}

bool stringToInt(const std::string &data, size_t start, size_t end, int &out)
{
    for (size_t i = start; i < end; i++)
    {
        if (!std::isdigit(data[i]))
        {
            return false;
        }
    }
    out = stoi(data.substr(start, end - start)); // arguments are position, length
    return true;
}

bool pinDigitalWrite(const std::string &data)
{
    if (data.length() % 3 != 0 || data.length() == 0)
    {
        error("Command data incorrect length for PDW: " + data);
        return false;
    }

    std::vector<std::tuple<int, int, int>> pinCommands; // pin, valve, dataVal

    for (size_t pos = 0; pos < data.length(); pos += 3)
    {
        int pin;
        int dataVal;
        if (!stringToInt(data, pos, pos + 2, pin))
        {
            error("Invalid pin for PDW: " + data);
            return false;
        }
        if (!stringToInt(data, pos + 2, pos + 3, dataVal) ||
            (dataVal != 0 && dataVal != 1))
        {
            error("Invalid pin high/low for PDW: " + data);
            return false;
        }

        // Serial.println(pin);
        int valve;
        if (!inArray(pin, valvePins, numValves, valve)) // sets valve if in array
        {
            error("Command includes out-of-bounds pin: " + std::to_string(pin));
            return false;
        }

        std::tuple<int, int, int> pinCommand = std::make_tuple(pin, valve, dataVal);
        pinCommands.push_back(pinCommand);
    }

    for (const auto &pinCommand : pinCommands)
    {
        int pin = std::get<0>(pinCommand);
        int valve = std::get<1>(pinCommand);
        int dataVal = std::get<2>(pinCommand);
        digitalWrite((uint8_t)pin, dataVal);
        valveStates[valve] = dataVal;

        Serial.print("Set pin ");
        Serial.print((uint8_t)pin);
        Serial.print(" to ");
        Serial.println(dataVal);
    }

    return true;
}

bool valveDigitalWrite(const std::string &data)
{ // VDW
    size_t dataLength = data.length();
    if (dataLength % 2 != 0 || dataLength == 0)
    {
        error("Command data incorrect length for VDW: " + data);
        return false;
    }

    std::vector<std::tuple<int, int>> valveCommands;

    for (size_t pos = 0; pos < dataLength; pos += 2)
    {
        int valve;
        int dataVal;
        if (!stringToInt(data, pos, pos + 1, valve))
        {
            error("Invalid valve for VDW: " + data);
            return false;
        }
        if (!stringToInt(data, pos + 1, pos + 2, dataVal) ||
            (dataVal != 0 && dataVal != 1)) 
        {
            error("Invalid valve high/low for VDW: " + std::to_string(valve) + " " + std::to_string(dataVal)); // TODO ????
            return false;
        }

        if (!(valve >= 0 && valve <= 5))
        {
            error(
                "Command includes out-of-bounds valve: " + std::to_string(valve));
            return false;
        }

        std::tuple<int, int> valveCommand = std::make_tuple(valve, dataVal);
        valveCommands.push_back(valveCommand);
    }

    for (const auto &valveCommand : valveCommands)
    {
        int valve = std::get<0>(valveCommand);
        int dataVal = std::get<1>(valveCommand);
        digitalWrite((uint8_t)valvePins[valve], dataVal);
        valveStates[valve] = dataVal;

        Serial.print("Set pin ");
        Serial.print((uint8_t)valvePins[valve]);
        Serial.print(" to ");
        Serial.println(dataVal);
    }

    return true;
}

void lostConnectionSequence() {
  Serial.println("Running lost connection sequence");
  // if no commands, immediately go to timeout state
  if (commandSchedule.empty()) {
    goToTimeoutState();
    return;
  }

  // otherwise, identify time of last scheduled command
  unsigned long lastTime = millis();
  for (const auto &command : commandSchedule) {
    unsigned long time = std::get<1>(command);
    if (time > lastTime) { 
      lastTime = time;
    }
  }

  // schedule timeout state after all other commands are executed
  std::tuple<std::string, unsigned long> timeoutCommand("TIM", lastTime);
  commandSchedule.push_back(timeoutCommand);
}

bool goToTimeoutState() {
  Serial.println("Going to timeout state");
  for(size_t i = 0; i < numValves; i++) {
    digitalWrite(valvePins[i], timeoutState[i]);
    valveStates[i] = timeoutState[i];
  }
  digitalWrite(sparkPlugPin, 0);
  sparkPlugState = 0;
  return true;
}

bool inArray(int element, const int *array, size_t arraySize, int &valve)
{
    for (size_t i = 0; i < arraySize; i++)
    {
        if (element == array[i])
        {
            valve = i;
            return true;
        }
    }
    return false;
}

void error(std::string message)
{
    Serial.println(message.c_str());
    udpSend("ERR: " + message);
}