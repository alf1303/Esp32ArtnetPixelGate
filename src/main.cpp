//platformio run -t upload --upload-port 192.168.0.41
//uploading by OTA command

#include <Arduino.h>
#include <FastLED.h>
#include <helper.h>
// #include <WebServer.h>

#include <artnetESP32/ArtnetESP32.h>
FASTLED_USING_NAMESPACE
#define NO_SIGNAL_PERIOD 7000

#define UNIVERSES_COUNT 5 //4
#define START_UNIVERSE 31 ///////////////////////////    UNIVERSE AND LAST IN IP

IPAddress ipaddr = IPAddress(192,168,0,START_UNIVERSE);
IPAddress gateway = IPAddress(192,168,0,101);
IPAddress subnet = IPAddress(255,255,255,0);

uint16_t pixelsPerUni = UNIVERSE_SIZE; //
uint8_t startUniverse = START_UNIVERSE; //
uint8_t universesCount = UNIVERSES_COUNT; //
uint16_t pixelCount = pixelsPerUni * universesCount;

uint8_t uniData[512];
uint8_t headerData[18]; //artnetHeader
long lostPackets = 0;
// long lastPacketTime = 0;

//for syncCount
uint32_t msyncmax, msync; 

//for syncTime
bool allowShow = false;
long lastPacketTime = 0;

//for blackout if no signal
long lastSignalTime = 0;
bool noSignal = false;

CRGB leds[850]; //840 is maximum for 8 universes with 120 pixels each
WiFiUDP udp;

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

void test() {
 fill_solid(leds, 850, CRGB(0, 0, 10));
 FastLED.show();
 delay(100);
 fill_solid(leds, 850, CRGB::Black);
 FastLED.show();
 delay(200);
}

void testlong() {
  for(int i = 0; i < 1; i++) {
    fill_solid(leds, 850, CRGB(10, 0, 0));
    FastLED.show();
    delay(5000);
    fill_solid(leds, 850, CRGB(0, 10, 0));
    FastLED.show();
    delay(5000);
    fill_solid(leds, 850, CRGB(0, 0, 10));
    FastLED.show();
    delay(5000);
  }
}

void test2() {
  uint8_t size = 24;
  uint16_t testLeds[size] = {0,1,118,119,120,121,238,239,240,241,358,359,360,361,478,479,480,481,598,599,600,601,718,719};
  for(int i = 0; i < size; i++) {
    leds[testLeds[i]].setRGB(0, 0, 50);
  }
  FastLED.show();  
  delay(100);
  fill_solid(leds, 850, CRGB::Black);
  FastLED.show();
  delay(200);
}

void performTest(void (*f)(void)) {
  f();
  f();
  f();
}

void setup() {
  Serial.begin(115200);
  delay(10);
  //connectWiFi();
  //artNetYvesConfig();
  beginLan();
  udp.begin(6454);
  fillFastLedNewUniversal(); // OLD version pinout
  // fillFastLedNewUniversal(); // NEW versio pinout, universal output
  // fillFastLedNew(); // NEW version pinout
  // fillFastLedDouble(); //for double universes strips OLD version
  // fillFastLedDoubleNew(); //for double universes strips NEW version
      msyncmax=(1<<universesCount)-1;
      msync=0;

  FastLED.clear();
  // performTest(testlong);
  testlong();
  OTA_Func();
}

uint32_t unisCount = 0;
void readUdp() {
  if(udp.parsePacket()) {
    udp.read(headerData, 18);
    int uniSize = (headerData[16] << 8) + headerData[17];
    uint8_t universe = headerData[14];
    if(universe >= startUniverse && universe < (startUniverse + universesCount) && uniSize > 500) {
      printf("univ: %d, time: %lumcs\n", universe, micros() - lastPacketTime);
      if(noSignal) noSignal = false;
      unisCount++;
      lastPacketTime = micros();
      lastSignalTime = millis();
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
           if(allowShow && (unisCount == universesCount || ((micros() - lastPacketTime) > SHOW_DELAY))) {
             allowShow = false;
          //  long tshow1 = micros();
          FastLED.setBrightness(215);
           FastLED.show();
          //  printf("***** show: %lu micros\n", tshow2-tshow1);
          Serial.println("******** Show");
          if(unisCount != UNIVERSES_COUNT) {
            lostPackets++;
            printf("***** lost: %d, packets loosed: %lu\n", unisCount, lostPackets);
          }
          
          //  long tshow2 = micros();
          //  int diff = universesCount-unisCount;
          //  if(diff != 0 || diff != universesCount)
          //  printf("diff: %d\n", universesCount-unisCount);
            //  lostPackets = lostPackets + (universesCount-unisCount);
          //  printf("***** show: %lu micros, lost: %d\n", tshow2-tshow1, lostPackets);
           unisCount = 0;
         }
}

void checkNoSignal() {
  if(!noSignal && ((millis() - lastSignalTime) > NO_SIGNAL_PERIOD)) {
    noSignal = true;
    fill_solid(leds, 850, CRGB(0, 0, 0));
    FastLED.show();
  }
}

void loop() {
  //MyLan
  ArduinoOTA.handle();
  readUdp();
  checkNoSignal();
  // showSyncCount();
  showSyncTime();
}

//ADDITIONAL PINS 1 3 0
//    FastLED.addLeds<UCS1903, 15, GRB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE); //*1
//FastLED.addLeds<WS2813, 15, GRB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE); // 

// PINOUT FOR NEW VERSION OF PLATA. 5 OUTPUT UNIVERSAL 2 + 1 + 1 + 1
// ON Board use output  1 for universes 0,1 output 3 for uni2, output 4 for uni3, output 5 for uni4 
void fillFastLedNewUniversal() {
  switch (universesCount)
  {
  case 5:
  // ws2811 UCS1904 *UCS1903*   (WS2811, WS2852 UCS1904Controller800Khz?, UCS2903?)
    // FastLED.addLeds<WS2811, 5, GRB>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE); // 5
    FastLED.addLeds<WS2811, 17, GRB>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE); // 17
    FastLED.addLeds<WS2811, 4, GRB>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE); // 4
    // FastLED.addLeds<WS2811, 12, GRB>(leds, 1*UNIVERSE_SIZE, UNIVERSE_SIZE); // 12
    FastLED.addLeds<WS2811, 14, GRB>(leds, 0, 2*UNIVERSE_SIZE); // 14
    break;
  default:
    FastLED.addLeds<WS2811, 14, GRB>(leds, 0, 2*UNIVERSE_SIZE);
    break;
  }
}

