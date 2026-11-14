// Library penting untuk WiFi, HTTP request, format JSON & sensor DHT
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Masukkan nama WiFi (hotspot) & password kat sini
const char* ssid = "vivo Y33s";
const char* password = "1122334455";

// Link Web App yang dapat lepas deploy Google Apps Script
const char* scriptURL = "https://script.google.com/macros/s/AKfycbwt-Fk91vD7B9oIFYKr18zfjbY80TAF8vQcztwtK442YHUmCFWkNZifl6nQ3i0iVVSrzA/exec";

// Setup pin & jenis sensor DHT yang dipakai
#define DHTPIN 14       // Pin data sensor sambung ke GPIO 14
#define DHTTYPE DHT11   // Guna sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

// Set selang masa untuk hantar data (dalam ms)
// Nota: 100000 ms = ~1 minit 40 saat. Tukar ke 600000 kalau nak tepat 10 minit
const unsigned long sendInterval = 100000;
unsigned long previousMillis = 0;

void setup() {
  // Hidupkan Serial Monitor untuk tengok output / debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 DHT11 Sensor Data Logger");

  // Start sensor DHT11
  dht.begin();

  // Mula sambung ke WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  // Tunggu sampai WiFi betul-betul connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Dah connect, print IP address ESP32
  Serial.println();
  Serial.print("Connected to WiFi with IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long currentMillis = millis();

  // Guna millis() supaya code tak freeze macam guna delay()
  // Check kalau dah cukup masa untuk hantar data seterusnya
  if (currentMillis - previousMillis >= sendInterval) {
    previousMillis = currentMillis; // Reset timer

    // Ambil bacaan kelembapan & suhu dari sensor
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Kalau sensor gagal baca (keluar NaN), skip & cuba lagi next loop
    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Print bacaan kat Serial Monitor untuk semakan
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    // Panggil function untuk hantar data ke Google Sheets
    sendDataToGoogleSheets(temperature, humidity);
  }
}

void sendDataToGoogleSheets(float temperature, float humidity) {
  // Double check WiFi masih connected ke tak sebelum hantar
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return;
  }

  // Setup sambungan HTTP ke Google Script
  HTTPClient http;
  http.begin(scriptURL);
  http.addHeader("Content-Type", "application/json");

  // Bungkus data suhu & kelembapan dalam format JSON
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  // Convert objek JSON tadi jadi String supaya boleh dihantar
  String jsonString;
  serializeJson(doc, jsonString);

  // Hantar data guna HTTP POST & tangkap response code dari server
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    // Kalau berjaya hantar, print respon dari Google Script
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
  } else {
    // Kalau gagal, print kod error untuk senang troubleshoot
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  // Tutup sambungan HTTP lepas siap
  http.end();
}