#define TINY_GSM_MODEM_SIM800
#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_DEBUG SerialMon
#define GSM_PIN ""  // No SIM PIN

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
#include <TinyGsmClient.h>

// GSM TX/RX pins
#define MODEM_TX 17
#define MODEM_RX 18

#define ADMIN_NUMBER "+2547xxxxxxxx"  // Replace with your phone number

TinyGsm modem(SerialAT);

// Wi-Fi credentials
const char* ssid = "dekut";
const char* password = "dekut@ict2023";

// Firebase credentials
const char* firebaseHost = "smart-irrigation-esp32-default-rtdb.europe-west1.firebasedatabase.app";
const char* firebaseAuth = "I0lYJbjuIVoShoUGbIBWDCyMXJWKdc2ChHDcotVb";

// Sensor pins
#define DHTPIN 33
#define DHTTYPE DHT11
#define MOISTURE_PIN 32
#define WATER_PIN 34

DHT dht(DHTPIN, DHTTYPE);

// Alert thresholds (The thresholds were selected for demonstration purposes.)
#define MOISTURE_THRESHOLD 5000   // 3000 for practical situations
#define WATER_THRESHOLD 1500    // 1000 for practical situations
#define TEMP_HIGH_THRESHOLD 20.0   // 35 for practical situationsin. (°C)
#define TEMP_LOW_THRESHOLD 10.0    //15 for practical situations. (°C)
#define HUMIDITY_LOW_THRESHOLD 70.0   // 40 for practical situations. (in %)

bool alertSent = false;

void connectToWiFi() {
  WiFi.begin(ssid, password);
  SerialMon.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SerialMon.print(".");
  }
  SerialMon.println("\n✅ Wi-Fi connected");
  SerialMon.print("IP Address: ");
  SerialMon.println(WiFi.localIP());
}

void setup() {
  SerialMon.begin(115200);
  delay(1000);

  dht.begin();
  connectToWiFi();

  // Initialize GSM
  SerialMon.println("Initializing GSM modem...");
  SerialAT.begin(9600, SERIAL_8N1, MODEM_TX, MODEM_RX);
  delay(3000);
  modem.restart();

  if (GSM_PIN && modem.getSimStatus() != 3) {
    modem.simUnlock(GSM_PIN);
  }

  SerialMon.print("Waiting for GSM network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success ✅");

  if (modem.isNetworkConnected()) {
    SerialMon.println("GSM network connected");
  }

  // Optional: send test SMS
  modem.sendSMS(ADMIN_NUMBER, "Hello :) ");
}

void sendSMSAlert(String message) {
  if (alertSent) return;
  SerialMon.println("📲 Sending SMS alert...");
  modem.sendSMS(ADMIN_NUMBER, message);
  SerialMon.println("✅ SMS sent");
  alertSent = true;
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int moisture = analogRead(MOISTURE_PIN);
  int waterLevel = analogRead(WATER_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    SerialMon.println("⚠️ Failed to read from DHT sensor!");
    delay(5000);
    return;
  }

  SerialMon.printf("T: %.1f°C, H: %.1f%%, Moisture: %d, Water: %d\n", temperature, humidity, moisture, waterLevel);

  // 🛑 COMBINED ALERT SECTION 🛑
  String alertMessage = "";

  if (moisture < MOISTURE_THRESHOLD) {
    alertMessage += "Soil moisture LOW: " + String(moisture) + "\n";
  }
  if (waterLevel < WATER_THRESHOLD) {
    alertMessage += "Water level LOW: " + String(waterLevel) + "\n";
  }
  if (temperature > TEMP_HIGH_THRESHOLD) {
    alertMessage += "Temperature HIGH: " + String(temperature, 1) + "°C\n";
  } else if (temperature < TEMP_LOW_THRESHOLD) {
    alertMessage += "Temperature LOW: " + String(temperature, 1) + "°C\n";
  }
  if (humidity < HUMIDITY_LOW_THRESHOLD) {
    alertMessage += "Humidity LOW: " + String(humidity, 1) + "%\n";
  }

  if (alertMessage != "") {
    sendSMSAlert("ALERT:\n" + alertMessage);
  } else {
    alertSent = false;  // Reset alert flag when all is normal
  }

  // Send data to Firebase
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Not secure for production

    HTTPClient https;
    String path = "/esp_data/sensor_data.json?auth=" + String(firebaseAuth);
    String url = "https://" + String(firebaseHost) + path;

    String jsonPayload = "{";
    jsonPayload += "\"temperature\":" + String(temperature, 1) + ",";
    jsonPayload += "\"humidity\":" + String(humidity, 1) + ",";
    jsonPayload += "\"moisture\":" + String(moisture) + ",";
    jsonPayload += "\"water\":" + String(waterLevel);
    jsonPayload += "}";

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");

    int httpCode = https.PUT(jsonPayload);

    SerialMon.print("📡 Firebase HTTP Code: ");
    SerialMon.println(httpCode);

    if (httpCode > 0) {
      SerialMon.println("✅ Data sent to Firebase");
    } else {
      SerialMon.println("❌ Firebase send error: " + https.errorToString(httpCode));
    }

    https.end();
  }

  delay(5000);
}
