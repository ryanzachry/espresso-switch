#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid       = "WiFi-SSID";
const char *password   = "SecretPSK";
const char *mqttServer = "192.168.0.3";
const char *clientId   = "espresso8266";

const int powerDetectPin = 14; // D5 on D1 mini
const int powerSwitch    = 0;  // D3 on D1 mini


WiFiClient wifiClient;
PubSubClient client(wifiClient);
unsigned long lastMsgTime = 0;



void callback(char *topic, byte *payload, unsigned int length) {
	// Shouldn't need to bother with checking if we're requested to turn on or off and the current 
	// machine status. The status is regularly updated to MQTT, only 1 topic subscribed so just hit
	// the button to toggle whatever state we're in.
	digitalWrite(powerSwitch, HIGH);
	delay(300);
	digitalWrite(powerSwitch, LOW);
}



void sendStatus() {
	// There is some periodic pulsing happening on the line we're monitoring for power status. The
	// pulses are short so two checks will confirm if it is really on.
	int firstTest  = digitalRead(powerDetectPin);
	delay(100);
	int secondTest = digitalRead(powerDetectPin);

	if (firstTest == HIGH && secondTest == HIGH) {
		Serial.println("Status: on");
		client.publish("espresso/status", "1");
	}
	else {
		Serial.println("Status: off");
		client.publish("espresso/status", "0");
	}
}



void reconnect() {
	while (!client.connected()) {
		Serial.print("MQTT connecting... ");
		if (client.connect(clientId)) {
			Serial.println("done");
			sendStatus();
			client.subscribe("espresso/set");
		} else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			// Wait and try again.
			delay(5000);
		}
	}
}



void setup() {
	pinMode(powerSwitch, OUTPUT);
	digitalWrite(powerSwitch, LOW);
	pinMode(powerDetectPin, INPUT);
	
	Serial.begin(115200);
	delay(10);

	Serial.print("Connecting to WiFi: ");
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println(" done");

	randomSeed(micros());
	client.setServer(mqttServer, 1883);
	client.setCallback(callback);
}



void loop() {
	if (!client.connected()) reconnect();
	client.loop();

	unsigned long now = millis();
	if (now - lastMsgTime > 2000) {
		lastMsgTime = now;
		sendStatus();
	}
}
