import paho.mqtt.client as mqtt
import random
import time

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-monitor-{random.randint(0, 1000)}'
# Символ '#' - это wildcard (маска), который означает "подписаться на все подтопики"
# внутри iot/home/.
TOPIC = "iot/home/#"  

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT."""
    if rc == 0:
        print("Монитор успешно подключен к MQTT брокеру.")
        # Подписываемся на все топики после успешного подключения
        client.subscribe(TOPIC)
        print(f"Подписан на все топики в ветке: {TOPIC}")
    else:
        print(f"Ошибка подключения монитора, код: {rc}")

def on_message(client, userdata, msg):
    """Callback при получении любого сообщения."""
    # Получаем текущее время для лога
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
    # Выводим сообщение в удобном формате
    print(f"[{timestamp}] [{msg.topic}]: {msg.payload.decode('utf-8')}")

if __name__ == "__main__":
    client = mqtt.Client(client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    
    try:
        print("Запуск MQTT монитора. Все сообщения из топиков iot/home/* будут отображаться здесь.")
        print("Нажмите CTRL+C для выхода.")
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nМонитор остановлен.")
    finally:
        client.disconnect()