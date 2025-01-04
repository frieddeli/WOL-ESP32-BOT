#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
#include <ESP8266Ping.h>


// ======= USER CONFIGURATIONS =======
const char* WIFI_SSID = "XXXXXXXXXXX";
const char* WIFI_PASS = "XXXXXXXXXXXXXXXXX";

// Telegram API info
const char* BOT_TOKEN = "XXXXXXXXXXXXXXXXXXXXXXXXX"; // Replace with your actual Bot token
long YOUR_CHAT_ID = XXXXXXXXXXXXXXXXX; // Replace with your authorized user's chat ID

// PC MAC address for WOL
uint8_t targetMac[6] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};  // Replace with your PC's MAC

// PC IP for Status check Via ping
IPAddress pcIP(XXX, XXX, X, XX);  // IPV4

// Polling Interval (milliseconds)
const unsigned long POLL_INTERVAL = 4000;

// LED pin
const int LED_PIN = 2; 

ESP8266WiFiMulti wifiMulti;
WiFiClientSecure httpsClient;
WiFiUDP udp;

const char* TELEGRAM_HOST = "api.telegram.org";
long lastUpdateId = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off if active low

  Serial.println("Attempting to connect to WiFi...");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Still trying to connect to WiFi...");
    blinkLED(100); // Indicate connecting
    delay(100);
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED off if active low
  Serial.println("WiFi connected, LED is solid ON.");
  delay(5000);

  httpsClient.setInsecure(); // For simplicity in this example
  if(!udp.begin(9)) {
    Serial.println("Failed to start UDP for WOL");
  } else {
    Serial.println("UDP initialized on port 9 for WOL packets.");
  }
}

void loop() {
  Serial.println("Polling Telegram for new messages...");
  blinkLED(100);
  delay(100);
  blinkLED(100);  // blink 2 for poll 
  delay(100);

  checkForTelegramCommands();
  delay(POLL_INTERVAL);
}

void checkForTelegramCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Error: Not connected to WiFi, cannot check Telegram API.");
    for(int i =0 ; i<5 ; i++)
    {
      blinkLED(100);    // blink 5 for error 
      delay(100);
    }
    return;
  }

  String url = "/bot";
  url += BOT_TOKEN;
  url += "/getUpdates?timeout=10";

  if (lastUpdateId > 0) {
    url += "&offset=";
    url += String(lastUpdateId + 1);
  }

  Serial.println("Connecting to Telegram API...");
  if (!httpsClient.connect(TELEGRAM_HOST, 443)) {
    Serial.println("Connection to Telegram failed.");
    for(int i =0 ; i<5 ; i++)
    {
      blinkLED(100);    // blink 5 for error 
      delay(100);
    }
    return;
  }

  httpsClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                    "Host: " + TELEGRAM_HOST + "\r\n" +
                    "Connection: close\r\n\r\n");

  // Read entire HTTP response
  String response = "";
  unsigned long startTime = millis();
  while ((httpsClient.connected() || httpsClient.available()) && millis() - startTime < 5000) {
    while (httpsClient.available()) {
      response += (char)httpsClient.read();
    }
    delay(10);
  }
  httpsClient.stop();

  if (response.length() == 0) {
    Serial.println("No data received from Telegram.");
    digitalWrite(LED_PIN, LOW); // LED ON TO SHOW NO DATA
    delay(500);
    return;
  }

  Serial.println("RAW JSON Payload from Telegram:");
  Serial.println(response);

  int jsonIndex = response.indexOf("\r\n\r\n");
  if (jsonIndex == -1) {
    Serial.println("Error: No JSON body found.");
    return;
  }

  String payload = response.substring(jsonIndex + 4);

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }

  bool ok = doc["ok"];
  if (!ok) {
    Serial.println("Telegram API returned not ok. Check bot token or API status.");
    return;
  }

  JsonArray result = doc["result"].as<JsonArray>();
  if (result.isNull() || result.size() == 0) {
    Serial.println("No new messages at this time.");
    return;
  }

  for (JsonVariant update : result) {
    long updateId = update["update_id"];
    if (updateId <= lastUpdateId) {
      continue;
    }

    const char* messageText = update["message"]["text"];
    long chatId = update["message"]["chat"]["id"];

    if (!messageText) {
      Serial.println("Received a message without text, ignoring.");
      lastUpdateId = updateId;
      continue;
    }

    handleCommand(chatId, messageText);

    lastUpdateId = updateId;
  }
}

