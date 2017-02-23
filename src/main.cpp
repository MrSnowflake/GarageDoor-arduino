#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266TrueRandom.h>

#include <Hash.h>

#include "settings.h"

const char* VERSION = "v1.1";

ESP8266WebServer server(80);

const int TOGGLE = D1;
const int STATUS_LED = D4;

long nonce = -1;

void updateNonce() {
	nonce = ESP8266TrueRandom.random();
}

void handleRoot() {
	char temp[400];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 400,

"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>GarageDoor</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>GarageDoor</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>%s</p>\
  </body>\
</html>",

		hr, min % 60, sec % 60, VERSION
	);
	server.send(200, "text/html", temp);
}

void handleNonce() {
	char temp[400];

	updateNonce();

	snprintf(temp, 400,
		"{\"nonce\":\"%lu\"}",
		nonce
	);
	server.send(200, "application/json", temp);
}

void flicker(int count, int delayMs) {
	for (int i = 0; i < count; i++) {
		digitalWrite(STATUS_LED, LOW);
		delay(delayMs);
		digitalWrite(STATUS_LED, HIGH);
		delay(delayMs);
	}
}

void openDoor() {
	digitalWrite(TOGGLE, HIGH);
	
	flicker(10, 30);

	delay(100);

	digitalWrite(TOGGLE, LOW);
}

void handleOpen() {
	char temp[400];

	String hashHeader = server.arg("hash");
	snprintf(temp, 400, "%s%lu%s", SHA_PASSWORD, nonce, SHA_PASSWORD);

	String hash = sha1(temp);

	Serial.print("Received Hash ");
	Serial.println(hashHeader);
	Serial.print("Calculated Hash ");
	Serial.println(hash);

	// to prevent reuse
	updateNonce();

	if (hash.compareTo(hashHeader) == 0) {
		Serial.println("Opening door");
		
		server.send(200, "application/json", "{\"status\":\"done\"}");

		openDoor();
	} else {
		Serial.println("Incorrect hash");

		server.send(401, "application/json", "{\"status\":\"error\", \"message\":\"Invalid Hash\"}");

		flicker(15, 500);
	}
}

void handleNotFound() {
	String message = "File Not Found\n\n";

	server.send(404, "text/plain", message);
}

void setup() {
	pinMode(TOGGLE, OUTPUT);
	pinMode(STATUS_LED, OUTPUT);

	digitalWrite(STATUS_LED, LOW);

	Serial.begin(115200);

	updateNonce();

	WiFi.begin(SSID, PASSWORD);
	Serial.println("Connecting to Wifi");

	bool statusLedStatus = false;

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");

		digitalWrite(STATUS_LED, statusLedStatus = !statusLedStatus);
	}
	digitalWrite(STATUS_LED, LOW);
	
	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(SSID);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	if (MDNS.begin(MDNS_NAME)) {
		Serial.println("MDNS responder started");
	}

	ArduinoOTA.setPort(8266);
	// Hostname defaults to esp8266-[ChipID]
	ArduinoOTA.setHostname(MDNS_NAME);
	// No authentication by default
	ArduinoOTA.setPassword(OTA_PASSWORD);


	server.on("/", handleRoot);
	server.on("/nonce", handleNonce);
	server.on("/open", handleOpen);
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");

	ArduinoOTA.onStart([]() {
		Serial.println("Received OTA Update.");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nOTA received restarting.");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	digitalWrite(STATUS_LED, HIGH);
}

void loop() {
	server.handleClient();
	ArduinoOTA.handle();
}
