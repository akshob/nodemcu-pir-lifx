#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WifiClientSecure.h>

#include "secrets.h"
#include "certs.h"

const char* ssid     = SSIDNAME;
const char* password = SSIDPASSWORD;

int sensor = 13;  // Digital pin D7

X509List cert(cert_ISRG_Root_X1);

void setup() {
  Serial.begin(115200);

  pinMode(sensor, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));
}

void toggleLights() {
  Serial.println("Toggling Living Room Lights...");
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  Serial.print("connecting to ");
  Serial.println(lifx_host);
  if (!client.connect(lifx_host, lifx_port)) {
    Serial.println("connection failed");
    return;
  }

  String url = "/v1/lights/group:Living Room/toggle";
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("POST ") + url + "?duration=2 HTTP/1.1\r\n" +
               "Host: " + lifx_host + "\r\n" +
               "User-Agent: NodeMCU_PIR_Hack\r\n" +
               "Authorization: Bearer " + LIFXTOKEN + "\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  Serial.println("Reply was:");
  Serial.println(line);
  Serial.println("Closing connection");
}

void loop() {
  long state = digitalRead(sensor);
  digitalWrite(LED_BUILTIN, HIGH);

  if (state == HIGH) {
    Serial.println("Motion Detected!");

    toggleLights();

    Serial.println("Waiting for 6 seconds...");
    int timer = 0;
    while (timer != 6000) {
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
      delay(1000);                      // Wait for a second
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
      delay(2000);
      timer += 3000;
    }
    
    toggleLights();
  }
  else {
    Serial.println("Motion Absent");
    delay(1000);
  }
}
