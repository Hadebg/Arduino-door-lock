#include <Arduino.h>
#include <main.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP32Servo.h>

/*status = 0: New RFID card input
  status = 1: New password input
  status = 2: Waiting screen (temperature and humidity display)
  status = 3: Checking password or RFID card
  status = 4: To make sure that LCD is cleared and ready to show the entering password screen (sry if too complicated ;-;)
  status = 5: Lockdown
*/

//Sequence: Input new RFID card (status 0) -> input new password (status 1) -> check password or RFID card to unlock (status 2, 3, 4, 5)

//Actually I wanted to have RTC included in my project as well but my RTC is broken sooooo idk ;-;

LiquidCrystal_I2C lcd(0x27, 16, 2);  
MFRC522 CardReader(SS_PIN, RST_PIN); 
Servo DoorLocker;
DHT dht(DHTPIN, DHTTYPE);
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(9600);
  DoorLocker.attach(DoorLock_Pin);
  DoorLocker.write(Lock);
  pinMode(BuzzerPin, OUTPUT);
  lcd.init();
  lcd.backlight();
  SPI.begin();
  CardReader.PCD_Init();
  dht.begin();
  //Begin to scan new RFID card
  lcd.clear();
  lcd.print("Quet the moi!"); //(LCD: Scan new card!)
}

void loop() {
  customKey = customKeypad.getKey();
  if (status == 0) {
    InputCard();  
  }
  else if (status == 1){
    KeypadInput(); 
  } 
  else if (status == 2){
    CheckCard();
    checkKeypad();
    displayTempAndHumidity();
    Lockdown();
  }
  else if (status == 3){
    //To make sure that it is still working
    CheckCard();
    checkKeypad();
    Lockdown();
  }
}

// Clear previous entered password
void clearData() {
  while (data_count != 0) {
    Data[--data_count] = 0;
  }
}

