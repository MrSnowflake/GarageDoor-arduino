#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "settings.h"

ESP8266WebServer server(80);

const int TOGGLE = D1;
const int STATUS_LED = D4;

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
  </body>\
</html>",

		hr, min % 60, sec % 60
	);
	server.send(200, "text/html", temp);
}

void handleNotFound() {
	String message = "File Not Found\n\n";

	server.send(404, "text/plain", message);
}

void setup() {
	pinMode(TOGGLE, OUTPUT);
	pinMode(STATUS_LED, OUTPUT);

	Serial.begin(115200);

	WiFi.begin(SSID, PASSWORD);
	Serial.println("Connecting to Wifif");

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

	if (MDNS.begin("esp8266")) {
		Serial.println("MDNS responder started");
	}

	server.on("/", handleRoot);
	server.on("/activate", [](){
		digitalWrite(TOGGLE, HIGH);
		delay(500);
		digitalWrite(TOGGLE, LOW);
		
		server.send(200, "application/json", "{\"status\":\"done\"}");
	});
	//server.on("/test.svg", drawGraph);
	/*server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});*/
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
}

void loop() {
	server.handleClient();
}
