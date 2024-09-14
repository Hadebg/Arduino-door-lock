#ifndef _MAIN_H_
#define _MAIN_H_

//RFID
#define SS_PIN 5  
#define RST_PIN 1  
byte rfidUID[10];  
int uidLength;  

//Keypad
#define ROWS 4
#define COLS 4
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {14, 27, 26, 25};
byte colPins[COLS] = {2, 4, 33, 32};

//Pin
#define DoorLock_Pin 17
#define BuzzerPin 15

//Locker (servo) pwm
#define Unlock 150
#define Lock 800

//DHT
#define DHTPIN 16 
#define DHTTYPE DHT11

//Password
#define Password_Length 16 //Because LCD can only show 16 digits in 1 line
char Data[Password_Length];
char Master[Password_Length]; 
byte data_count = 0;
char customKey;

//Status (explained in cpp file)
byte status = 0;

//Lockdown
byte Attempt = 0;
byte Countdown = 60;
byte PrevCount = 0;
byte MaxAttempt = 5;


void clearData();
void InputCard();
void CheckCard();
void KeypadInput();
void checkKeypad();
void displayTempAndHumidity();
void Lockdown();
void WrongTone();
void SuccessTone();
void LockedTone();
void KeypadTone();
void LockdownTone();

#endif