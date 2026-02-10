
//================ MODULO CANCELLO - GGNO-IOT =================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HardwareSerial.h>
#include "time.h"

//---------------- WIFI ----------------
const char* ssid = "Tua wifi";
const char* password = "TuanPassword";

//---------------- SUPABASE ----------------
//---------------- SUPABASE ----------------
#define SUPABASE_URL  "https://TUO_PROGETTO.supabase.co"
#define SUPABASE_ANON_KEY "TUO_ANON_KEY"



// ----------- TELEGRAM -----------
#define BOT_TOKEN "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define CHAT_ID "xxxxxxxxxxxxxxxxxxxxxxxxxxx"

WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);

//---------------- PIN ----------------
#define GATE_RELAY_PIN 27

//---------------- SIM800L ----------------
HardwareSerial sim800(1);
String simBuffer = "";

//---------------- TELEGRAM TIMING ----------------
unsigned long lastTelegramCheck = 0;
const unsigned long TELEGRAM_INTERVAL = 2000;

//---------------- STATO CANCELLO ----------------
unsigned long relayOnTime = 0;
bool relayActive = false;

unsigned long lastOpen = 0;
const unsigned long OPEN_COOLDOWN = 10000;

String ultimoNome = "N/D";
String ultimoQuando = "N/D";

//---------------- UTENTI AUTORIZZATI ----------------
struct Utente {
  const char* numero;
  const char* nome;
};

Utente utentiAutorizzati[] = {
  {"+39xxxxxxxxxxxxxxxxxx", "aaaaaaaaaaaaaaaa"},
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

String nowString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "ora non disponibile";

  char buf[25];
  strftime(buf, sizeof(buf), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buf);
}

void apriCancello(const String& nome) {
  if (millis() - lastOpen < OPEN_COOLDOWN) return;
  lastOpen = millis();

  digitalWrite(GATE_RELAY_PIN, HIGH);
  relayOnTime = millis();
  relayActive = true;

  ultimoNome = nome;
  ultimoQuando = nowString();

  if (WiFi.status() == WL_CONNECTED) {
    String msg = "🚪 Cancello aperto\n👤 " + nome + "\n🕒 " + ultimoQuando;
    bot.sendMessage(CHAT_ID, msg, "");
  }
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

void gestisciTelegram() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastTelegramCheck < TELEGRAM_INTERVAL) return;
  lastTelegramCheck = millis();

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = bot.messages[i].chat_id;
      String text = bot.messages[i].text;

      //if (chat_id != CHAT_ID) continue;

      if (text == "/apri") {
        apriCancello("Telegram");
      }
      else if (text == "/stato") {
        String msg = "📡 Stato modulo\n";
        msg += WiFi.status() == WL_CONNECTED ? "✅ WiFi OK\n" : "❌ WiFi OFF\n";
        msg += "📶 GSM attivo";
        bot.sendMessage(CHAT_ID, msg, "");
      }
      else if (text == "/ultimo") {
        String msg = "🧾 Ultimo accesso\n";
        msg += "👤 " + ultimoNome + "\n";
        msg += "🕒 " + ultimoQuando;
        bot.sendMessage(CHAT_ID, msg, "");
      }
    }
    numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  }
}

//---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  pinMode(GATE_RELAY_PIN, OUTPUT);
  digitalWrite(GATE_RELAY_PIN, LOW);

  WiFi.begin(ssid, password);
  client.setInsecure();

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connesso");
    configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
  } else {
    Serial.println("WiFi NON disponibile, GSM attivo");
  }

  sim800.begin(9600, SERIAL_8N1, 34, 17);
  delay(500);
  sim800.println("ATE0");
  sim800.println("AT+CLIP=1");

  Serial.println("Modulo cancello pronto");
}

//---------------- LOOP ----------------
void loop() {
  gestisciSIM800();
  gestisciTelegram();

  if (relayActive && millis() - relayOnTime > 1000) {
    digitalWrite(GATE_RELAY_PIN, LOW);
    relayActive = false;
  }
}
