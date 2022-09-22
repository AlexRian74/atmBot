#define CREATOR_ID 1028255209
#define EEPROM_SIZE 512
#include <EasyNTPClient.h>
#include <WiFiUdp.h>
#include <FastBot.h>


FastBot bot("5732234013:AAHXECLY-toweps91RR3KBM7YFWHZ40YA0Q");  // с указанием токена
uint16_t msgTTLsec = 0;  //время жизни сообщения, сек. Сообщение, время отправки которых превышает текущее время на указаннуювеличину, игнорируются

#include "Users.h"
#include "Menu.h"
#include "myWiFi.h"

Menu* aMenu = new Menu(adminMenu, TOTAL_ADMIN_ITEMS);
Menu* uMenu = new Menu(userMenu, TOTAL_USER_ITEMS);

void setup() {
  WiFiUDP ntpUDP;                   //udp объект для easyNTPClient
  EasyNTPClient ntpClient(ntpUDP);  //Объявляем ntpClient класса EasyNTPClient
  Serial.begin(115200);
  connection.connect(); //подключаемся к сети

  Serial.println("Obtaining time from ntp");
  uint32_t time = ntpClient.getUnixTime();
  if (time) {
    bot.setUnix(time);
    FB_Time getTime(time, 5);
    Serial.println(getTime.dateString());
    Serial.println(getTime.timeString());
  } else {
    Serial.println("Failed");
  }
  bot.attach(newMsg);  // подключаем обработчик сообщений


}





void loop() {
  bot.tick();
  delay(1000);
  connection.keepAlive();
}

void newMsg(FB_msg& msg) {
  String path;
  bool auth = 0, creator = 0;
  uint32_t tID = msg.userID.toInt();
  Serial.println("message: ");
  Serial.println(msg.text);

  //Проверяем, что пользователь в списке пользователей или создатель
  for (byte i = 0; i < user.totalUsers; i++) {
    userEntry tmp;
    user.getByNum(tmp, i);
    if (tID == tmp.telegramID) {
      auth = 1;
      user.selectUser(tID);  //если найден, выбираем
    }
  }
  if (tID == CREATOR_ID) {
    creator = 1;
    botMenu = aMenu;  //меню администратора
    path = String(botMenu->navStr);
  }
  if (!auth && !creator) {
    return;
  }
  //Выбираем чат для отправки сообщений
  bot.setChatID(msg.chatID);

  //Проверяем, что сообщение не просрочилось
  uint32_t time = bot.getUnix();
  uint16_t shift = time - msg.unix;
  if (time - msg.unix >= msgTTLsec && msgTTLsec != 0) {
    bot.sendMessage(F("Сообщение устарело. Отправьте запрос повторно"));
    return;
  }

  //Выбираем меню в зависимости от категории пользователя
  if (user.userSelected && !creator)  // если сообщение от пользователя, но не создателя
  {
    if (user.parameters.admin) {
      botMenu = aMenu;
    } else {
      botMenu = uMenu;
    }
  }

  //Проверяем, для кого сообщение (для меню или для функции меню)
  if (callMeBack.callMe) {
    callMeBack.botMessage = &msg;  //адрес на структуру сообщения
    callMeBack.call();
    return;
  }

  String menuText = botMenu->checkCommand(msg.text);
  if (menuText == "") { return; }  //Если в меню вызвана команда, то она будет отрисовывать меню, мы выходим

  if (botMenu == aMenu) {
    path = String(botMenu->navStr);
    if (botMenu->menuItems[botMenu->parentID].parentID == USERS_ID)  //Если администратор зашел в элемент меню, у которого родитель == USERS_ID
    {
      uint32_t tIDu = user.getIdByName(botMenu->menuItems[botMenu->parentID].itemName);
      user.selectUser(tIDu);
      path += F("\nБаланс: ") + String(user.parameters.balance) + F("\nЛимит: ") + String(user.parameters.limit);
    }

  } else {
    uint16_t avail = user.parameters.balance > user.parameters.limit ? user.parameters.balance - user.parameters.limit : 0;
    path = F("Ваш баланс: ") + String(user.parameters.balance) + F("Р") + F("\nДоступно: ") + String(avail) + F("Р");
    menuText += "\tОбновить";
  }
  bot.showMenuText(path, menuText);
}