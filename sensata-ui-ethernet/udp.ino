#include "udp.h"

using namespace qindesign::network;

void udpSetup()
{
    // Get Teensy mac address
    uint8_t mac[6];
    Ethernet.macAddress(mac); // This is informative; it retrieves, not sets
    printf(
        "MAC = %02x:%02x:%02x:%02x:%02x:%02x\r\n", mac[0], mac[1], mac[2],
        mac[3], mac[4], mac[5]);

    // Listen for link changes
    Ethernet.onLinkState([](bool state)
                         {
        if (state) {
            printf(
                "[Ethernet] Link ON: %d Mbps, %s duplex, %s crossover\r\n",
                Ethernet.linkSpeed(),
                Ethernet.linkIsFullDuplex() ? "full" : "half",
                Ethernet.linkIsCrossover() ? "is" : "not"
            );
        } else {
            printf("[Ethernet] Link OFF\r\n");
        } });

    // Begin Ethernet connection with static IP address
    Ethernet.setDHCPEnabled(false);
    Ethernet.begin(teensyIP, subnet, gateway);

    // Print Teensy IP
    IPAddress ip = Ethernet.localIP();
    printf("    Local IP     = %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);

    while(1) {
      if (!Ethernet.waitForLink(500))
      {
          printf("Failed to get link. Retrying ... \r\n");
      }
      else
      {
          printf("Link established \r\n");
          break;
      }
    }

    // Start UDP server
    udp.begin(kPort);
}

void udpSend(const std::string &message)
{
    udp.beginPacket(remoteIP, kPort);
    udp.print(message.c_str());
    udp.endPacket();
    udpCommsData = SD.open(filename, FILE_WRITE);
    if(udpCommsData) {
      udpCommsData.print(millis());
      udpCommsData.print(", ");      
      udpCommsData.println(message.c_str());
    }
    udpCommsData.close();
}

void sendSensorValues()
{
    std::string message = "DAT";
    if (sensataStream)
        sensataToString(message);
    if (thermoStream)
        thermocoupleToString(message);
    if (sensataStream || thermoStream)
        udpSend(message);
}

void sendValveStates()
{
  std::string message = "VDW";
  for(size_t i = 0; i < numValves; i++) {
    message += std::to_string(i) + std::to_string(valveStates[i]);
  }
  udpSend(message);
}