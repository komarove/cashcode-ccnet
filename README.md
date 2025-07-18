# CashCode-CCNET: Тестовый проект для работы с купюроприёмником/валидатором по протоколу CCNET

## Описание

Этот проект предназначен для тестирования работы купюроприёмника по протоколу CCNET с использованием Arduino и ESP32. В репозитории представлены два скетча:
- `arduino/arduino.ino` — для Arduino (Uno/Nano и др.)
- `esp32/esp32.ino` — для ESP32

## Схемы подключения

- **Реальное подключение:**
  ![Реальное подключение](docs/connect.png)
- **Схематичное подключение:**
  ![Схема подключения](docs/esp.png)

## Протокол обмена (CCNET)

Для управления купюроприёмником используются следующие команды:

```cpp
uint8_t CashCodeReset[]  = {0x02, 0x03, 0x06, 0x30, 0x41, 0xB3};
uint8_t CashCodeAck[]    = {0x02, 0x03, 0x06, 0x00, 0xC2, 0x82};
uint8_t CashCodePoll[]   = {0x02, 0x03, 0x06, 0x33, 0xDA, 0x81};
uint8_t CashCodeEnable[] = {0x02, 0x03, 0x0C, 0x34, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x66, 0xC1};
```

### Расшифровка байтов команд

| № | Назначение         | Пример значения | Описание                                      |
|---|--------------------|-----------------|-----------------------------------------------|
| 1 | Start byte         | 0x02            | Начало пакета                                 |
| 2 | Address            | 0x03            | Адрес устройства (обычно 0x03 для CCNET)      |
| 3 | Length             | 0x06/0x0C       | Длина пакета (включая команду, данные, CRC)   |
| 4 | Command            | 0x30/0x00/0x33/0x34 | Код команды (Reset, Ack, Poll, Enable)    |
| 5+| Data/CRC           | ...             | Данные команды или часть CRC                  |
| n | CRC                | ...             | Контрольная сумма пакета                      |

**Примеры:**
- `CashCodeReset`: 0x02 (Start), 0x03 (Addr), 0x06 (Len), 0x30 (Reset), 0x41, 0xB3 (CRC)
- `CashCodeAck`:   0x02, 0x03, 0x06, 0x00 (Ack), 0xC2, 0x82 (CRC)
- `CashCodePoll`:  0x02, 0x03, 0x06, 0x33 (Poll), 0xDA, 0x81 (CRC)
- `CashCodeEnable`: 0x02, 0x03, 0x0C, 0x34 (Enable), 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x66, 0xC1 (данные + CRC)

> **Примечание:**
> CRC (контрольная сумма) рассчитывается по алгоритму, описанному в документации на CCNET.

## Таблица ошибок (индикация светодиодами)

| Кол-во миганий         | Описание неисправности                                                                 | Критические ошибки, требующие вмешательства |
|------------------------|----------------------------------------------------------------------------------------|:------------------------------------------:|
| 1 красный на чёрном    | Кассета извлечена из купюроприёмника                                                  | ✔ |
| 3 красный на чёрном    | Кассета заполнена                                                                     | ✔ |
| 4 красный на чёрном    | Неисправность механизма укладки                                                       | ✔ |
| 5 красный на чёрном    | Неисправность ёмкостных датчиков                                                      | ✔ |
| 6 красный на чёрном    | Неисправность оптических датчиков                                                     | ✔ |
| 7 красный на чёрном    | Неисправность магнитных датчиков                                                      | ✔ |
| 8 красный на чёрном    | Неисправность двигателя транспортировки (таймаут)                                     | ✔ |
| 10 красный на чёрном   | Неисправность механизма выравнивания                                                  | ✔ |
| 11 красный на чёрном   | Купюропровод не пуст (зажата купюра)                                                  | ✔ |
| 12 красный на чёрном   | Ошибка возврата купюры. Купюра в приёмном слоте кассеты                               | ✔ |
| 1 зелёный на красном   | Ошибка CRC COM порта                                                                  | ✖ |
| 2 зелёный на красном   | Внутренняя ошибка CRC                                                                 | ✖ |
| 3 зелёный на красном   | Неправильный формат CCMS                                                              | ✖ |
| 4 зелёный на красном   | CCMS отсутствует                                                                      | ✖ |
| 5 зелёный на красном   | Неправильный тип CCMS                                                                 | ✖ |
| 6 зелёный на красном   | Ошибка загрузки                                                                       | ✖ |

> **Примечание:**
> Если подать питание нестабильное, ниже 12 вольт или менее 2 ампер, вероятно купюроприёмник не будет работать и будет постоянно мигать красным.

---

## Таблица кодов ошибок

| Код  | Название/Описание (англ.)               | Описание (рус.)                              | Критические ошибки, требующие вмешательства |
|------|-----------------------------------------|----------------------------------------------|:------------------------------------------:|
| 0x10 | Defective Motor                         | Неисправность двигателя                      | ✔ |
| 0x11 | Sensor Problem                          | Проблема с датчиком                          | ✔ |
| 0x12 | Bill Jam                                | Застревание купюры                           | ✔ |
| 0x13 | Stacker Open                            | Кассета открыта                              | ✔ |
| 0x14 | Stacker Full                            | Кассета заполнена                            | ✔ |
| 0x15 | Stacker Problem                         | Проблема с кассетой                          | ✔ |
| 0x1C | Cheated                                 | Попытка мошенничества                        | ✔ |
| 0x1D | Pause                                   | Пауза                                        | ✖ |
| 0x1E | Generic Failure                         | Общая ошибка                                 | ✔ |
| 0x1F | Invalid Command                         | Неверная команда                             | ✖ |
| 0x20 | Escrow Position                         | Купюра в позиции эскроу                      | ✖ |
| 0x30 | Power Up                                | Включение питания                            | ✖ |
| 0x41 | Power Up with Bill in Validator         | Включение с купюрой внутри                   | ✔ |
| 0x42 | Power Up with Bill in Stacker           | Включение с купюрой в кассете                | ✔ |
| 0x43 | Power Up with Bill in Escrow            | Включение с купюрой в эскроу                 | ✔ |
| 0x44 | Stacker Removed                         | Кассета извлечена                            | ✔ |
| 0x45 | Stacker Inserted                        | Кассета вставлена                            | ✖ |
| 0x46 | Drop Cassette Out of Position           | Кассета не на месте                          | ✔ |
| 0x47 | Drop Cassette Jammed                    | Кассета заклинила                            | ✔ |
| 0x48 | Drop Cassette Full                      | Кассета заполнена                            | ✔ |
| 0x49 | Drop Cassette Sensor Error              | Ошибка датчика кассеты                       | ✔ |

> **Примечание:**
> В зависимости от модели и прошивки купюроприёмника, список кодов может отличаться. Для модели MSM-3024 полный список приведён выше.

## Использование

1. Залейте соответствующий скетч на Arduino или ESP32.
2. Подключите купюроприёмник согласно схеме (см. картинки выше).
3. Приёмник будет инициализирован, включён и начнёт опрашиваться.

## Документация
- [MSM-Manual_P1_1.pdf](docs/MSM-Manual_P1_1.pdf) — официальный мануал купюроприёмника
- [MSM-3024_C.pdf](docs/MSM-3024_C.pdf) — официальный мануал купюроприёмника

---

**Вопросы и предложения:**
Пишите в issues.