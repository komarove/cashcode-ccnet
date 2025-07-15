uint8_t CashCodeReset[] = {0x02, 0x03, 0x06, 0x30, 0x41, 0xB3};
uint8_t CashCodeAck[] = {0x02, 0x03, 0x06, 0x00, 0xC2, 0x82};
uint8_t CashCodePoll[] = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};
uint8_t CashCodeEnable[] = {0x02, 0x03, 0x0C, 0x34, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x66, 0xC1};

unsigned long timeInterval = 500;
unsigned long lastTime = 0;

uint32_t money_time = 0;
// Пины для UART2 на ESP32
#define RXD2 16 // RX пин для Serial2
#define TXD2 17 // TX пин для Serial2

void setup() {

  Serial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  delay(5000);
  
  Serial.write(CashCodeReset, sizeof(CashCodeReset));

  delay(12000);

  Serial.write(CashCodeEnable, sizeof(CashCodeEnable));
  delay(3000);

  while(Serial.available()){
    // TODO read errors and ready status
    Serial.read(); // Clear buffer
  }
}

void loop() {
  if(millis() < lastTime) lastTime = 0;
  if(millis() - lastTime > timeInterval || lastTime == 0){
    lastTime = millis();

    Serial.write(CashCodePoll, sizeof(CashCodePoll));

    delay(100);

    uint8_t byteData[6];

    if(Serial.available()) {
      // Read first 3 byte
      byteData[0] = Serial.read(); // always 0x02
      byteData[1] = Serial.read(); // perefery address for CCNET 0x03
      byteData[2] = Serial.read(); // count byte in sended paket (we can get byet count from hear, by defalt we read only 6: byteData[6])

      // if first two byte are ok read other byets
      if(byteData[0] == 0x02 && byteData[1] == 0x03) {
          byteData[3] = Serial.read(); // command
          byteData[4] = Serial.read(); // CRC (controll sum) / Data
          byteData[5] = Serial.read(); // CRC (controll sum)

          // if pakedge more then 6 byet use loop to read all
          while(Serial.available()){
            Serial.read(); // Clear buffer
          }

          // Wait for money
          if(byteData[3] == 0x14) {
            money_time = millis(); // TODO make reset if time more than 15 sec?
          }

          // Correct read money
          else if(byteData[3] == 0x81) {
            Serial.write(CashCodeAck, sizeof(CashCodeAck));


            // byteData[4] TODO display data, for example 0x01 = 10 uah
          }

          // if error codes reset
          else if(byteData[3] == 0x10 || byteData[3] == 0x11 || byteData[3] == 0x12 || byteData[3] == 0x46){
            Serial.write(CashCodeReset, sizeof(CashCodeReset));
            delay(5000);

            Serial.write(CashCodeEnable, sizeof(CashCodeEnable));
            delay(3000);

            Serial.write(CashCodePoll, sizeof(CashCodePoll));

          }
      }
      // resive wrong data
      else{
        while(Serial.available()){
          Serial.read(); // Clear buffer
        }
      }

      Serial.write(CashCodeAck, sizeof(CashCodeAck)); // just in case send ack to confirm that we resive data
    }
  }
}