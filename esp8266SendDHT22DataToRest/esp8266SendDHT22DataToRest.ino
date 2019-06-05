#include <DHT.h>
#include <ESP8266WiFi.h>

#define DHTVIN 0
#define DHTPIN 2
#define DHTTYPE DHT22

ADC_MODE(ADC_VCC);

const char* ssid = "";
const char* password = "";
const char* host = "";

const int deviceId = 1;
const int httpPort = 8080;
const int sleepTimeMin = 15;

DHT dht(DHTPIN, DHTTYPE);

IPAddress ip;
unsigned long timerStart;

void setup() {
  timerStart = millis();
  pinMode(DHTVIN, OUTPUT);
  digitalWrite(DHTVIN, LOW);
  Serial.begin(115200);
  dht.begin();
  connectWiFi();
}

void loop() {
  float vccVoltage = ((float)ESP.getVcc()) / 1024;
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read DHT sensor!");
    return;
  }

  postValues(temperature, humidity, vccVoltage);
  Serial.print("Running time: ");
  Serial.println(millis() - timerStart);
  ESP.deepSleep(sleepTimeMin * 60 * 1000000, WAKE_RF_DEFAULT);
  delay(100);
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  unsigned long timeout = millis();
  Serial.println();
  Serial.print("Waiting to connect");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    if ((millis() - timeout) > 10000) {
      Serial.print("Connection failed going to sleep.");
      ESP.deepSleep(sleepTimeMin * 60 * 1000000, WAKE_RF_DEFAULT);
    }
  }
  
  Serial.println();
  Serial.print("IP address: ");
  ip = WiFi.localIP();
  Serial.println(ip);
}

void postValues(float temp, float hum, float voltage) {

  Serial.print("connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  String url = "/api/insert/dht?";
  url += "device_id=";
  url += deviceId;
  url += "&";
  url += "temperature=";
  url += temp;
  url += "&";
  url += "humidity=";
  url += hum;
  url += "&";
  url += "voltage=";
  url += voltage;
  url += "&";
  url += "awake_time=";
  url += millis() - timerStart;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;

    }
  }

  Serial.println();
  Serial.println("closing connection");
}

