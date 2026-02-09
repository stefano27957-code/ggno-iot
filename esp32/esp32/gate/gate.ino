//================ MODULO CANCELLO - GGNO-IOT =================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HardwareSerial.h>
#include "time.h"

//---------------- WIFI ----------------
const char* ssid = "TUA_WIFI";
const char* password = "TUA_PASSWORD";

//---------------- TELEGRAM ----------------
#define BOT_TOKEN "TUO_BOT_TOKEN"
#define CHAT_ID   "TUO_CHAT_ID"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

//---------------- PIN ----------------
#define GATE_RELAY_PIN 27

//---------------- SIM800L ----------------
HardwareSerial sim800(1);
String simBuffer = "";

//---------------- UTENTI AUTORIZZATI ----------------
struct Utente {
  const char* numero;
  const char* nome;
};

Utente utentiAutorizzati[] = {
  {"+39XXXXXXXXXX", "Stefano"},
};

int numAutorizzati = sizeof(utentiAutorizzati) / sizeof(utentiAutorizzati[0]);

//---------------- FUNZIONI ----------------
String nomeDaCLIP(const String& clipLine) {
  for (int i = 0; i < numAutorizzati; i++) {
    if (clipLine.indexOf(utentiAutorizzati[i].numero) >= 0) {
      return String(utentiAutorizzati[i].nome);
    }
  }
  return "";
}

void apriCancello(const String& nome) {
  digitalWrite(GATE_RELAY_PIN, HIGH);
  delay(1000);
  digitalWrite(GATE_RELAY_PIN, LOW);

  String msg = "🚪 Cancello aperto\n👤 " + nome;
  bot.sendMessage(CHAT_ID, msg, "");
}

void gestisciSIM800() {
  while (sim800.available()) {
    char c = sim800.read();
    if (c == '\n') {
      simBuffer.trim();

      if (simBuffer.startsWith("+CLIP")) {
        String nome = nomeDaCLIP(simBuffer);
        if (nome != "") {
          apriCancello(nome);
        }
        sim800.println("ATH");
      }
      simBuffer = "";
    } else if (c != '\r') {
      simBuffer += c;
    }
  }
}

//---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  pinMode(GATE_RELAY_PIN, OUTPUT);
  digitalWrite(GATE_RELAY_PIN, LOW);

  WiFi.begin(ssid, password);
  client.setInsecure();
  while (WiFi.status() != WL_CONNECTED) delay(500);

  sim800.begin(9600, SERIAL_8N1, 34, 17);
  delay(500);
  sim800.println("ATE0");
  sim800.println("AT+CLIP=1");

  Serial.println("Modulo cancello pronto");
}

//---------------- LOOP ----------------
void loop() {
  gestisciSIM800();
}
