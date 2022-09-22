#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

#ifndef EEPROM_SIZE
#define EEPROM_SIZE 512
#endif
#define TIMEOUT 10000 //таймаут в мсек, по истечении которого подключение считается неудачным
#define KA_TIMEOUT 5000 //как часто проверять состояние сети
/*
wifi(start_eeprom_addr); //аргумент - начальный адрес структуры в EEPROM
wifi.connect() - подключается к wi-fi из  EEPROM или к сети по "умолчанию". Если ни к оной сети не поключился, просит ввести данные через Serial, затем пробует, в случае успеха - сохраняет
wifi.saveNew() - пробует подключиться к новой сети, в случае успеха, сохраняет ее в EEPROM. возвращает 1 если успешно
wifi.keepAlive() - проверяет статус сети каждые KA_TIMEOUT мсек, при необходимости переподключается
*/

struct auth {
  char ssid[16] = "";
  char psw[16] = "";
};

class myWifi {
public:
  myWifi(const int16_t &startAddr);             
  void connect();                               
  bool saveNew(auth &data);                     
  void keepAlive();
  auth def_auth = { "Marianna", "marianna" };  //default network

private:
  bool tryThis(auth &data);
  auth _auth;
  uint16_t _startAddr;
};
extern myWifi connection(2);

myWifi::myWifi(const int16_t &startAddr) {
  WiFi.setAutoReconnect(true);  //auto reconnect on disconnect
  _startAddr = startAddr;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(_startAddr, _auth);
  EEPROM.end();

}



void myWifi::connect() {
  if (WiFi.isConnected()) { return; }
  if (tryThis(_auth)) {
    return;
  } else {
    if (tryThis(def_auth)) {
      Serial.println(F("Connected to the default wi-fi"));
      return;
    }
  }
 uint32_t time = millis();
 try_again:
  Serial.println(F("Unable to connect to any wi-fi, i'll try to reconnect in 60sec. You can enter SSID manually:"));
  while (!Serial.available()) {
    yield();
    if(millis()-time > 60000) return;
  }
  String ssid = Serial.readString();
  ssid.trim();
  ssid.toCharArray(_auth.ssid, 16);
  Serial.println(F("Enter password within 60 sec:"));
  time = millis();
  while (!Serial.available()) {
    yield();
    if(millis()-time>60000) return;
  }
  String psw = Serial.readString();
  psw.trim();
  psw.toCharArray(_auth.psw, 16);
  if (tryThis(_auth)) {
    Serial.println(F("Connected"));
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(_startAddr, _auth);
    EEPROM.end();
    return;
  } else {
    goto try_again;
  }
  
}

bool myWifi::saveNew(auth &data) {
  if (WiFi.isConnected()) { WiFi.disconnect(); }
  if (tryThis(data)) {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(_startAddr, data);
    EEPROM.end();
    return 1;
  } else {  
    tryThis(_auth);
  }
  return 0;
}

void myWifi::keepAlive()
{
  static uint32_t time;
  if(millis()-time < KA_TIMEOUT) {return;}
  time = millis();
  if(!WiFi.isConnected()) connect();
}

bool myWifi::tryThis(auth &data) {
  WiFi.begin(data.ssid, data.psw);
  uint32_t start = millis();
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    if (millis() - start > TIMEOUT) { return 0; }
  }
  return 1;
}

