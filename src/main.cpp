#include <Arduino.h>
#include <FastLED.h>
#include <helper.h>

#include <artnetESP32/ArtnetESP32.h>
FASTLED_USING_NAMESPACE

IPAddress ipaddr = IPAddress(192,168,0,53);
IPAddress gateway = IPAddress(192,168,0,101);
IPAddress subnet = IPAddress(255,255,255,0);

uint16_t pixelsPerUni = UNIVERSE_SIZE; //
uint8_t startUniverse = START_UNIVERSE; //
uint8_t universesCount = UNIVERSES_COUNT; //
uint16_t pixelCount = pixelsPerUni * universesCount;

uint8_t uniData[512];
uint8_t headerData[18]; //artnetHeader
int16_t lostPackets = 0;

//for syncCount
uint32_t msyncmax, msync; 

//for syncTime
bool allowShow = false;
long lastPacketTime = 0;

CRGB leds[960]; //840 is maximum for 8 universes with 120 pixels each
WiFiUDP udp;

void fillFastLed();

void connectWiFi() {
  WiFi.mode(WIFI_STA);

    Serial.printf("Connecting ");
    WiFi.begin("udp", "esp18650");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.println(WiFi.status());

        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void beginLan() {
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  ETH.config(ipaddr, gateway, subnet);
}

void setup() {
  Serial.begin(115200);
  beginLan();
  //connectWiFi();
  //  artNetYvesConfig();
   udp.begin(6454);
  fillFastLed();
      msyncmax=(1<<universesCount)-1;
      msync=0;
}

uint32_t unisCount = 0;
void readUdp() {
  if(udp.parsePacket()) {
    udp.read(headerData, 18);
    int uniSize = (headerData[16] << 8) + headerData[17];
    uint8_t universe = headerData[14];
    if(universe >= startUniverse && universe < (startUniverse + universesCount) && uniSize > 500) {
      printf("univ: %d, time: %lumcs\n", universe, micros() - lastPacketTime);
      unisCount++;
      lastPacketTime = micros();
      allowShow = true;
      int offset = (universe - startUniverse)*pixelsPerUni;
      udp.read((uint8_t *)(leds+offset), pixelsPerUni*3);
    
      msync=msync | (1<<(universe - startUniverse));
               if(universe==startUniverse)
         {
          msync=1;
         }

    }
    udp.flush();
  }
}

//sends data to strips when packet for all universes received
void showSyncCount() {
           if(msync==msyncmax) {
           long tshow1 = micros();
           FastLED.show();
           long tshow2 = micros();
           printf("***** show: %lu micros\n", tshow2-tshow1);
           msync=0;
         }
}

//sends data to strips after passing some time after last packet received
void showSyncTime() {
           if(allowShow && (((micros() - lastPacketTime) > SHOW_DELAY) || unisCount == universesCount)) {
             allowShow = false;
          //  long tshow1 = micros();
           FastLED.show();
          //  printf("***** show: %lu micros\n", tshow2-tshow1);
                     printf("***** show:\n");

          //  long tshow2 = micros();
          //  int diff = universesCount-unisCount;
          //  if(diff != 0 || diff != universesCount)
          //  printf("diff: %d\n", universesCount-unisCount);
            //  lostPackets = lostPackets + (universesCount-unisCount);
          //  printf("***** show: %lu micros, lost: %d\n", tshow2-tshow1, lostPackets);
           unisCount = 0;
         }
}

void loop() {
  //MyLan
  readUdp();
  // showSyncCount();
  showSyncTime();
}

void fillFastLed() {
  switch (universesCount)
  {
  case 8:
    FastLED.addLeds<WS2813, 4, RGB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 7:
    FastLED.addLeds<WS2813, 2, RGB>(leds, 6*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 6:
    FastLED.addLeds<WS2813, 15, RGB>(leds, 5*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 5:
    FastLED.addLeds<WS2813, 32, RGB>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 4:
    FastLED.addLeds<WS2813, 33, RGB>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 3:
    FastLED.addLeds<WS2813, 14, RGB>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 2:
    FastLED.addLeds<WS2813, 12, RGB>(leds, 1*UNIVERSE_SIZE, UNIVERSE_SIZE);
  case 1:
    FastLED.addLeds<WS2813, 13, RGB>(leds, 0, UNIVERSE_SIZE);
    break;
  default:
    FastLED.addLeds<WS2813, 13, RGB>(leds, 0, UNIVERSE_SIZE);
    break;
  }
}