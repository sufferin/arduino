const int ledPin = 13;
char mode = 'd'; // 'u' - on, 'd' - off, 'b' - blink
bool ledState = LOW;
unsigned long previousMillis = 0;
const long blinkInterval = 500; // Интервал мигания

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  // Обработка команд
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'u') {
      mode = 'u';
      digitalWrite(ledPin, HIGH);
      Serial.println("LED_GOES_ON");
    } else if (cmd == 'd') {
      mode = 'd';
      digitalWrite(ledPin, LOW);
      Serial.println("LED_GOES_OFF");
    } else if (cmd == 'b') {
      mode = 'b';
      Serial.println("LED_WILL_BLINK");
    }
  }

  // Логика мигания
  if (mode == 'b') {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}