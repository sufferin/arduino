// Пин, к которому подключен фоторезистор
const int photoresistorPin = A0;

// Флаг для управления режимом потоковой передачи данных
bool isStreaming = false;

// Переменные для неблокирующей задержки в режиме потока
unsigned long previousMillis = 0;
const long streamInterval = 2000; // Отправлять данные каждые 2 секунды (2000 мс)

void setup() {
  // Инициализируем последовательный порт для общения с компьютером
  Serial.begin(9600);
  
  // Настраиваем пин A0 как вход (хотя для аналоговых пинов это по умолчанию)
  pinMode(photoresistorPin, INPUT);
}

void loop() {
  // 1. Проверяем, пришла ли команда с компьютера
  if (Serial.available() > 0) {
    // Считываем команду
    char cmd = Serial.read();
    
    // Если команда 'p' (ping/poll) - запросить одно значение
    if (cmd == 'p') {
      isStreaming = false; // Останавливаем потоковый режим, если он был активен
      sendSensorValue();   // и отправляем одно значение
    } 
    // Если команда 's' (stream) - начать потоковую передачу
    else if (cmd == 's') {
      isStreaming = true;
      Serial.println("STREAM_STARTED");
    }
  }

  // 2. Если включен режим потоковой передачи
  if (isStreaming) {
    unsigned long currentMillis = millis(); // Получаем текущее время
    
    // Проверяем, прошло ли 2 секунды с последней отправки
    if (currentMillis - previousMillis >= streamInterval) {
      previousMillis = currentMillis; // Сохраняем время последней отправки
      sendSensorValue();              // Отправляем значение
    }
  }
}

// Вспомогательная функция для считывания и отправки значения датчика
void sendSensorValue() {
  // Считываем значение с аналогового пина (будет от 0 до 1023)
  int sensorValue = analogRead(photoresistorPin);
  
  // Отправляем на компьютер в требуемом формате "SENSOR_VALUE:<значение>"
  Serial.print("SENSOR_VALUE:");
  Serial.println(sensorValue);
}