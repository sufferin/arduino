/*
 * Seven Segment Stopwatch with Override
 * Logic:
 * 1. Start: Wait for Serial input.
 * 2. Normal: Count up every second.
 * 3. User Input:
 *    - First time: Sets the start time (e.g., 42).
 *    - Next times: Overrides display for ONE second only (sequence continues in background).
 */

#include <avr/io.h>
#include <avr/interrupt.h>

int latchPin = 5;
int clockPin = 3;
int dataPin = 7;

bool digits[10][8] = {
  {1,1,0,1,1,1,0,1},  // 0
  {0,1,0,1,0,0,0,0},  // 1
  {1,1,0,0,1,1,1,0},  // 2
  {1,1,0,1,1,0,1,0},  // 3
  {0,1,0,1,0,0,1,1},  // 4
  {1,0,0,1,1,0,1,1},  // 5
  {1,0,1,1,1,1,1,1},  // 6
  {1,1,0,1,0,0,0,0},  // 7
  {1,1,0,1,1,1,1,1},  // 8
  {1,1,1,1,1,0,1,1}   // 9
};

volatile int internal_counter = 0;
volatile int override_value = -1;
volatile bool system_running = false;

volatile uint16_t shift_buffer = 0;  // 16 бит для отправки
volatile int bit_index = 15;         // Какой бит сейчас отправляем
volatile int ms_counter = 0;         // Счетчик миллисекунд

enum State { STATE_WAIT, STATE_SHIFT, STATE_LATCH };
volatile State current_state = STATE_WAIT;

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
  
  digitalWrite(clockPin, LOW);
  digitalWrite(latchPin, HIGH);
  
  Serial.begin(9600);
  Serial.println(F("SYSTEM READY. Send start value (e.g. 42)..."));

  cli(); // Отключить прерывания
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  
  // OCR1A = (16*10^6) / (64 * 1000) - 1 = 249
  OCR1A = 249;            
  TCCR1B |= (1 << WGM12);   // CTC режим
  TCCR1B |= (1 << CS11) | (1 << CS10);  // Prescaler 64
  TIMSK1 |= (1 << OCIE1A);  // Включить прерывание по совпадению A
  sei(); // Включить прерывания
}

void loop() {
  static char input_buf[3]; 
  static byte pos = 0;

  if (Serial.available()) {
    char c = Serial.read();
    
    if (c >= '0' && c <= '9') {
      if (pos < 2) input_buf[pos++] = c;
    }
    
    // Обработка ввода (Enter или 2 цифры)
    if (pos == 2 || c == '\n') {
      input_buf[pos] = 0;
      int val = atoi(input_buf);
      
      if (val >= 0 && val <= 99) {
        // меняем переменные, используемые в прерывании
        cli();
        
        if (!system_running) {
          // ПЕРВЫЙ ЗАПУСК
          internal_counter = val;    // Устанавливаем начальное время
          system_running = true;     // Запускаем часы
          ms_counter = 999;          // Форсируем обновление дисплея прямо сейчас
          Serial.print(F("STARTED at: ")); Serial.println(val);
        } else {
          // ПОДМЕНА (Override)
          override_value = val;      // Записываем значение для "глюка"
          // Внутренний internal_counter НЕ МЕНЯЕМ!
          Serial.print(F("OVERRIDE next frame with: ")); Serial.println(val);
        }
        
        sei();
      }
      pos = 0;
    }
  }
}

// Вспомогательная функция для сборки 16 бит из массива digits
uint16_t encode_number(int number) {
  int tens = number / 10;
  int ones = number % 10;
  
  uint16_t bits = 0;
  
  // Сначала загружаем Единицы (они уйдут в первый регистр/дальний)
  // Или Десятки, в зависимости от порядка подключения.
  // Логика каскада: Первый отправленный бит уползает в самый конец.
  
  // Добавляем биты Десятков (старший байт)
  for(int i=7; i>=0; i--) {
     if(digits[tens][i]) bits |= (1 << (8 + i));
  }
  
  // Добавляем биты Единиц (младший байт)
  for(int i=7; i>=0; i--) {
     if(digits[ones][i]) bits |= (1 << i);
  }
  
  return bits;
}

// --- ПРЕРЫВАНИЕ ТАЙМЕРА (1000 раз в секунду) ---
ISR(TIMER1_COMPA_vect) {
  // 1. Считаем миллисекунды
  ms_counter++;

  // 2. Логика Секундомера (раз в секунду)
  if (ms_counter >= 1000) {
    ms_counter = 0;
    
    if (system_running) {
      // Определяем, что показать
      int value_to_show;
      
      if (override_value != -1) {
        // Если есть задание на подмену - показываем его
        value_to_show = override_value;
        override_value = -1; // Сбрасываем подмену (одноразовая)
        
        // ВАЖНО: Мы все равно увеличиваем внутренний счетчик, 
        // чтобы "время шло" в фоне
        internal_counter++; 
      } else {
        // Обычный режим
        value_to_show = internal_counter;
        internal_counter++;
      }

      if (internal_counter > 99) internal_counter = 0; // Цикл 0-99

      // Подготовка данных для сдвига
      shift_buffer = encode_number(value_to_show);
      
      // Запуск процесса передачи
      current_state = STATE_SHIFT;
      bit_index = 15; // 16 бит (0..15)
    }
  }

  // 3. Машина состояний для управления пинами (Shift Register)
  switch (current_state) {
    case STATE_SHIFT:
      // Выставляем бит данных
      if ((shift_buffer >> bit_index) & 1) {
        PORTD |= (1 << 7); // Data Pin 7 HIGH
      } else {
        PORTD &= ~(1 << 7); // Data Pin 7 LOW
      }
      
      // Дергаем Clock (Pin 3)
      PORTD |= (1 << 3);  // Clock HIGH
      PORTD &= ~(1 << 3); // Clock LOW
      
      bit_index--;
      if (bit_index < 0) {
        current_state = STATE_LATCH;
      }
      break;

    case STATE_LATCH:
      // Дергаем Latch (Pin 5) чтобы обновить дисплей
      PORTD |= (1 << 5);  // Latch HIGH
      PORTD &= ~(1 << 5); // Latch LOW
      
      current_state = STATE_WAIT; // Ждем следующей секунды
      break;

    case STATE_WAIT:
      // Ничего не делаем, ждем
      break;
  }
}
