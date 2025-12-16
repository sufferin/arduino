/*
 * IoT Матричная Клавиатура - Реализация Пункта 2
 * Функционал: Отслеживание длительности и времени начала нажатия + Журналирование событий
 * Архитектура: Сканирование в прерываниях Timer1 (Фон) + Логика фронтов в loop (Основной поток)
*/

#include <avr/io.h>
#include <avr/interrupt.h>

// --- Конфигурация ---
#define NROWS 3
#define NCOLS 3

// --- Глобальное состояние ---
// Хранит "живое" состояние кнопок (обновляется мгновенно в ISR)
// uint16_t нужен, чтобы вместить 9 бит (кнопок)
volatile uint16_t button_state_bitmap = 0; 
volatile int current_row = 0;

// Массив для хранения времени начала нажатия каждой кнопки (в мс)
unsigned long btn_start_times[9];

// Макрос для перевода координат (строка, столбец) в индекс (0-8)
#define GET_BTN_INDEX(row, col) ((row * NCOLS) + col)

void setup() {
  Serial.begin(9600);

  // --- 1. Настройка Портов (Прямой доступ к регистрам) ---
  // Строки (Выходы): D2, D3, D4
  // Столбцы (Входы): D5, D6, D7
  DDRD |= 0x1C;  // Устанавливаем биты 2,3,4 в 1 (OUTPUT)
  DDRD &= ~0xE0; // Сбрасываем биты 5,6,7 в 0 (INPUT)
  PORTD |= 0x1C; // Строки в HIGH (Неактивны)
  PORTD |= 0xE0; // Включаем подтяжку (Pull-ups) для столбцов

  // --- 2. Настройка Timer1 (Режим CTC, ~5мс на строку) ---
  noInterrupts();           // Отключаем прерывания на время настройки
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 1249;             // Число для сравнения (для 5мс интервала)
  TCCR1B |= (1 << WGM12);   // Включаем режим CTC
  TCCR1B |= (1 << CS11) | (1 << CS10); // Предделитель 64
  TIMSK1 |= (1 << OCIE1A);  // Разрешаем прерывание
  interrupts();             // Включаем прерывания обратно
  
  Serial.println("Система готова. Отслеживание длительности включено.");
}

void loop() {
  // Переменная для хранения состояния с предыдущего цикла loop
  static uint16_t last_processed_state = 0;
  uint16_t current_state_copy;

  // 1. Атомарное чтение состояния (защита от изменения данных во время чтения)
  noInterrupts();
  current_state_copy = button_state_bitmap;
  interrupts();

  // 2. Если состояние изменилось (кто-то нажал или отпустил кнопку)
  if (current_state_copy != last_processed_state) {
    unsigned long current_time = millis();

    // Проходимся по всем 9 кнопкам, чтобы найти, какая именно изменилась
    for (int i = 0; i < 9; i++) {
      // Получаем состояние i-й кнопки (1 - нажата, 0 - отпущена)
      bool is_pressed_now = (current_state_copy >> i) & 1;
      bool was_pressed_before = (last_processed_state >> i) & 1;

      // ОБНАРУЖЕНИЕ НАЖАТИЯ (Rising Edge: было 0, стало 1)
      if (is_pressed_now && !was_pressed_before) {
        btn_start_times[i] = current_time; // Запоминаем время старта
      }

      // ОБНАРУЖЕНИЕ ОТПУСКАНИЯ (Falling Edge: было 1, стало 0) -> Требование Пункта 2
      if (!is_pressed_now && was_pressed_before) {
        // Вычисляем, сколько времени кнопка удерживалась
        unsigned long duration = current_time - btn_start_times[i];
        
        Serial.print(">> Кнопка ");
        Serial.print(i + 1);
        Serial.print(" отпущена. Длительность: ");
        Serial.print(duration);
        Serial.print(" мс. Начало в: ");
        Serial.print(btn_start_times[i]);
        Serial.println(" мс.");
      }
    }

    // 3. Вывод общего списка зажатых кнопок (Требование Пункта 1)
    printCurrentList(current_state_copy);

    // Обновляем историю для следующего сравнения
    last_processed_state = current_state_copy;
  }
  
  delay(50); // Небольшая задержка для стабилизации вывода
}

// --- Помощник: Печать списка нажатых кнопок ---
void printCurrentList(uint16_t state) {
  if (state == 0) {
    Serial.println("Статус: Все кнопки отпущены.");
    return;
  }

  Serial.print("Статус: нажаты кнопки [ ");
  bool first = true;
  for (int i = 0; i < 9; i++) {
    if ((state >> i) & 1) {
      if (!first) Serial.print(", ");
      Serial.print(i + 1);
      first = false;
    }
  }
  Serial.println(" ]");
}

// --- Обработчик Прерывания (ISR) - Аппаратное сканирование ---
ISR(TIMER1_COMPA_vect) {
  // 1. Выключаем текущую строку (HIGH)
  PORTD |= (1 << (current_row + 2)); 

  // 2. Переходим к следующей
  current_row++;
  if (current_row >= NROWS) {
    current_row = 0;
  }

  // 3. Включаем новую строку (LOW - активный уровень)
  PORTD &= ~(1 << (current_row + 2)); 

  // 4. Читаем состояние порта D
  uint8_t pin_state = PIND; 
  
  for (int col = 0; col < NCOLS; col++) {
    // Проверяем бит (Active LOW: 0 - нажато)
    bool is_pressed = !(pin_state & (1 << (5 + col)));
    int btn_index = GET_BTN_INDEX(current_row, col);
    
    // Записываем бит в карту (используем 1U для корректного сдвига в 16 бит)
    if (is_pressed) {
      button_state_bitmap |= (1U << btn_index); 
    } else {
      button_state_bitmap &= ~(1U << btn_index); 
    }
  }
}