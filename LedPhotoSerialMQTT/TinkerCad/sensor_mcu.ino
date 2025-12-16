const int photoresistorPin = A0;
bool isStreaming = false;
unsigned long previousMillis = 0;
const long streamInterval = 2000; // Интервал потоковой передачи 2 секунды

void setup() {
  Serial.begin(9600);
  pinMode(photoresistorPin, INPUT);
}

void loop() {
  // Обработка входящих команд
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'p') {
      isStreaming = false; // Останавливаем поток, если он был активен
      sendSensorValue();
    } else if (cmd == 's') {
      isStreaming = true;
      Serial.println("STREAM_STARTED");
    }
  }

  // Логика потоковой передачи
  if (isStreaming) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= streamInterval) {
      previousMillis = currentMillis;
      sendSensorValue();
    }
  }
}

void sendSensorValue() {
  int sensorValue = analogRead(photoresistorPin);
  Serial.print("SENSOR_VALUE:");
  Serial.println(sensorValue);
}