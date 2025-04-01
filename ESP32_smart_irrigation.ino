#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>

// Wi-Fi
const char* ssid = "dekut";
const char* password = "dekut@ict2023";

// Firebase
const char* firebaseHost = "smart-irrigation-esp32-default-rtdb.europe-west1.firebasedatabase.app";
const char* firebaseAuth = "I0lYJbjuIVoShoUGbIBWDCyMXJWKdc2ChHDcotVb"; // Database secret

// Sensor setup
#define DHTPIN 33
#define DHTTYPE DHT11
#define MOISTURE_PIN 32
#define WATER_PIN 34
DHT dht(DHTPIN, DHTTYPE);

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  connectToWiFi();
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int moisture = analogRead(MOISTURE_PIN);
  int water = analogRead(WATER_PIN);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Skip certificate verification for simplicity

    HTTPClient https;
    
    // Full path under esp data
    String url = "https://" + String(firebaseHost) + "/esp data/sensor_data.json?auth=" + firebaseAuth;
    url.replace(" ", "%20"); // Ensure the space is properly encoded

    // Create JSON string
    String json = "{";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidity, 1) + ",";
    json += "\"moisture\":" + String(moisture) + ",";
    json += "\"water\":" + String(water);
    json += "}";

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");

    int httpCode = https.PUT(json);

    Serial.print("HTTP Response code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      Serial.println("Payload: " + json);
      Serial.println("Firebase response: " + https.getString());
    } else {
      Serial.println("Error sending data");
    }

    https.end();
  }

  delay(5000); // Delay 5 seconds between readings
}