/// PINOUT FOR OLD VERSION KIEV-KLUB 5 OUTPUTS ONLY
void fillFastLed() {
  switch (universesCount)
  {
  case 8:
    FastLED.addLeds<UCS1903, 15, GRB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 7:
    FastLED.addLeds<UCS1903, 14, GRB>(leds, 6*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 6:
    FastLED.addLeds<UCS1903, 12, GRB>(leds, 5*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 5:
    FastLED.addLeds<UCS1903, 4, GRB>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 4:
    FastLED.addLeds<UCS1903, 17, GRB>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 3:
    FastLED.addLeds<UCS1903, 5, GRB>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 2:
    FastLED.addLeds<UCS1903, 33, GRB>(leds, 1*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 1:
    FastLED.addLeds<UCS1903, 32, GRB>(leds, 0, UNIVERSE_SIZE); //
    break;
  default:
    FastLED.addLeds<UCS1903, 32, GRB>(leds, 0, UNIVERSE_SIZE);
    break;
  }
}

// PINOUT FOR NEW VERSION OF PLATA. 5 OUTPUT
void fillFastLedNew() {
  switch (universesCount)
  {
  case 8:
    FastLED.addLeds<UCS1903, 15, GRB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 7:
    FastLED.addLeds<UCS1903, 32, GRB>(leds, 6*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 6:
    FastLED.addLeds<UCS1903, 33, GRB>(leds, 5*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 5:
    FastLED.addLeds<UCS1903, 5, GRB>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 4:
    FastLED.addLeds<UCS1903, 17, GRB>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 3:
    FastLED.addLeds<UCS1903, 4, GRB>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 2:
    FastLED.addLeds<UCS1903, 12, GRB>(leds, 1*UNIVERSE_SIZE, UNIVERSE_SIZE); // 
  case 1:
    FastLED.addLeds<UCS1903, 14, GRB>(leds, 0, UNIVERSE_SIZE); //
    break;
  default:
    FastLED.addLeds<UCS1903, 32, GRB>(leds, 0, UNIVERSE_SIZE);
    break;
  }
}

//PINOUT FOR DOUBLE OLD
void fillFastLedDouble() {
    FastLED.addLeds<UCS1903, 33, GRB>(leds, 2*UNIVERSE_SIZE, 2*UNIVERSE_SIZE); // 
    FastLED.addLeds<UCS1903, 32, GRB>(leds, 0, 2*UNIVERSE_SIZE); //
}

//PINOUT FOR DOUBLE NEW
void fillFastLedDoubleNew() {
    FastLED.addLeds<UCS1903, 12, GRB>(leds, 2*UNIVERSE_SIZE, 2*UNIVERSE_SIZE); // 
    FastLED.addLeds<UCS1903, 14, GRB>(leds, 0, 2*UNIVERSE_SIZE); //
}

//32 33 5 17 4 12 14 15 

// void fillFastLed() {
//   switch (universesCount)
//   {
//   case 8:
//     FastLED.addLeds<WS2813, 4, RGB>(leds, 7*UNIVERSE_SIZE, UNIVERSE_SIZE); //*1
//   case 7:
//     FastLED.addLeds<WS2813, 2, RGB>(leds, 6*UNIVERSE_SIZE, UNIVERSE_SIZE); //*16
//   case 6:
//     FastLED.addLeds<WS2813, 15, RGB>(leds, 5*UNIVERSE_SIZE, UNIVERSE_SIZE); //*34
//   case 5:
//     FastLED.addLeds<WS2813, 32, RGB>(leds, 4*UNIVERSE_SIZE, UNIVERSE_SIZE); //35
//   case 4:
//     FastLED.addLeds<WS2813, 33, RGB>(leds, 3*UNIVERSE_SIZE, UNIVERSE_SIZE); //32
//   case 3:
//     FastLED.addLeds<WS2813, 14, RGB>(leds, 2*UNIVERSE_SIZE, UNIVERSE_SIZE); //33
//   case 2:
//     FastLED.addLeds<WS2813, 12, RGB>(leds, 1*UNIVERSE_SIZE, UNIVERSE_SIZE); //* 14
//   case 1:
//     FastLED.addLeds<WS2813, 13, RGB>(leds, 0, UNIVERSE_SIZE); //13
//     break;
//   default:
//     FastLED.addLeds<WS2813, 13, RGB>(leds, 0, UNIVERSE_SIZE);
//     break;
//   }
// }

  //OTA - Flashing over Air
void OTA_Func() {
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  }