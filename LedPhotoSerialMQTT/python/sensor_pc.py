import serial
import time
import paho.mqtt.client as mqtt
import random

SERIAL_PORT = 'COM8' 
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
# Уникальный ID клиента, чтобы избежать конфликтов на публичном брокере
CLIENT_ID = f'iot-project-sensor-publisher-{random.randint(0, 1000)}'

# Топики для публикации
LUMINOSITY_TOPIC = "iot/home/luminosity"
STATUS_TOPIC = "iot/home/sensor_status"

def on_connect(client, userdata, flags, rc):
    """Callback-функция, вызываемая при подключении к MQTT брокеру."""
    if rc == 0:
        print("Подключено к MQTT брокеру!")
        # Публикуем сообщение о том, что клиент-датчик подключился
        client.publish(STATUS_TOPIC, "Sensor client connected", qos=1, retain=True)
    else:
        print(f"Не удалось подключиться, код ошибки: {rc}")

def setup_serial():
    """Настраивает и открывает Serial-соединение с Arduino."""
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"Подключено к Arduino на порту {SERIAL_PORT}")
        time.sleep(2) # Даем время на перезагрузку Arduino после открытия порта
        return ser
    except serial.SerialException as e:
        print(f"Ошибка: Не удалось открыть порт {SERIAL_PORT}. Убедитесь, что порт правильный и не занят другой программой (например, Монитором порта в Arduino IDE).")
        print(e)
        exit()

if __name__ == "__main__":
    # Настройка MQTT клиента
    client = mqtt.Client(client_id=CLIENT_ID)
    client.on_connect = on_connect
    # "Последняя воля": если клиент отключится некорректно, брокер опубликует это сообщение
    client.will_set(STATUS_TOPIC, "Sensor client disconnected unexpectedly", qos=1, retain=True)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start() # Запускаем сетевой цикл в фоновом потоке

    # Настройка Serial порта
    arduino = setup_serial()

    try:
        # Бесконечный цикл опроса датчика
        while True:
            # Отправляем команду 'p' для получения одного значения
            arduino.write(b'p')
            
            # Ждем ответа от Arduino
            if arduino.in_waiting > 0:
                response = arduino.readline().decode('utf-8').strip()
                
                # Проверяем, что ответ имеет правильный формат
                if response.startswith("SENSOR_VALUE:"):
                    try:
                        # Извлекаем числовое значение
                        value = response.split(':')[1]
                        print(f"Получено значение освещенности: {value}")
                        
                        # Публикуем значение в MQTT топик
                        client.publish(LUMINOSITY_TOPIC, value, qos=1)
                        print(f"Опубликовано в топик {LUMINOSITY_TOPIC}")
                        
                    except (IndexError, ValueError) as e:
                        print(f"Ошибка парсинга ответа от Arduino: {response}, {e}")
            
            # Пауза перед следующим опросом
            time.sleep(5) 

    except KeyboardInterrupt:
        print("\nПрограмма завершена пользователем.")
    finally:
        # Корректно завершаем работу
        client.publish(STATUS_TOPIC, "Sensor client disconnected", qos=1, retain=True)
        client.disconnect()
        client.loop_stop()
        arduino.close()
        print("Соединения закрыты.")