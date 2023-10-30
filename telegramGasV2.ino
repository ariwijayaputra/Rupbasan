/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/telegram-control-esp32-esp8266-nodemcu-outputs/

  Project created using Brian Lough's Universal Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
  Example based on the Universal Arduino Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot/blob/master/examples/ESP8266/FlashLED/FlashLED.ino
*/

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <elapsedMillis.h>

elapsedMillis sinceOn;
elapsedMillis alarmPaused;
// Replace with your network credentials
const char* ssid = "Gudang V";
const char* password = "rupbasansdp23";

// Initialize Telegram BOT
#define BOTtoken "5958390552:AAH7cCmRBoOrpcGfIrkdA-2jJX6k26jgnCQ"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "5700172489"

#ifdef ESP8266
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
int alarmRequestDelay = 1000;
unsigned long lastTimeAlarmOn;
int alarmOnDelay = 5000;
unsigned long lastAlarmOn;
bool canAlarmOff = true;

const int Sensor_input = 4;
const int ledPin = 2;
bool ledState = LOW;
const int relayPin = 5;
const int bluePin = 25;
const int redPin = 26;
bool relayState = LOW;
bool alarmState = false;
String telegramId[64];
int pointer = 0;

void initiateTelegramId() {
  for (int i = 0; i < 64; i++) {
    telegramId[i] = "0";
  }
}

String pushNewId(String id) {
  if (pointer < 64) {
    telegramId[pointer] = id;
    pointer++;
    bot.sendMessage(id, "berhasil subscribe. anda akan menerima notifikasi apabila alarm menyala. \nUntuk berhenti menerima notifikasi, kirim /unsubscribe.", "");
    return id;
  }
  bot.sendMessage(id, "pengguna penuh, tidak bisa subscribe.", "");
}

void printAllTelegramId() {
  for (int i = 0; i < 64; i++) {
    Serial.print(telegramId[i]);
    Serial.print(", ");
  }
  Serial.println();
}

int findIndexOf(String id) {
  for (int i = 0; i < 64; i++) {
    if (telegramId[i] == id) {
      return i;
    }
  }
  Serial.println("id tidak ditemukan.");
}

int deleteId(String id) {
  int index = findIndexOf(id);
  if (index == 63) {
    telegramId[index] = "0";
    bot.sendMessage(id, "berhasil unsubscribe", "");

    return 1;
  }
  for (int i = index; i < 63; i++) {
    telegramId[i] = telegramId[i + 1];
    bot.sendMessage(id, "berhasil unsubscribe", "");

    return 1;
  }
  return 0;
}


int readSensor() {
  int sensor_Aout = digitalRead(Sensor_input);
  if (!sensor_Aout) {
    return 1;
  } else {
    return 0;
  }
}


void IRAM_ATTR alarmOn() {
  if (readSensor() && canAlarmOff) {
    alarmState = true;
    sinceOn = 0;
    digitalWrite(relayPin, HIGH);
    digitalWrite(redPin, HIGH);
    digitalWrite(bluePin, LOW);
  } else {
    alarmState = false;
    //digitalWrite(relayPin, LOW);
  }
}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
   

    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      welcome += "Gunakan perintah dibawah untuk mengendalikan alarm.\n\n";
      welcome += "/subscribe -> untuk mendaftar sebagai penerima notifikasi saat alarm menyala\n";
      welcome += "/unsubscribe -> berhenti menerima notifikasi alarm \n";
      welcome += "/pause -> pause alarm dan sistem pendeteksi kebocoran selama 1 menit \n";
      welcome += "/resume -> resume alarm dan sistem pendeteksi \n";
      welcome += "/state -> menampilkan hasil deteksi saat ini \n\n";
      welcome += "Note: apabila terjadi pemadaman listrik atau hal lainnya yang menyebabkan alat mati, harap melakukan /subscribe kembali saat alat sudah menyala\n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/subscribe") {
      bot.sendMessage(chat_id, "Subscribing, Mohon Tunggu...", "");
      pushNewId(chat_id);
    }
    if (text == "/unsubscribe") {
      bot.sendMessage(chat_id, "Unsubscribing, Mohon tunggu...", "");
      deleteId(chat_id);
    }
    if (text == "/pause") {
      bot.sendMessage(chat_id, "Sistem tidak aktif selama 60 detik...", "");
      digitalWrite(relayPin, LOW);
      digitalWrite(redPin, LOW);
      digitalWrite(bluePin, HIGH);
      canAlarmOff=false;
      Serial.println("Paused");
      alarmPaused=0;
    }
    if (text == "/resume") {
      bot.sendMessage(chat_id, "Sistem diaktifkan", "");
      canAlarmOff=true;
      Serial.println("Resumed");
    }
    if (text == "/state") {
      if (readSensor()) {
        bot.sendMessage(chat_id, "Kebocoran gas terdeteksi", "");
      }
      else {
        bot.sendMessage(chat_id, "Tidak terdeteksi kebocoran gas", "");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

#ifdef ESP8266
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org
#endif
  pinMode(Sensor_input, INPUT);
  attachInterrupt(Sensor_input, alarmOn, CHANGE);
  pinMode(relayPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  digitalWrite(relayPin, relayState);
  digitalWrite(redPin, relayState);
  digitalWrite(bluePin, HIGH);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
#endif
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  digitalWrite(ledPin, HIGH);
  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  initiateTelegramId();
}

void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  if (alarmState) {
    Serial.println("on");
    sinceOn = 0;
    if (millis() > lastTimeAlarmOn + alarmRequestDelay)  {
      Serial.println("1s after alarm on timer");
      Serial.println("Sending notification to telegram");
      for (int i = 0; i < 64; i++) {
        if (telegramId[i] != "0") {
          bot.sendMessage(telegramId[i], "Alarm ON!", "");
          Serial.print("send to ");
          Serial.println(telegramId[i]);
        }
      }
      
      lastTimeAlarmOn = millis();
    }
  }
  if(sinceOn > 10000 && canAlarmOff){
    digitalWrite(relayPin, LOW);
    digitalWrite(redPin, LOW);
    digitalWrite(bluePin,HIGH);
  }
  if(alarmPaused > 60000){
    canAlarmOff = true;
  }
}
