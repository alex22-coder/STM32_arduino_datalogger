#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

// Пины для LED и SD карты
#define LED_PIN PB12

// Пины для SPI (SD карта)
#define SD_CS_PIN PA4
#define SD_SCK_PIN PB13
#define SD_MISO_PIN PB14  
#define SD_MOSI_PIN PB15

// Аналоговые входы
#define ANALOG_0 PA0
#define ANALOG_1 PA1
#define ANALOG_2 PA2
#define ANALOG_3 PA3

// Переменные для конфигурации
unsigned long logInterval = 1000;
String dataFileName = "";
bool sdInitialized = false;
bool isWriting = false;

// функция для поиска следующего номера лог-файла
int findNextLogNumber() {
  // Проверяем существование файлов по порядку от 1 до 999
  for (int i = 1; i <= 999; i++) {
    char testFileName[20];
    sprintf(testFileName, "log%03d.csv", i);
    
    if (!SD.exists(testFileName)) {
      return i;
    }
  }
  
  // Если все номера до 999 заняты, начинаем с 1
  return 1;
}

// Функция для создания имени нового лог-файла
String createNewLogFileName() {
  int nextNumber = findNextLogNumber();
  char fileName[20];
  sprintf(fileName, "log%03d.csv", nextNumber);
  return String(fileName);
}

// Функция для чтения конфигурации из файла
bool readConfig() {
  if (!SD.exists("config.txt")) {
    return false;
  }
  
  File configFile = SD.open("config.txt");
  if (!configFile) {
    return false;
  }
  
  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();
    
    if (line.startsWith("INTERVAL=")) {
      logInterval = line.substring(9).toInt();
    }
  }
  
  configFile.close();
  return true;
}

// Функция создания заголовка CSV файла
void createCSVHeader() {
  // Удаляем файл если он существует (на всякий случай)
  if (SD.exists(dataFileName.c_str())) {
    SD.remove(dataFileName.c_str());
  }
  
  File dataFile = SD.open(dataFileName.c_str(), FILE_WRITE);
  if (dataFile) {
    dataFile.println("Timestamp(ms),Analog_0,Analog_1,Analog_2,Analog_3");
    dataFile.close();
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Настройка SPI для SD карты
  SPI.setMOSI(SD_MOSI_PIN);
  SPI.setMISO(SD_MISO_PIN);
  SPI.setSCLK(SD_SCK_PIN);
  
  // Инициализация SD карты
  if (SD.begin(SD_CS_PIN)) {
    sdInitialized = true;
    
    // Создаем имя для нового лог-файла
    dataFileName = createNewLogFileName();
    
    // Читаем конфигурацию
    readConfig();
    
    // Создаем заголовок CSV файла
    createCSVHeader();
    
    // Мигнем LED 3 раза для индикации успешной инициализации SD карты
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  } else {
    sdInitialized = false;
    // Быстро мигаем при ошибке инициализации SD карты
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(50);
      digitalWrite(LED_PIN, LOW);
      delay(50);
    }
  }
  
  // Настройка аналоговых входов
  pinMode(ANALOG_0, INPUT_ANALOG);
  pinMode(ANALOG_1, INPUT_ANALOG);
  pinMode(ANALOG_2, INPUT_ANALOG);
  pinMode(ANALOG_3, INPUT_ANALOG);
}

void loop() {
  static unsigned long lastLogTime = 0;
  unsigned long currentTime = millis();
  
  // Логика работы светодиода:
  if (!sdInitialized) {
    // Нет SD карты - быстрое мигание (500мс)
    static unsigned long lastLedTime = 0;
    if (currentTime - lastLedTime >= 500) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastLedTime = currentTime;
    }
  } else if (isWriting) {
    // Идет запись - светодиод постоянно горит
    digitalWrite(LED_PIN, HIGH);
  } else {
    // Ожидание - медленное мигание (1000мс)
    static unsigned long lastLedTime = 0;
    if (currentTime - lastLedTime >= 1000) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      lastLedTime = currentTime;
    }
  }
  
  // Проверяем время для записи данных
  if (currentTime - lastLogTime >= logInterval && sdInitialized) {
    lastLogTime = currentTime;
    isWriting = true;
    
    // Читаем аналоговые входы
    int analog0 = analogRead(ANALOG_0);
    int analog1 = analogRead(ANALOG_1);
    int analog2 = analogRead(ANALOG_2);
    int analog3 = analogRead(ANALOG_3);
    
    // Записываем данные в файл
    File dataFile = SD.open(dataFileName.c_str(), FILE_WRITE);
    if (dataFile) {
      dataFile.print(currentTime);
      dataFile.print(",");
      dataFile.print(analog0);
      dataFile.print(",");
      dataFile.print(analog1);
      dataFile.print(",");
      dataFile.print(analog2);
      dataFile.print(",");
      dataFile.println(analog3);
      dataFile.close();
    }
    
    isWriting = false;
  }
  
  delay(50);
}