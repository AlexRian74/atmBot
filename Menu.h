#pragma once
#include <Arduino.h>
#include "myWiFi.h"

//MENU ID'S
#define ROOT_MENU 0

//ADMIN MENU ID's
#define USERS_ID 1
#define USERS_ADD_ID 2
#define SERVICE_ID 3
#define SERVICE_ADMINISTRATORS_ID 4
#define ADMINISTRATORS_ADD_ID 5
#define SERVICE_LOG_ID 6
#define SERVICE_WIFI_ID 7
#define SERVICE_MSGTTL_ID 8
#define SERVICE_HARDRESET_ID 9
#define SERVICE_SOFTRESET_ID 10
#define DISPENSERS_ID 11
#define DISPENSER1_ID 12
#define DISPENSER1_SETBILLCOUNT_ID 13
#define DISPENSER1_DENOMINATION_ID 14
#define DISPENSER1_SHOWSTATUS_ID 15
#define DISPENSER1_DISABLE_OR_ENABLE_ID 16
#define DISPENSER2_ID 17
#define DISPENSER2_SETBILLCOUNT_ID 18
#define DISPENSER2_DENOMINATION_ID 19
#define DISPENSER2_SHOWSTATUS_ID 20
#define DISPENSER2_DISABLE_OR_ENABLE_ID 21
#define VALIDATOR_ID 22
#define VALIDATOR_MAXBILL_ID 23
#define VALIDATOR_STATUS_ID 24
#define VALIDATOR_DISABLE_ID 25
#define TOTAL_ADMIN_ITEMS 26
//USER MENU ID's
#define WITHDRAW_ID 1
#define BALANCE_ID 2
#define HISTORY_ID 3
#define TOTAL_USER_ITEMS 3

User user(2+32);
extern uint16_t msgTTLsec;

//Заголовки фукнций для работы с меню, сами функции ниже
void addUser();
void getLog();
void setWifi();
void msgTTL();
void hardReset();
void softReset();
void setBillCount();
void denomination();
void showDispenserStatus();
void disableOrEnableDispenser();
void maxBill();
void validatorStatus();
void disableOrEnableValidator();
void changeBalance();
void setLimit();
void blockUser();
void removeUser();
void withdrawMoney();
void userBalance();
void userHistory();

struct menuItem {
  char itemName[16] = "";  //Имя элемента меню
  byte parentID;
  void (*cmdFunc)();
};

struct {
  bool callMe = 0;
  byte count = 0;
  void (*call)();      //адрес функции
  FB_msg* botMessage;  //полученное сообщение
} callMeBack;

menuItem adminMenu[] = {  //default administrator's menu
  { "Root", 255, 0 },
  { "Users", ROOT_MENU },
  { "Add", USERS_ID, addUser },

  { "Service", ROOT_MENU },
  { "Administrators", SERVICE_ID },
  { "Add", SERVICE_ADMINISTRATORS_ID, addUser },
  { "Log", SERVICE_ID, getLog },
  { "Wi-Fi", SERVICE_ID, setWifi },
  { "Msg TTL", SERVICE_ID, msgTTL },
  { "Hard reset", SERVICE_ID, hardReset },
  { "Soft reset", SERVICE_ID, softReset },

  { "Dispensers", ROOT_MENU },
  { "Dispenser1", DISPENSERS_ID },
  { "SetBillCount", DISPENSER1_ID, setBillCount },
  { "Denomination", DISPENSER1_ID, denomination },
  { "ShowStatus", DISPENSER1_ID, showDispenserStatus },
  { "Disable", DISPENSER1_ID, disableOrEnableDispenser },
  { "Dispenser2", DISPENSERS_ID },
  { "SetBillCount", DISPENSER2_ID, setBillCount },
  { "Denomination", DISPENSER2_ID, denomination },
  { "ShowStatus", DISPENSER2_ID, showDispenserStatus },
  { "Disable", DISPENSER2_ID, disableOrEnableDispenser },

  { "Validator", ROOT_MENU },
  { "MaxBill", VALIDATOR_ID, maxBill },
  { "Status", VALIDATOR_ID, validatorStatus },
  { "Disable", VALIDATOR_ID, disableOrEnableValidator }


};

