#define SCAN_TIME  30 // seconds
#define MAXDEVICES 4  // max devices to find

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "bmp.h"
#include <string>
using namespace std;


#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define TFT_MOSI            19
#define TFT_SCLK            18
#define TFT_CS              5
#define TFT_DC              16
#define TFT_RST             23

#define TFT_BL          4  // Display backlight control pin
#define ADC_EN          14
#define BUTTON_1        35
#define BUTTON_2        0

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);

char buff[512];
int btnCick = false;

int i = 1;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
      //if (advertisedDevice.haveManufacturerData()){
      if (advertisedDevice.haveName()){
        String d = advertisedDevice.getAddress().toString().c_str();
        Serial.print(String("Adr: ") + d + " ");
        tft.print(String("Adr: ") + d + " ");

        if (advertisedDevice.haveName())
        {
            String  d = advertisedDevice.getName().c_str();
            Serial.print(String("Name: ") + d + " ");
            tft.print(String("Name: ") + d + " ");
        }

        if (advertisedDevice.haveServiceUUID())
        {
            String d = advertisedDevice.getServiceUUID().toString().c_str();
            Serial.print(String("UUID: ") + d + " ");
            tft.print(String("UUID: ") + d + " ");
        }

        if (advertisedDevice.haveTXPower())
        {
            String d = String(advertisedDevice.getTXPower());
            Serial.print(String("TXPower: ") + d + " ");
            tft.print(String("TXPower: ") + d + " ");
        } 

        Serial.println();
        tft.println();
        
        std::string md = advertisedDevice.getManufacturerData();
        uint8_t* mdp = (uint8_t*)advertisedDevice.getManufacturerData().data();
        char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
        Serial.print(String("Data: ") + pHex + " ");
        tft.print(String("Dat: ") + pHex + " ");
        free(pHex);
        
        Serial.println();
        tft.println();
        tft.println();
        i += 1;
        if (i > MAXDEVICES) {
          advertisedDevice.getScan()->stop();
          //i = 1;
        }
      }
    }
};

void button_init()
{
    btn1.setPressedHandler([](Button2 & b) {
        btnCick = false;
        Serial.println("BLE Scan");
        ble_scan();
    });

    btn2.setPressedHandler([](Button2 & b) {
        btnCick = false;
        Serial.println("btn press wifi scan");
        wifi_scan();
    });
}

void button_loop()
{
    btn1.loop();
    btn2.loop();
}

void ble_scan()
{
  BLEDevice::init("");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(1);

  tft.drawString("BLE Scan in progress", tft.width() / 2, tft.height() / 2);
  delay(100);

  BLEScan *pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);

  Serial.printf("Start BLE scan for %d seconds...\n", SCAN_TIME);

  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);
  int count = foundDevices.getCount();
  Serial.println("Count: " + String(count));
  if (count == 0 || i == 1) {
      Serial.println("no BLE Devices found");
      tft.fillScreen(TFT_BLACK);
      tft.setTextDatum(MC_DATUM);
      tft.setTextDatum(TL_DATUM);
      tft.setTextSize(1);
      tft.setCursor(0, 0);
      tft.drawString("no BLE Devices found", tft.width() / 4, tft.height() / 2);
  }
  
}

void wifi_scan()
{
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    tft.drawString("Scan Network", tft.width() / 2, tft.height() / 2);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int16_t n = WiFi.scanNetworks();
    tft.fillScreen(TFT_BLACK);
    if (n == 0) {
        tft.drawString("no networks found", tft.width() / 2, tft.height() / 2);
    } else {
        tft.setTextDatum(TL_DATUM);
        tft.setCursor(0, 0);
        Serial.printf("Found %d net\n", n);
        for (int i = 0; i < n; ++i) {
            sprintf(buff,
                    "[%d]:%s(%d)",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i));
            tft.println(buff);
        }
    }
    WiFi.mode(WIFI_OFF);
}

void setup()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    BLEDevice::init("");
    
    Serial.begin(115200);
    delay(1000);

    Serial.println("Start");
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);

    if (TFT_BL > 0) {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH);
    }
    tft.setSwapBytes(true);
    tft.pushImage(0, 0,  240, 135, ttgo);
    delay(5000);

    button_init();
}



void loop()
{
    if (btnCick) {
        ble_scan();
    }
    button_loop();
}
