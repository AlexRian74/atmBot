#pragma once
#include <Arduino.h>
#include <EEPROM.h>


//Структура, описывающая пользователя
struct userEntry {
  byte     startSign = 0xBA;// Знак начала пользовательской структуры
  bool     admin     = 0; // 1-admin, 0-user
  bool     enabled   = 0;//1-yes, 0-no
  uint16_t balance   = 0; //баланс пользователя
  uint16_t limit     = 0; //неснижаемый остаток на счету
  uint32_t     telegramID = 0;// телеграм ID пользователя
  char     userName[16];// Имя пользователя не более 16 символов
  byte     endSign   = 0xAB;// Знак конца пользовательской структуры
};

//Класс с методами для работы с пользователями
class User {

  public:

    User(const uint16_t &startAddr); //Конструктор. При вызове устанавливает начальный адрес по которому должна храниться структуруа
    void    initUser(); //делает работу конструктора
    bool    selectUser(const uint32_t &telegramID); //Заполняет parameters информацией об указанном пользователе. возвращает 1- успешно, 0 - не успешно
    bool    add(char* userName, const uint32_t &telegramID, bool admin); //добавляет нового пользователя. 1 - успешно, 0 - ошибка
    void    setLimit(int16_t &limit);
    void    del(); // removes selected user, returns bool
    bool    changeBalance(const int16_t &amount); //увеличивает баланс на сумму, возвращает 1 если успешно
    bool    saveEntry(); //saves parameters to EEPROM
    bool    getByNum(userEntry &tmp, const uint16_t &i); //заполняет структуру tmp пользователем №i
    uint32_t    getIdByName(char* userName);

    userEntry   parameters;  //Структура, содержащая сведения о выбранном пользователе
    uint8_t     totalUsers = 0;
    bool        userSelected = 0;
  private:

    bool getUserAddr(const uint32_t &telegramID);//если пользователь найден, вернет 1 и запишет адрес его структуры в переменную userAddr
    void getUserCount(); //Ищет записи о пользователях начиная с адреса startAddr, возвращает число найденных записей
    void defragEeprom(); //устраняет пропуски в массиве пользователей после удаления пользователя
    uint16_t _startAddr; //адрес EEPROM по которому хранятся структуры с данными пользователей
    uint16_t _userAddr = 0; //адрес выбранного пользователя

};

User::User(const uint16_t &startAddr)
{
  _startAddr = startAddr;
  initUser();
}

void User::initUser() //делает работу конструктора
{
  getUserCount();
  _userAddr = 0;
  userSelected = 0;
}

bool User::selectUser(const uint32_t &telegramID)
{
  if (!getUserAddr(telegramID)) {
    return 0; //если адрес пользователя не найден, выходим
  }
  else {
    EEPROM.get(_userAddr, parameters);
    userSelected = 1;
    EEPROM.end();
    return 1;
  }

}

bool User::add(char* userName, const uint32_t &telegramID, bool admin)
{
  //проверяем, есть ли пользователь с указанным именем или таким же телеграм
  EEPROM.begin(EEPROM_SIZE);
  userEntry userBuf;
  for (uint16_t Pos = _startAddr; Pos <  _startAddr + totalUsers * sizeof(userEntry); Pos += sizeof(userEntry))
  {
    EEPROM.get(Pos, userBuf);
    if (telegramID == userBuf.telegramID || !strncmp(userBuf.userName, userName, sizeof(userBuf.userName)))
    {
      EEPROM.end();
      return 0; //Если совпала телега или имя, возвращаем 0
    }
  }
  //Добавляем:
  userEntry newUser = {0xBA, admin, 1, 0, 0, telegramID, "", 0xAB};
  strncpy(newUser.userName, userName,16);
  newUser.userName[15] = '\0';
  EEPROM.put(_startAddr+totalUsers * sizeof(userEntry), newUser);
  EEPROM.end();
  initUser();   //обновляем число пользователей
  selectUser(telegramID);//выбираем пользователя
  return 1; //возвращаем результат записи
}

void User::setLimit(int16_t &limit)
{
  //Проверяет, выбран ли пользователь
  if (!userSelected) {
    return;
  }
  EEPROM.begin(EEPROM_SIZE);
  parameters.limit = limit;
  EEPROM.put(_userAddr, parameters); //сохраняем в EEPROM
  EEPROM.end();
}