// Input the new RFID card into the reader
void InputCard() {
  if (!CardReader.PICC_IsNewCardPresent()) return;
  if (!CardReader.PICC_ReadCardSerial()) return;

  uidLength = CardReader.uid.size;
  for (byte i = 0; i < uidLength; i++) {
    rfidUID[i] = CardReader.uid.uidByte[i];
  }

  Serial.print("UID saved: ");
  for (byte i = 0; i < uidLength; i++) {
    Serial.print(rfidUID[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  lcd.clear();
  lcd.print("Thanh cong!"); //(LCD: Success!)
  SuccessTone();
  delay(1500);
  CardReader.PICC_HaltA();  
  //Begin the entering new password phase
  lcd.clear();
  lcd.print("Mat khau moi:"); //(LCD: New password:)
  status = 1;
}

//Check whether card is correct or not
void CheckCard(){
  if (!CardReader.PICC_IsNewCardPresent()) return;
  if (!CardReader.PICC_ReadCardSerial()) return;

  if (CardReader.uid.size == uidLength) {
    status = 3;
    for (byte i = 0; i < uidLength; i++) {
      if (CardReader.uid.uidByte[i] != rfidUID[i]) {
        lcd.clear();
        lcd.print("Sai the! Thu lai!"); //(LCD: Wrong card! Retry!)
        Attempt++;
        WrongTone();
        delay(900);
        break;
      }
      else {
      lcd.clear();
      lcd.print("Da mo khoa!"); //(LCD: Unlocked!)
      //Reset
      Attempt = 0;
      MaxAttempt = 5;
      Countdown = 60;
      DoorLocker.write(Unlock);
      SuccessTone();
      delay(4500); //Unlock time 
      DoorLocker.write(Lock);
      lcd.clear();
      lcd.print("Da khoa!"); //(LCD: Locked!)
      LockedTone();
      delay(1300); //Enough time to show on screen
      break;
      }
    }
  CardReader.PICC_HaltA();
  status = 2;
  }
}

// Enter a new password
void KeypadInput() {
  if (customKey){
    if (customKey == '#'){
      lcd.clear();
      lcd.print("Thanh cong!"); //(LCD: Success!)
      status = 2;
      data_count = 0;
      SuccessTone();
      delay(1500);
    }
    else if (customKey == '*'){
      if (data_count != 0){
        data_count--;
      }
      lcd.setCursor(data_count, 1);
      lcd.print(" ");
      Master[data_count] = 0;
      KeypadTone();
    }
    else{
      if (data_count < Password_Length - 1) {
        Master[data_count] = customKey;
        data_count++;
        Serial.println(customKey);
        lcd.setCursor(data_count - 1, 1);
        lcd.print(customKey);
        KeypadTone();
      }
    }
  }
}

//Check if password is correct (pretty long:)))))))
void checkKeypad() {
  if (customKey) {
    if (data_count < Password_Length - 1) {
      if (status == 2){
        status = 4; //Make sure that waiting screen is cleared
      }

      if (status == 3){
         if (customKey == '#') {
          lcd.clear();
          if (!strcmp(Data, Master)) {
            Serial.println("Password is correct!");
            lcd.print("Da mo khoa!"); //(LCD: Unlocked!)
            //Reset
            Attempt = 0;
            MaxAttempt = 5;
            Countdown = 60;
            DoorLocker.write(Unlock);
            SuccessTone();
            delay(4500); //Unlock time
            DoorLocker.write(Lock);
            lcd.clear();
            lcd.print("Da khoa!"); //(LCD: Locked!)
            LockedTone();
            delay(1300);
          } 
          else {
            Serial.println("Password is incorrect!");
            lcd.print("Sai! Thu lai!"); //(LCD: Wrong! Retry!)
            Attempt++;
            WrongTone();
            delay(900);
          }     
          lcd.clear();
          clearData();
          status = 2;
        }
        else if (customKey == '*'){
          if (data_count != 0){
          data_count--;
          }
          lcd.setCursor(data_count, 1);
          lcd.print(" ");
          Data[data_count] = 0;
          KeypadTone();
        }
        else{
          Data[data_count] = customKey;
          data_count++;
          Serial.println(customKey);
          lcd.setCursor(data_count - 1, 1);
          lcd.print(customKey);
          KeypadTone();
        }
      }
    }
  }
}

// Waiting screen (show Temperature and Humidity)
void displayTempAndHumidity() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Khong doc duoc gia tri tu DHT11"); //(Serial: Unable to read DHT11)
    return;
  }

  Serial.print("Do am: "); //(Serial: Humidity:)
  Serial.print(h);
  Serial.print("%\t");
  Serial.print("Nhiet do: "); //(Serial: Temperature:)
  Serial.print(t);
  Serial.println(" *C");

  lcd.setCursor(0, 0);
  lcd.print("Nhiet do: "); //(LCD: Temperature:)
  lcd.print(t);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Do am: "); //(LCD: Humidity:)
  lcd.print(h);
  lcd.print("%");

  //Make sure that waiting screen is cleared ;-;
  if (status == 4){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Mat khau:"); //(LCD: Password:)
    status = 3;
    }
  delay(5); //Hope ;-;
}

//If there are too many wrong attempts, lock the system in a period of time
void Lockdown(){
  if (Attempt == MaxAttempt){
    PrevCount = Countdown;
    status = 5;
    Attempt = 0;
    //Decrease the maximum attempt if we input wrong for the next time
    if (MaxAttempt != 0){
    MaxAttempt--; 
    }
    lcd.clear();
    LockdownTone();
    while (Countdown != 0){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Sai qua nhieu!"); //(LCD: Too many attempts!)
      lcd.setCursor(0,1);
      lcd.print("Cho: "); //(LCD: Wait: )
      lcd.print(Countdown);
      lcd.print(" giay"); //(LCD: second)
      Countdown--;
      delay(1000);
    }
    status = 2;
    Countdown = PrevCount * 2; //Multiply timer if we input wrong for the next time
  }
}

//Wrong password or card, 2 short beeps
void WrongTone(){
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(BuzzerPin, LOW);
}

//Input successful or opened, 1 long beep
void SuccessTone(){
  digitalWrite(BuzzerPin, HIGH);
  delay(500);
  digitalWrite(BuzzerPin, LOW);
}

//Locked, 1 short beep
void LockedTone(){
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(BuzzerPin, LOW);
}

//Keypad sound, 1 very short beep
void KeypadTone(){
  digitalWrite(BuzzerPin, HIGH);
  delay(50);
  digitalWrite(BuzzerPin, LOW);
}

//Lockdown, 1 very long beep
void LockdownTone(){
  digitalWrite(BuzzerPin, HIGH);
  delay(2000);
  digitalWrite(BuzzerPin, LOW);
}

