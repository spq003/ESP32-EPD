#include "ESP32epdx.h"  //E-paper SPI etc
#include "EPD.h"        //E-paper driver
#include "IMAGE.h"      //E-paper image data
#include <BluetoothSerial.h>
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
unsigned char RX_BUF[EPD_ARRAY];  // 15000*8(char==8bit) == 400*300=120000
int received = 0;
bool receiving = false;

#define EPD_STATUS 0

void update_screen() {
  #if EPD_STATUS == 1
  EPD_HW_Init();
  EPD_WhiteScreen_ALL(RX_BUF);
  EPD_DeepSleep();
  #endif

  Serial.printf("[EPD] Screen Update Start...\n");
  for(int i = 0; i < (EPD_ARRAY / 20); i++){
    for(int j = 0; j < 20; j++){
      Serial.printf("0x%02X, ", RX_BUF[(20*i)+j]);
    }
    Serial.printf("\n");
  }
  Serial.printf("[EPD] Screen Update End.\n");
}

void setup() {
  /* ESP32-WROOM-32D (Using hardware SPI)
   * BUSY: GPIO13    RES: GPIO12    DC: GPIO14    CS: GPIO27    SCK: GPIO18    SDIN:GPIO23
   *
   * 풀스크린 업데이트 전 Init 호출 필수
   * 풀스크린 업데이트 중 화면 깜빡임은 정상
   * 화면 업데이트 이후에는 !!반드시!! 슬립모드로 설정
   * 전원 인가 중 FPC 커넥터 분리 금지
   */

  pinMode(13, INPUT);   // BUSY
  pinMode(12, OUTPUT);  // RES
  pinMode(14, OUTPUT);  // DC
  pinMode(27, OUTPUT);  // CS

  SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
  SPI.begin();

  Serial.begin(115200);
  // Serial.begin(9600);
  SerialBT.begin("Evidence-EPD-01");  // Bluetooth device name
  esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
  Serial.println("The device started, now you can pair it with bluetooth!");

#if EPD_STATUS == 1
  EPD_HW_Init();
  EPD_WhiteScreen_ALL();
  EPD_DeepSleep();
#endif
}

void loop() {
#if EPD_STATUS == 1         // Full screen update, fast update, and partial update demostration.
  EPD_HW_Init();            // Full screen update initialization.
  EPD_WhiteScreen_White();  // Clear screen function.
  EPD_DeepSleep();          // Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
  delay(2000);              // Delay for 2s.
  /************Full display(2s)*******************/
  EPD_HW_Init();                  // Full screen update initialization.
  EPD_WhiteScreen_ALL(gImage_1);  // To Display one image using full screen update.
  EPD_DeepSleep();                // Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
  delay(2000);                    // Delay for 2s.
  /************Fast update mode(1.5s)*******************/
  EPD_HW_Init_Fast();                  // Fast update initialization.
  EPD_WhiteScreen_ALL_Fast(gImage_2);  // To display the second image using fast update.
  EPD_DeepSleep();                     // Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
  delay(2000);                         // Delay for 2s.
#endif

  if (SerialBT.hasClient()) {
    int avail = SerialBT.available();
    if (avail > 0) {
      if (!receiving) {
        Serial.println("RX Starting...");
        received = 0;
        receiving = true;
      }
      // Serial.printf("Available: %d bytes\n", avail);

      while (SerialBT.available() && received < EPD_ARRAY) {
        if(received >= 15000){
          Serial.printf("Packet Ignored. recived %dbytes.\n", received);
        }
        RX_BUF[received++] = SerialBT.read();
      }

      Serial.printf("Received: %d\n", received);

      if (received >= EPD_ARRAY) {
        Serial.printf("RX Complete. [%d] Bytes\n", received);
        update_screen();
        receiving = false;
      }
    }
  } else {
    delay(100);
  }
}