void User::del() // removes selected user, returns bool
{
  //Проверяет, выбран ли пользователь
  if (!userSelected) {
    return;
  }
  byte* b = (byte*)&parameters; //указатель типа Byte на адрес структуры parameters
  for (byte i = 0; i < sizeof(parameters); i++)
  {
    *(b + i) = 0; //стираем все данные по этому адресу
  }
  //стави флаг, что пользователь не выбран
  userSelected = 0;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(_userAddr, parameters); //записываем в EEPROM текущему пользователю нули(удаляем)
  EEPROM.end();
  defragEeprom(); //убираем пропуски в EEPROM после удаления пользователя
  initUser();//заново инициализируем класс
}

bool User::changeBalance(const int16_t &amount)
{
  if (!userSelected || !parameters.enabled) {
    return 0; //Если пользователь не выбран или заблокирован, выходим из функции
  }
  EEPROM.begin(EEPROM_SIZE);
  parameters.balance += amount; //меняем баланс
  EEPROM.put(_userAddr, parameters); //сохраняем в EEPROM
  EEPROM.end();
  return 1;
}

bool User::saveEntry()
{
  if (!userSelected) {
    return 0;
  }
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(_userAddr, parameters); //сохраняем в EEPROM
  EEPROM.end();
  return 1;
}

bool User::getByNum(userEntry &tmp, const uint16_t &i)
{
  if (i >= totalUsers) {
    return 0;
  }
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(_startAddr + i * sizeof(userEntry), tmp);
  EEPROM.end();
  return 1;

}
uint32_t User::getIdByName(char* userName)
{
  userEntry tmp;
  for (byte i = 0; i < totalUsers; i++)
  {
    getByNum(tmp, i);
    if (strncmp(tmp.userName, userName, sizeof(tmp.userName)) == 0)
    {
      return tmp.telegramID;
    }
  }
  return 0;
}

//====================================================Private functions=======================================================
bool User::getUserAddr(const uint32_t &telegramID)//если пользователь найден, вернет 1 и запишет адрес его структуры в переменную userAddr
{
  userEntry userBuf;
  EEPROM.begin(EEPROM_SIZE);
  for (uint16_t Pos = _startAddr; Pos <  _startAddr + totalUsers * sizeof(userEntry); Pos += sizeof(userEntry))
  {
    EEPROM.get(Pos, userBuf);
    if (telegramID == userBuf.telegramID)
    {
      _userAddr =  Pos;
      EEPROM.end();
      return 1;
    }
  }
  EEPROM.end();
  return 0; //если пользователь не найден
}

void User::getUserCount() //Сколько пользователей у нас есть
{
  userEntry userBuf;
  bool found = 0;
  totalUsers = 0; // если в eeprom Нет никаких записей, found=0;
  EEPROM.begin(EEPROM_SIZE);
  for (uint16_t addr = _startAddr; addr  < EEPROM_SIZE; addr += sizeof(userEntry))
  {
    EEPROM.get(addr, userBuf);
    if (userBuf.startSign == 0xBA && userBuf.endSign == 0xAB) //Если есть указатели начала и конца структуры
      {
      found = 1;
      }
    else
      {
      if (found) {
        totalUsers = (addr-_startAddr) / sizeof(userEntry);  //пишем число найденных записей
        EEPROM.end();
        return;
      }
    }
  }
  EEPROM.end();
}

void User::defragEeprom() //дефрагментирует EEPROM после удаления пользователя
{
  EEPROM.begin(EEPROM_SIZE);
  userEntry temp;
  uint8_t size = sizeof(userEntry);
  byte nullmas[sizeof(userEntry)];
  for (byte i = 0; i < size; i++) {
    nullmas[i] = 0;
  }
  for (uint16_t addr = _userAddr + size; addr < _startAddr + totalUsers * size; addr += size)
  {
    EEPROM.get(addr, temp); //считываем данные сразу за дыркой
    if (temp.startSign == 0xBA && temp.endSign == 0xAB) //если в них структура
    {
      EEPROM.put(addr - size, temp); //пишем ее на место дырки
      EEPROM.put(addr, nullmas); //а тут стираем
    }
  }
  EEPROM.end();
}
