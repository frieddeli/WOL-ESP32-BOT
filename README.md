# ESP32 Wake-on-LAN (WOL) and Remote Shutdown Bot

This project leverages an ESP32 microcontroller to remotely wake or shut down a PC via Telegram commands. The design avoids port forwarding, ensuring security and ease of use while maintaining control over your PC's power state.

---

## **Key Benefits**

### 1. **No Port Forwarding Required**
   - By utilizing the Telegram Bot API, this design avoids exposing your network to external threats via port forwarding.
   - Communication is encrypted and routed through Telegram's servers, enhancing security.

### 2. **User Authentication**
   - The bot verifies the sender's `chatId` before executing commands, ensuring only authorized users can control the PC.

### 3. **Ease of Use**
   - Intuitive Telegram commands like `/wake`, `/shutdown`, and `/status` simplify remote operations.

### 4. **Expandable Design**
   - Additional functionality can be added with minimal modifications to the code.

---

## **Design Overview**

### **Hardware Components**
- ESP32 microcontroller
- A PC configured for Wake-on-LAN (WOL)
- (Optional) Relay module for physical shutdown functionality

### **Software Components**
- Arduino IDE with necessary libraries:
  - `WiFi.h` (ESP32 Wi-Fi connectivity)
  - `WiFiUdp.h` (UDP functionality for WOL)
  - `ESP8266Ping` (for PC status checks via ICMP ping)

---

## **Operation**

1. **Initialization**
   - The ESP32 connects to your Wi-Fi network and sets up Telegram Bot API communication using your unique bot token.
   - The bot verifies the sender's `chatId` for all incoming messages.

2. **Command Processing**
   - The bot listens for specific Telegram commands:
     - `/start`: Displays a welcome message and available commands.
     - `/wake`: Sends a WOL packet to wake the target PC.
     - `/status`: Pings the PC to check if it's online.
     - `/shutdown`: Activates a relay module to simulate a physical shutdown button press (optional).

3. **Communication Flow**
   - Telegram -> ESP32:
     - Authorized commands are sent via Telegram messages.
   - ESP32 -> Target PC:
     - WOL packets are sent via UDP to wake the PC.
     - Shutdown signals are sent via GPIO (if a relay is used).

4. **Security Features**
   - Commands are executed only if the sender's `chatId` matches the authorized `chatId`.
   - The bot operates within the Telegram framework, eliminating the need for public IP addresses or exposed ports.

---

## **Setup Instructions**

### 1. **Prepare Your PC**
- Enable Wake-on-LAN (WOL) in your PC's BIOS/UEFI settings.
- Configure your network adapter to allow WOL.

### 2. **Set Up the ESP32**
- Install the required libraries in the Arduino IDE.
- Clone or download this repository and open the `.ino` file in the Arduino IDE.
- Configure the following variables in the code:
  ```cpp
  const char* ssid = "YOUR_WIFI_SSID";       // Your Wi-Fi SSID
  const char* password = "YOUR_WIFI_PASSWORD"; // Your Wi-Fi Password
  const char* botToken = "YOUR_BOT_TOKEN";     // Telegram Bot Token
  const long authorizedChatId = 123456789;     // Your Telegram Chat ID
  const char* targetMac = "XX:XX:XX:XX:XX:XX"; // Target PC MAC Address
  const char* targetIP = "192.168.1.XX";      // Target PC IP Address
  ```
- Upload the code to the ESP32 using the Arduino IDE.

### 3. **Deploy**
- Connect the ESP32 to power and your Wi-Fi network.
- Interact with the bot using Telegram commands to wake or shut down your PC.


### 4. **LED Behaviour*
Poll Telegram - 2 Blinks 
No new message - LED ON
ERROR connecting - 5 Blinks 

---

## **Commands**

| Command    | Description                                                                 |
|------------|-----------------------------------------------------------------------------|
| `/start`   | Displays a welcome message and available commands.                         |
| `/wake`    | Sends a WOL packet to wake the PC.                                         |
| `/status`  | Checks if the PC is online using an ICMP ping.                             |
| `/shutdown`| (Optional) Activates a relay module to shut down the PC.                   |

---

## **Future Improvements**

- Implement more robust status detection (e.g., querying services on the PC).
- Add support for multiple authorized users.
- Integrate additional remote control functionality, such as restarting the PC.

---

## **License**
This project is licensed under the MIT License. See the `LICENSE` file for more details.

---

## **Contributions**
Feel free to fork this repository and contribute improvements via pull requests. Suggestions and bug reports are welcome!