menuItem userMenu[] = { { "Root", 255, 0 }, { "Снять", ROOT_MENU, withdrawMoney }, { "История", ROOT_MENU, userHistory } };  //users Menu

class Menu {

public:
  Menu(menuItem* pointer, const byte& size);  //size - количество элементов в массиве
  ~Menu();
  void initMenu();  //creates menu in memory
  String checkCommand(const String& command);
  bool addMenuItems(menuItem* arrayOfItems, const byte& count);  //add several items to the menu
  void updateUsers();

  byte grandParentID;
  byte parentID;
  byte lastCmdID;
  byte menuSize;              //current size of a menu
  byte iMenuSize;             //initial size of a menu
  char navStr[30] = "Root>";  //navigation string parent/children>
  menuItem* menuItems;        //current items of a menu
  menuItem* iMenuPointer;     //initial pointer of a menu
};


Menu::Menu(menuItem* pointer, const byte& size) {
  iMenuPointer = pointer;
  iMenuSize = size;
  grandParentID = 255;
  parentID = 0;
  lastCmdID = 0;
  initMenu();
}
void Menu::initMenu() {
  menuItems = (menuItem*)malloc(iMenuSize * sizeof(menuItem));                  //выделяем размер в памяти под элементы меню
  memcpy((void*)menuItems, (void*)iMenuPointer, iMenuSize * sizeof(menuItem));  //копируем в выделенный размер элементы меню
  menuSize = iMenuSize;                                                         //задаем число элементов в меню равным первоначальному значению
  user.initUser();
  if (iMenuSize >= TOTAL_ADMIN_ITEMS) {  //Если это меню администратора, загружаем в меню пользователей из EEPROM, Обновляем msgTTL
    char itemName[16] = "msgTTL [ \0";   //обновляем Значение msgTTL
    EEPROM.begin(4);
    EEPROM.get(0, msgTTLsec);
    EEPROM.end();
    itoa(msgTTLsec, itemName + 8, 10);
    strcat(itemName, "]");
    strcpy((menuItems + 8)->itemName, itemName);
    updateUsers();
  }
}
Menu::~Menu()  //деструктор
{
  free(menuItems);  //удаляем меню
}

