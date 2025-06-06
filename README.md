# ESP8266-based WiFi-MQTT Power Control Module

This project implements a configurable power control module using an ESP8266 microcontroller with WiFi connectivity and MQTT communication. It provides a web-based configuration interface and allows remote power control through MQTT messages.

The system creates a user-friendly web interface for initial setup, storing configuration in EEPROM for persistence. Once configured, it connects to the specified WiFi network and MQTT broker to enable remote power control. The device features automatic reconnection capabilities and LED status indicators for visual feedback.

## Repository Structure
```
.
├── platformio.ini          # PlatformIO configuration file with build settings and dependencies
├── src/
│   └── main.cpp           # Core application logic for WiFi, MQTT, and power control
└── .gitignore             # Git ignore rules for PlatformIO and VSCode files
```

## Usage Instructions
### Prerequisites
- PlatformIO IDE or CLI
- ESP8266 development board (ESP-01 model)
- USB-to-Serial adapter
- 3.3V power supply
- Access to WiFi network
- MQTT broker (for remote control)

Required Libraries:
- PubSubClient
- ESPAsyncWebServer-esphome
- ESP8266WiFi (included in ESP8266 core)

### Installation

1. Clone the repository:
```bash
git clone [repository-url]
cd [repository-name]
```

2. Install PlatformIO (if not already installed):
```bash
pip install platformio
```

3. Build the project:
```bash
platformio run
```

4. Upload to ESP8266:
```bash
platformio run --target upload
```

### Quick Start

1. Power up the ESP8266 device
2. Connect to the WiFi access point "ESP8266-Access-Point" with password "123456789"
3. Open a web browser and navigate to the device's IP address (typically 192.168.4.1)
4. Configure the following parameters:
   - Device ID (number between 0 and 99)
   - WiFi SSID and password
   - MQTT broker IP and port
   - MQTT username and password
5. Click "Save configuration" to store settings

### More Detailed Examples

MQTT Topic Structure:
```
scr/[device-id]/in    # Control topic
scr/[device-id]/out   # Status topic
scr/debug/[device-id] # Debug messages
```

Power Control via MQTT:
```bash
# Example Set power to 50% for deviceId 0
mosquitto_pub -t "scr/0/in" -m "50"

# Example Turn power off
mosquitto_pub -t "scr/0/in" -m "0"
```

### Troubleshooting

Common Issues:
1. Device not creating AP mode
   - Check power supply voltage (needs 3.3V)
   - Reset device and wait 30 seconds
   - Check LED status indicators

2. Cannot connect to WiFi
   - Verify SSID and password
   - Ensure 2.4GHz network availability
   - Check signal strength
   - Monitor serial output at 115200 baud

3. MQTT connection fails
   - Verify broker IP and port
   - Check MQTT credentials
   - Ensure broker is accessible from network
   - Enable debug messages for detailed logs

Debug Mode:
- Connect to serial monitor at 115200 baud
- Monitor LED blink patterns:
  * Fast blink: AP Mode ON for wifi configuration (it happends when the module is unable to connect to wifi)
  * Normal blink: Connected to WiFi
  * Off: Check powersupply 

## Data Flow
The system processes power control commands through MQTT messages and manages device configuration through a web interface with EEPROM storage.

```ascii
[WiFi Client] <-> [ESP8266] <-> [MQTT Broker]
     ^              |
     |              v
[Web Config] <-> [EEPROM]
```

Component Interactions:
1. Web interface provides configuration input
2. EEPROM stores persistent configuration
3. WiFi module connects to configured network
4. MQTT client handles command and status messages
5. Power control module responds to MQTT commands
6. LED provides visual status feedback
7. Automatic reconnection handles network issues