void handleCommand(long chatId, const char* messageText) {
  // Security Check: Only respond if chatId matches YOUR_CHAT_ID
  if (chatId != YOUR_CHAT_ID) {
    Serial.println("Unauthorized user attempted to use the bot.");
    sendMessageToTelegram(chatId, "Sorry, you are not authorized to use this bot.");
    return;
  }

  // Authorized user commands
  if (strcmp(messageText, "/start") == 0) {
    sendMessageToTelegram(chatId, "Welcome to the WOL bot!\nType /help to see available commands.");
  } else if (strcmp(messageText, "/help") == 0) {
    String helpText = "Available commands:\n";
    helpText += "/wake - Wake the PC\n";
    helpText += "/status - Check the PC status\n";
    helpText += "/help - Show this help message\n";
    sendMessageToTelegram(chatId, helpText);
  } else if (strcmp(messageText, "/wake") == 0) {
    Serial.println("'/wake' command received.");
    // Blink LED 3 times quickly before sending WOL
    for (int i = 0; i < 3; i++) {
      blinkLED(100);
      delay(100);
    }
    sendWOLPacket(targetMac);
    sendMessageToTelegram(chatId, "WOL packet sent. The PC should wake soon.");
    } else if (strcmp(messageText, "/status") == 0) {
      // Try to ping the PC
      bool isAlive = Ping.ping(pcIP, 3); // Ping 3 times
      if (isAlive) {
        sendMessageToTelegram(chatId, "PC status: Online (reachable via ping).");
      } else {
        sendMessageToTelegram(chatId, "PC status: Offline or unreachable.");
      }
    } else {
      sendMessageToTelegram(chatId, "Unknown command. Type /help for a list of commands.");
    }
}




void sendMessageToTelegram(long chatId, const String& text) {
  // Sends a text message via Telegram's sendMessage API
  // Implementing a simple sendMessage

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi, cannot send message.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();

  String messageUrl = String("/bot") + BOT_TOKEN + "/sendMessage";
  if (!client.connect(TELEGRAM_HOST, 443)) {
    Serial.println("Failed to connect to Telegram for sendMessage.");
    return;
  }

  String requestBody = "chat_id=" + String(chatId) + "&text=" + urlencode(text);

  // Send POST request
  client.println("POST " + messageUrl + " HTTP/1.1");
  client.println("Host: " + String(TELEGRAM_HOST));
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Content-Length: " + String(requestBody.length()));
  client.println("Connection: close");
  client.println();
  client.print(requestBody);

  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 5000) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      // For debugging you can print the response
    }
  }
  client.stop();
}

void sendWOLPacket(uint8_t *mac) {
  uint8_t packet[102];
  for (int i = 0; i < 6; i++) {
    packet[i] = 0xFF;
  }
  for (int i = 6; i < 102; i += 6) {
    memcpy(&packet[i], mac, 6);
  }

  IPAddress broadcastIP(255, 255, 255, 255);

  Serial.println("Sending WOL packet...");
  if (udp.beginPacket(broadcastIP, 9) == 0) {
    Serial.println("Failed to begin UDP packet for WOL.");
    return;
  }

  if (udp.write(packet, sizeof(packet)) != sizeof(packet)) {
    Serial.println("Failed to write WOL packet.");
    udp.endPacket();
    return;
  }

  if (!udp.endPacket()) {
    Serial.println("Failed to send WOL packet via UDP.");
    return;
  }

  Serial.println("WOL packet sent.");
  blinkLED(200);
}

void blinkLED(unsigned long duration) {
  digitalWrite(LED_PIN, LOW);   // LED on
  delay(duration);
  digitalWrite(LED_PIN, HIGH);  // LED off
  delay(duration);
}

// A simple URL encoder for sending text in sendMessage
String urlencode(const String &str) {
  String encoded = "";
  const char* hex = "0123456789ABCDEF";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      encoded += hex[(c >> 4) & 0xF];
      encoded += hex[c & 0xF];
    }
  }
  return encoded;
}