String Menu::checkCommand(const String& command) {
  byte currentID = 0;
  String response;
  bool cmdFound = 0;
  char cmd[16];  // макс. длина menuItem.itemName  + 1
  command.toCharArray(cmd, command.length() + 1 > 16 ? 16 : command.length() + 1);
  for (byte i = 0; i < menuSize; i++)  //Ищем ID переданной команды
  {
    //UP command - выход из удаляемого элемента меню
    if (!strncmp("UP", cmd, 2)) {
      currentID = grandParentID;
      if (currentID == 255) {
        currentID = 0;
      }
      parentID = menuItems[currentID].parentID;
      if (parentID == 255) {
        parentID = 0;
      }
      cmdFound = 1;
      break;
    }
    //Если имя команды совпало и ID родителя команды совпал, запоминаем ID.
    if (!strncmp(menuItems[i].itemName, cmd, strlen(menuItems[i].itemName)) && menuItems[i].parentID == parentID) {
      currentID = i;
      cmdFound = 1;
      break;
    }
  }

  if (!cmdFound)  //Если команда не найдена
  {
    if (lastCmdID) {  //Возврат на уровень вверх из команды
      currentID = parentID;
      parentID = menuItems[currentID].parentID;
      if (parentID == 255) {
        parentID = 0;
      }
    } else {  //Возврат на уровень вверх из папки
      currentID = menuItems[parentID].parentID;
      if (currentID == 255) {
        currentID = 0;
      }
      parentID = menuItems[currentID].parentID;
      if (parentID == 255) {
        parentID = 0;
      }
    }
  }



  //Выводим детей Текущей команды
  byte count = 1;
  for (byte i = 0; i < menuSize; i++) {
    if (menuItems[i].parentID == currentID) {
      if (count && count % 3 == 0) {
        response = response + menuItems[i].itemName + "\n";
      } else {
        response = response + menuItems[i].itemName + "\t";
      }
      count++;
    }
  }

  navStr[0] = 'R';
  navStr[1] = 'o';
  navStr[2] = 'o';
  navStr[3] = 't';
  navStr[4] = '>';
  navStr[5] = '\0';
  byte pLength = 0;
  if (parentID > 0) {
    pLength = strlen(menuItems[parentID].itemName);  //длина родителя
    strncpy(navStr, menuItems[parentID].itemName, pLength);
    navStr[pLength++] = '\\';
    navStr[pLength] = '\0';
  }
  byte cLength = 0;
  if (currentID > 0) {
    cLength = strlen(menuItems[currentID].itemName);  //длина currentID
    strncpy(navStr + pLength, menuItems[currentID].itemName, cLength);
    navStr[pLength + cLength] = '>';
    navStr[pLength + cLength + 1] = '\0';
  }

  lastCmdID = 0;  // Переменная содержит номер команды сразу после поступления команды,в остальных случаях 0

  grandParentID = menuItems[parentID].parentID;
  if (menuItems[currentID].cmdFunc == NULL)  //текущий ID меню - папка
  {
    parentID = currentID;  //Запоминаем ее как родителя для следующей команды
  } else {                 //Если текущий ID не папка, значит команда. у нее нет детей, но есть родитель, который будет "grandParentID"
    lastCmdID = currentID;
    menuItems[currentID].cmdFunc();  //вызов функции меню
    navStr[0] = '\0';
    return "";
  }

  return currentID > 0 ? response + "\n Back" : response;
}

bool Menu::addMenuItems(menuItem* arrayOfItems, const byte& count) {
  byte oldMenuSize = menuSize;
  menuSize += count;  //увеличиваем количество элементов в меню
  //Изменяем размер памяти для хранения меню
  menuItems = (menuItem*)realloc(menuItems, menuSize * sizeof(menuItem));
  //дописываем новые строки в меню
  memcpy((void*)(menuItems + oldMenuSize), (void*)arrayOfItems, sizeof(menuItem) * count);  //копируем новые элементы в конец массива

  return 1;
}

void Menu::updateUsers() {
  if (user.totalUsers == 0) {
    return;
  }
  userEntry tmpUser;
  menuItem tmpItems[] = { { "", USERS_ID, 0 }, { "ChangeBalance", 0, changeBalance }, { "SetLimit", 0, setLimit }, { "", 0, blockUser }, { "Remove", 0, removeUser } };
  menuItem tmpAdmItems[] = { { "", SERVICE_ADMINISTRATORS_ID, 0 }, { "Remove", 0, removeUser } };
  menuItem* pointer;
  byte items;  //количество добавляемых в меню элементов
  for (byte i = 0; i < user.totalUsers; i++) {
    user.getByNum(tmpUser, i);
    //Если пользователь -  администратор
    if (tmpUser.admin) {
      pointer = tmpAdmItems;
      strncpy(pointer->itemName, tmpUser.userName, strlen(tmpUser.userName) + 1);  //копируем имя пользователя
      (pointer + 1)->parentID = menuSize;
      items = 2;  //сколько элементов в меню администратора
    } else {      //Для обычного пользователя
      pointer = tmpItems;
      strncpy(pointer->itemName, tmpUser.userName, strlen(tmpUser.userName) + 1);  //копируем имя пользователя
      //Обновляем родителя для детей пользователя, дети начинаются со второго элемента в массиве (j=1)
      for (byte j = 1; j < sizeof(tmpItems) / sizeof(menuItem); j++) {
        (pointer + j)->parentID = menuSize;
      }
      //в зависимости от статуса пользователя называем кнопку в меню
      if (tmpUser.enabled == 1) {
        strncpy(tmpItems[3].itemName, "Block", 6);
      } else {
        strncpy(tmpItems[3].itemName, "unBlock", 8);
      }
      items = 5;  //сколько элементов в элементе меню пользователя
    }
    addMenuItems(pointer, items);
  }
}

