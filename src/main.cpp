#include <Arduino.h>
#include <FastLED.h>
#include <helper.h>

//#include <artnetESP32/ArtnetESP32.h>

const uint32_t universesCount = NUM_LEDS/UNIVERSE_SIZE;
uint8_t uniData[512];
uint8_t headerData[18]; //artnetHeader
uint32_t syncmax,sync; //for syncCount
int16_t lostPackets = 0;

//for syncTime
bool allowShow = false;
long lastPacketTime = 0;

CRGB leds[NUM_LEDS];

WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  udp.begin(6454);
    FastLED.addLeds<NEOPIXEL, 13>(leds, 0, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 12>(leds, UNIVERSE_SIZE, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 14>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 33>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 32>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 15>(leds, 5*UNIVERSE_SIZE, UNIVERSE_SIZE);
    FastLED.addLeds<NEOPIXEL, 2>(leds, 6*UNIVERSE_SIZE, UNIVERSE_SIZE);
    // FastLED.addLeds<NEOPIXEL, 4>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE);
      syncmax=(1<<universesCount)-1;
     // printf("setup syncmax: %d, univerCount: %d\n", syncmax, universesCount);
      sync=0;
}

uint32_t unisCount = 0;
void readUdp() {
  
  if(udp.parsePacket()) {
    udp.read(headerData, 18);
    int uniSize = (headerData[16] << 8) + headerData[17];
    uint8_t universe = headerData[14];
    if(universe >= START_UNIVERSE && universe < (START_UNIVERSE + universesCount) && uniSize > 500) {
      printf("univ: %d, time: %lumcs\n", universe, micros() - lastPacketTime);
      unisCount++;
      lastPacketTime = micros();
      allowShow = true;
      int offset = (universe - START_UNIVERSE)*UNIVERSE_SIZE;
      udp.read((uint8_t *)(leds+offset), UNIVERSE_SIZE*3);
      sync=sync | (1<<(universe - START_UNIVERSE));
               if(universe==START_UNIVERSE)
         {
          sync=1;
         }
          /* end of code **/

    }
    udp.flush();
  }
}

//sends data to strips when packet for all universes received
void showSyncCount() {
           if(sync==syncmax) {
           long tshow1 = micros();
           FastLED.show();
           long tshow2 = micros();
           printf("***** show: %lu micros\n", tshow2-tshow1);
           sync=0;
         }
}

//sends data to strips after passing some time after last packet received
void showSyncTime() {
           if(allowShow && (micros() - lastPacketTime) > SHOW_DELAY) {
             allowShow = false;
           long tshow1 = micros();
           FastLED.show();
           long tshow2 = micros();
           printf("diff: %d\n", universesCount-unisCount);
             lostPackets = lostPackets + (universesCount-unisCount);
           printf("***** show: %lu micros, lost: %d\n", tshow2-tshow1, lostPackets);
           unisCount = 0;
         }
}

void loop() {
  readUdp();
  //showSyncCount();
  showSyncTime();
}