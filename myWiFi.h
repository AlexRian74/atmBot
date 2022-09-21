#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>

/*
wifi(start_eeprom_addr);
wifi.connect() - подключается к wi-fi из  EEPROM или к сети по "умолчанию". Если ни к оной сети не поключился, просит ввести данные через Serial, затем пробует, в случае успеха - сохраняет
wifi.saveNew() - пробует подключиться к новой сети, в случае успеха, сохраняет ее в EEPROM. возвращает 1 если успешно

*/

struct auth {
  char ssid[16] = "";
  char psw[16] = "";
};

class myWifi {
public:
  myWifi(const int16_t &startAddr);             //аргумент - начальный адрес структуры в EEPROM
  bool connect();                               //
  bool saveNew(auth &data);                     //
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
  EEPROM.begin(_startAddr + 32);
  EEPROM.get(_startAddr, _auth);
  EEPROM.end();

}



bool myWifi::connect() {
  if (WiFi.isConnected()) { return 1; }
  if (tryThis(_auth)) {
    return 1;
  } else {
    if (tryThis(def_auth)) {
      Serial.println(F("Выполнено подключение к сети по умолчанию"));
      return 1;
    }
  }

 try_again:
  Serial.println(F("Не удалось подключиться к Wi-Fi\n Введите SSID:"));
  while (!Serial.available()) {
    yield();
  }
  String ssid = Serial.readString();
  ssid.trim();
  ssid.toCharArray(_auth.ssid, 16);
  Serial.println(F("Введите пароль:"));
  while (!Serial.available()) {
    yield();
  }
  String psw = Serial.readString();
  psw.trim();
  psw.toCharArray(_auth.psw, 16);
  if (tryThis(_auth)) {
    Serial.println(F("Успешно"));
    EEPROM.begin(_startAddr + 32);
    EEPROM.put(_startAddr, _auth);
    EEPROM.end();
    return 1;
  } else {
    goto try_again;
  }
  
}

bool myWifi::saveNew(auth &data) {
  if (WiFi.isConnected()) { WiFi.disconnect(); }
  if (tryThis(data)) {
    EEPROM.begin(_startAddr+32);
    EEPROM.put(_startAddr, data);
    EEPROM.end();
    return 1;
  } else {  
    tryThis(_auth);
  }
  return 0;
}

bool myWifi::tryThis(auth &data) {
  WiFi.begin(data.ssid, data.psw);
  uint32_t start = millis();
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
    if (millis() - start > 10000) { return 0; }
  }
  return 1;
}

