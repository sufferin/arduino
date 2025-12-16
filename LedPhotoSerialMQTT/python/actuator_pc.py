import serial
import time
import paho.mqtt.client as mqtt
import random

SERIAL_PORT = 'COM10'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-actuator-subscriber-{random.randint(0, 1000)}'

# Топики
LUMINOSITY_TOPIC = "iot/home/luminosity"
LIGHT_STATUS_TOPIC = "iot/home/light_status"
ACTUATOR_STATUS_TOPIC = "iot/home/actuator_status"

# Порог освещенности для включения/выключения света
LUMINOSITY_THRESHOLD = 10

# Глобальная переменная для доступа к Serial порту из callback-функции
arduino = None

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT."""
    if rc == 0:
        print("Подключено к MQTT брокеру!")
        client.publish(ACTUATOR_STATUS_TOPIC, "Actuator client connected", qos=1, retain=True)
        # Подписываемся на топик с данными об освещенности
        client.subscribe(LUMINOSITY_TOPIC)
        print(f"Подписан на топик: {LUMINOSITY_TOPIC}")
    else:
        print(f"Не удалось подключиться, код ошибки: {rc}")

def on_message(client, userdata, msg):
    """Callback при получении сообщения из подписанного топика."""
    global arduino
    try:
        # Декодируем сообщение и преобразуем в число
        luminosity = int(msg.payload.decode('utf-8'))
        print(f"Получено значение освещенности: {luminosity}")

        # Проверяем, что Arduino подключена
        if arduino and arduino.is_open:
            if luminosity < LUMINOSITY_THRESHOLD:
                # Если темно, отправляем команду 'u' (up/on)
                arduino.write(b'u')
                # Публикуем текущий статус света
                client.publish(LIGHT_STATUS_TOPIC, "ON", qos=1, retain=True)
                print("Команда Arduino: включить свет (u)")
            else:
                # Если светло, отправляем команду 'd' (down/off)
                arduino.write(b'd')
                client.publish(LIGHT_STATUS_TOPIC, "OFF", qos=1, retain=True)
                print("Команда Arduino: выключить свет (d)")
    except (ValueError, TypeError) as e:
        print(f"Не удалось обработать сообщение: {msg.payload}, {e}")
    except serial.SerialException as e:
        print(f"Ошибка записи в Serial порт: {e}")

def setup_serial():
    """Настраивает и открывает Serial-соединение с Arduino."""
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"Подключено к Arduino на порту {SERIAL_PORT}")
        time.sleep(2)
        return ser
    except serial.SerialException as e:
        print(f"Ошибка: Не удалось открыть порт {SERIAL_PORT}. Проверьте подключение и номер порта.")
        return None

if __name__ == "__main__":
    arduino = setup_serial()
    
    # Запускаем MQTT клиент только если удалось подключиться к Arduino
    if arduino:
        client = mqtt.Client(client_id=CLIENT_ID)
        client.on_connect = on_connect
        client.on_message = on_message
        client.will_set(ACTUATOR_STATUS_TOPIC, "Actuator client disconnected unexpectedly", qos=1, retain=True)
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        try:
            # loop_forever() - это блокирующий цикл, который слушает сеть и вызывает callbacks.
            # Идеально для подписчика, которому нужно только реагировать на сообщения.
            client.loop_forever() 
        except KeyboardInterrupt:
            print("\nПрограмма завершена пользователем.")
        finally:
            client.publish(ACTUATOR_STATUS_TOPIC, "Actuator client disconnected", qos=1, retain=True)
            client.disconnect()
            arduino.close()
            print("Соединения закрыты.")
    else:
        print("Не удалось подключиться к Arduino. Программа завершена.")