//======================================================================================MENU FUNCTIONS=====================================
Menu* botMenu;  // = new Menu(adminMenu, 25);

void addUser() {
  static char tmpName[16];
  static uint32_t tID;
  bool admin;
  if (user.totalUsers >= 10) {
    String menuText = botMenu->checkCommand(F(" "));
    String path = F("Максимальное количество всех пользователей  - 10\n") + String(botMenu->navStr);
    bot.showMenuText(path, menuText);
    return;
  }
  if (botMenu->menuItems[botMenu->lastCmdID].parentID == SERVICE_ADMINISTRATORS_ID) {
    admin = 1;
  } else {
    admin = 0;
  }
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = addUser;
  switch (callMeBack.count) {
    case 0:
      bot.closeMenuText(F("Введите имя"));
      callMeBack.count++;
      break;
    case 1:
      {
        byte nameLength = (callMeBack.botMessage->text).length();
        if (nameLength > 15 || nameLength < 1) {
          bot.sendMessage(F("от 1 до 16 лат. символов:"));
          break;
        }
        String msgText = callMeBack.botMessage->text;
        msgText.toCharArray(tmpName, 16);
        bot.sendMessage(F("Введите telegramID"));
        callMeBack.count++;
      }
      break;
    case 2:
      {
        String menuText;
        String path(botMenu->navStr);
        tID = (callMeBack.botMessage->text).toInt();
        if (user.add(tmpName, tID, admin)) {
          path += F("\nУспешно");
        } else {
          path += F("\nОшибка");
        }

        callMeBack.callMe = 0;
        callMeBack.count = 0;
        callMeBack.call = 0;
        free(botMenu->menuItems);                  //удаляем старое меню
        botMenu->initMenu();                       //перезапускаем меню
        menuText = botMenu->checkCommand(F(" "));  //обновляем строку меню
        bot.showMenuText(path, menuText);
      }
      break;
    default:
      bot.sendMessage(F("Исключение в addUser()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}


void getLog() {
}

void setWifi() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = setWifi;

  switch (callMeBack.count) {
    static auth _auth;
    case 0:
      {
        String path = "Текущее подключение: " + WiFi.SSID();
        path += "\nВведите новый SSID или 0 для отмены";
        bot.closeMenuText(path);
        callMeBack.count++;
      }
      break;
    case 1:
      {
        String msg = callMeBack.botMessage->text;
        msg.trim();
        if ((msg) == "0") {
          String menuText = botMenu->checkCommand(F(" "));
          String path(botMenu->navStr);
          callMeBack.callMe = 0;
          callMeBack.count = 0;
          bot.showMenuText(path, menuText);
          return;
        }
        msg.toCharArray(_auth.ssid, 16);
        bot.sendMessage(F("Введите пароль или 0 для отмены"));
        callMeBack.count++;
      }
      break;
    case 2:
      {
        String menuText = botMenu->checkCommand(F(" "));
        String path(botMenu->navStr);
        String msg = callMeBack.botMessage->text;
        msg.trim();
        if (msg == "0") {
          callMeBack.callMe = 0;
          callMeBack.count = 0;
          bot.showMenuText(path, menuText);
          return;
        }
        msg.toCharArray(_auth.psw, 16);
        bot.sendMessage(F("Пробую подключиться к ") + String(_auth.ssid));
        if (connection.saveNew(_auth)) {
          path += F("\nДанные подключения обновлены");
        } else {
          path += F("\nНе удалось подключиться к сети");
        }
        bot.showMenuText(path, menuText);
        callMeBack.callMe = 0;
        callMeBack.count = 0;
      }
      break;
    default:
      bot.sendMessage(F("Исключение в setWifi()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}

void msgTTL() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = msgTTL;
  String path = String(botMenu->navStr);
  switch (callMeBack.count) {
    case 0:
      {
        path += F("\nВведите время жизни сообещния (от 10 до 65535) в секундах или 0 чтобы принимать все сообщения.");
        bot.closeMenuText(path);
        callMeBack.count++;
      }
      break;
    case 1:
      {
        msgTTLsec = (callMeBack.botMessage->text).toInt();
        if (msgTTLsec < 10 && msgTTLsec != 0) msgTTLsec = 10;
        //пишем
        EEPROM.begin(EEPROM_SIZE);
        EEPROM.put(0, msgTTLsec);
        EEPROM.end();
        //перерисовываем меню
        free(botMenu->menuItems);  //удаляем старое меню
        botMenu->initMenu();       //перезапускаем меню
        String menuText = botMenu->checkCommand(F(" "));
        String path = String(botMenu->navStr);
        bot.showMenuText(path, menuText);  //Рисуем новое меню
        callMeBack.callMe = 0;
        callMeBack.count = 0;
      }
      break;
    default:
      bot.sendMessage(F("Исключение в msgTTL()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}

void hardReset() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = hardReset;
  String path = String(botMenu->navStr);
  switch (callMeBack.count) {
    case 0:
      path += F("\nВсе данные будут стерты, продолжить?");
      bot.showMenuText(path, F("Да\tОтмена"));
      callMeBack.count++;
      break;
    case 1:
      {
        String menuText = botMenu->checkCommand(F(" "));
        String path = String(botMenu->navStr);
        EEPROM.begin(EEPROM_SIZE);
        if (callMeBack.botMessage->text == String("Да")) {
          for (uint16_t i = 0; i < EEPROM_SIZE; i++) {
            EEPROM.write(i, 0);
          }
          EEPROM.end();
          path += F("\nEEPROM очищен");
          free(botMenu->menuItems);  //удаляем старое меню
          botMenu->initMenu();       //запускаем новое меню
        }
        callMeBack.callMe = 0;
        callMeBack.count = 0;
        bot.showMenuText(path, menuText);
      }
      break;
    default:
      bot.sendMessage(F("Исключение в hardReset()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}
void softReset() {
  bot.sendMessage(F("Перезагрузка"));
  ESP.restart();
}

void setBillCount() {
  // if ((menuha->lastCmdID) < DISPENSER2_ID)
  //  {return "Dispenser1-set bill count";}

  // if((menuha->lastCmdID) > DISPENSER2_ID)
  // {return "Dispenser2-set bill count";}
}

void denomination() {
  // if ((menuha->lastCmdID) < DISPENSER2_ID)
  //{return "Dispenser1-denomination";}

  //  if((menuha->lastCmdID) > DISPENSER2_ID)
  //  {return "Dispenser2-denomination";}
}

void showDispenserStatus() {
  // if ((botMenu->lastCmdID) < DISPENSER2_ID)
  // {return "Dispenser1- show stat";}

  // if((botMenu->lastCmdID) > DISPENSER2_ID)
  //  {return "Dispenser2- show stat";}
}

void disableOrEnableDispenser() {
  //  if ((botMenu->lastCmdID) < DISPENSER2_ID)
  // {return "Dispenser1-disable or enable";}

  //  if((botMenu->lastCmdID) > DISPENSER2_ID)
  //  {return "Dispenser2-disabe or enable";}
}

void maxBill() {
}

void validatorStatus() {
}

void disableOrEnableValidator() {
}


void changeBalance() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = changeBalance;
  String path = String(botMenu->navStr);
  uint32_t tID = user.getIdByName(botMenu->menuItems[botMenu->parentID].itemName);
  user.selectUser(tID);
  switch (callMeBack.count) {
    case 0:
      path += F("\nВведите число от -32500 до 32500):");
      bot.closeMenuText(path);
      callMeBack.count++;
      break;
    case 1:
      {
        String menuText = botMenu->checkCommand(F(" "));
        path = String(botMenu->navStr);
        int32_t increment = (callMeBack.botMessage->text).toInt();
        path = String(botMenu->navStr);
        if (increment < -32500 || increment > 32500 || (increment + user.parameters.balance) < 0 || (increment + user.parameters.balance) > 65500) {
          path += F("\nОшибка.");
          path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
          bot.showMenuText(path, menuText);
          callMeBack.callMe = 0;
          callMeBack.count = 0;
          break;
        }
        if (!user.changeBalance(increment)) { path += F("\nОшибка"); };
        path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
        bot.showMenuText(path, menuText);
        callMeBack.callMe = 0;
        callMeBack.count = 0;
      }
      break;
    default:
      bot.sendMessage(F("Исключение в changeBalance()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}

void setLimit() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = setLimit;
  uint32_t tID = user.getIdByName(botMenu->menuItems[botMenu->parentID].itemName);
  user.selectUser(tID);
  switch (callMeBack.count) {
    case 0:
      {
        String path(botMenu->navStr);
        path += F("\nТекущий лимит ") + String(user.parameters.limit) + F("Р, введите новый:");
        bot.closeMenuText(path);
        callMeBack.count++;
      }
      break;
    case 1:
      {

        int16_t limit = (callMeBack.botMessage->text).toInt();
        String menuText = botMenu->checkCommand(F(" "));
        user.setLimit(limit);
        String path(botMenu->navStr);
        path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
        bot.showMenuText(path, menuText);
        callMeBack.callMe = 0;
        callMeBack.count = 0;
      }
      break;
    default:
      bot.sendMessage(F("Исключение в setLimit()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}

void blockUser() {
  String menuText;
  uint32_t tID = user.getIdByName(botMenu->menuItems[botMenu->parentID].itemName);
  user.selectUser(tID);
  user.parameters.enabled = !user.parameters.enabled;
  user.saveEntry();
  free(botMenu->menuItems);                  //удаляем старое меню
  botMenu->initMenu();                       //запускаем новое меню
  menuText = botMenu->checkCommand(F(" "));  //выходим из "команды" на уровень вверх
  String path = String(botMenu->navStr);
  path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
  bot.showMenuText(path, menuText);
}

void removeUser() {
  if (!callMeBack.callMe) {
    callMeBack.callMe = true;
  }
  callMeBack.call = removeUser;
  uint32_t tID = user.getIdByName(botMenu->menuItems[botMenu->parentID].itemName);
  user.selectUser(tID);  // Выбираем пользователя
  switch (callMeBack.count) {
    case 0:
      {
        char buf[16] = "";
        String path(botMenu->navStr);
        strncpy(buf, botMenu->menuItems[botMenu->parentID].itemName, 16);
        path = "Удалить пользователя " + String(buf) + "?";
        bot.showMenuText(path, F("Да\tНет"));
        callMeBack.count++;
      }
      break;

    case 1:
      {
        String menuText;
        String path;
        if (callMeBack.botMessage->text == String("Да")) {
          //удалить
          user.del();
          user.initUser();
          free(botMenu->menuItems);  //удаляем старое меню
          botMenu->initMenu();       //Обновляем меню
          menuText = botMenu->checkCommand(F("UP"));
          path = String(botMenu->navStr);
        } else {
          menuText = botMenu->checkCommand(F(" "));
          path = String(botMenu->navStr);
          path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
        }
        callMeBack.callMe = 0;
        callMeBack.count = 0;
        callMeBack.call = 0;
        bot.showMenuText(path, menuText);
      }
      break;
    default:
      bot.sendMessage(F("Исключение в removeUser()"));
      callMeBack.callMe = 0;
      callMeBack.count = 0;
      break;
  }
}

void withdrawMoney() {
  bot.sendMessage(F("Снимаю деньги"));
}

void userBalance() {
}

void userHistory() {
  bot.sendMessage(F("История операций"));
}