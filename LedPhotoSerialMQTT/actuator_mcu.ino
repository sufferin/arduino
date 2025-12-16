// Пин, к которому подключен светодиод. 
// На большинстве плат Nano, как и на Uno, пин 13 имеет встроенный светодиод.
const int ledPin = 13;

// Переменная для хранения текущего режима работы.
// 'd' - down (выключен), 'u' - up (включен), 'b' - blink (мигание).
char mode = 'd'; 

// Переменные для неблокирующего мигания.
bool ledState = LOW;
unsigned long previousMillis = 0;
const long blinkInterval = 500; // Интервал мигания: 500 мс вкл, 500 мс выкл.

void setup() {
  // Инициализируем последовательный порт для общения с компьютером/другим Arduino.
  Serial.begin(9600);
  
  // Настраиваем пин светодиода как выход.
  pinMode(ledPin, OUTPUT);
  
  // Устанавливаем начальное состояние светодиода (выключен).
  digitalWrite(ledPin, LOW);
}

void loop() {
  // 1. Проверяем, пришла ли новая команда.
  if (Serial.available() > 0) {
    // Считываем команду.
    char cmd = Serial.read();
    
    if (cmd == 'u') { // Команда "Up" - включить
      mode = 'u';
      digitalWrite(ledPin, HIGH); // Сразу включаем светодиод
      Serial.println("LED_GOES_ON");
    } 
    else if (cmd == 'd') { // Команда "Down" - выключить
      mode = 'd';
      digitalWrite(ledPin, LOW); // Сразу выключаем светодиод
      Serial.println("LED_GOES_OFF");
    } 
    else if (cmd == 'b') { // Команда "Blink" - мигать
      mode = 'b';
      Serial.println("LED_WILL_BLINK");
    }
  }

  // 2. Если текущий режим - мигание, выполняем логику мигания.
  // Эта часть кода работает постоянно, но меняет светодиод только в режиме 'b'.
  if (mode == 'b') {
    unsigned long currentMillis = millis(); // Получаем текущее время
    
    // Проверяем, прошло ли достаточно времени для переключения светодиода.
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis; // Сохраняем время переключения
      
      // Инвертируем состояние светодиода (если был включен - выключаем, и наоборот).
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}