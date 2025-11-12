//#include <WiFi.h>
//#include <PubSubClient.h>
#include <Arduino.h>

// === CONFIGURA QUI ===
const char* WIFI_SSID     = "iPhone di Martina";
const char* WIFI_PASS     = "12345678";

// Adafruit IO (MQTT)
const char* AIO_SERVER    = "io.adafruit.com";
const int   AIO_PORT      = 1883;          // 8883 per SSL (richiede WiFiClientSecure)
const char* AIO_USERNAME  = "jacopogambi";


// === PIN ===
#define LED_VERDE 25
#define LED_ROSSO 26
#define PULSANTE  14

WiFiClient espClient;
PubSubClient mqtt(espClient);

bool occupato = false;           // false=LIBERO (verde), true=OCCUPATO (rosso)
unsigned long lastDebounce = 0;
const unsigned long debounceMs = 200;

void setLedsByState() {
  digitalWrite(LED_VERDE, occupato ? LOW : HIGH);
  digitalWrite(LED_ROSSO, occupato ? HIGH : LOW);
}

void publishState(bool retain = true) {
  const char* msg = occupato ? "OCCUPATO" : "LIBERO";
  mqtt.publish(AIO_FEED, msg, retain);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
  }
}

void connectMQTT() {
  mqtt.setServer(AIO_SERVER, AIO_PORT);
  while (!mqtt.connected()) {
    String clientId = "ESP32-ombrellone-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(clientId.c_str(), AIO_USERNAME, AIO_KEY)) {
      // appena connesso pubblichiamo lo stato corrente
      publishState(true);
    } else {
      delay(1000);
    }
  }
}

void setup() {
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROSSO, OUTPUT);
  pinMode(PULSANTE, INPUT_PULLUP);

  // Stato iniziale: LIBERO (verde acceso)
  occupato = false;
  setLedsByState();

  connectWiFi();
  connectMQTT();
}

void loop() {
  // mantieni connessioni
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  // lettura pulsante (attivo basso)
  int reading = digitalRead(PULSANTE);
  if (reading == LOW && (millis() - lastDebounce) > debounceMs) {
    lastDebounce = millis();

    // toggle dello stato
    occupato = !occupato;
    setLedsByState();
    publishState(true); // aggiorna dashboard

    // attesa rilascio (evita ripetizioni)
    while (digitalRead(PULSANTE) == LOW) {
      mqtt.loop();
      delay(10);
    }
  }
}
