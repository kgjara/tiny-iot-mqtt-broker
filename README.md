# Tiny IoT MQTT Broker

A lightweight IoT project featuring:
- **Publisher**: ESP32 with DHT22 and potentiometer sensors (Wokwi simulator)
- **Gateway**: C application that aggregates data from multiple publishers
- **Broker**: Custom MQTT-like broker in C using TCP sockets (no external libraries)
- **Subscriber**: Command-line subscriber in C to receive messages

## Protocol

Custom protocol over TCP sockets:
```
PUB <topic> <payload>   # Publisher sends data
SUB <topic>             # Subscriber subscribes to topic
UNSUB <topic>           # Subscriber unsubscribes
```

## Files

- `publisher_esp32/` - Arduino code for ESP32 with FreeRTOS tasks
- `broker.c` - MQTT-like broker with subscription management
- `gateway.c` - Gateway that bridges publishers to broker
- `subscriber.c` - CLI subscriber to view published messages

## Building & Running

### Broker
```bash
gcc -o broker broker.c -pthread
./broker 1883
```

### Gateway
```bash
gcc -o gateway gateway.c -pthread
./gateway 192.168.1.50 1883 5000
```

### Subscriber
```bash
gcc -o subscriber subscriber.c
./subscriber 192.168.1.50 1883
```

### Publisher (ESP32 in Wokwi)
Upload the sketch to your Wokwi project: https://wokwi.com/projects/449363833910091777

## Architecture

```
ESP32 (Wokwi)  -->  Gateway  -->  Broker  <--  Subscriber (CLI)
[Publisher]    [Aggregator]    [Router]     [Consumer]
```

## Notes

- Uses FreeRTOS for multitasking on ESP32
- TCP sockets for reliable communication
- Thread-based broker for handling multiple clients
- Educational implementation without external MQTT library
