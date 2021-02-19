#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

#define HTTP_REST_PORT 80
#define WIFI_RETRY_DELAY 500
#define MAX_WIFI_INIT_RETRY 50

#define WIFI_SSID "Sales"
#define WIFI_PASS "57545311"
 
#define AP_SSID "ESP8266"
#define AP_PASS "automagico"


struct Led {
    byte id;
    byte gpio;
    byte state;
} led_resource;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

void init_led_resource()
{
    led_resource.id = 0;
    led_resource.gpio = 0;
    led_resource.state = 0;
}


void network(){
	// Begin Access Point
	WiFi.softAP(AP_SSID, AP_PASS);

	// Begin WiFi
	WiFi.begin(WIFI_SSID, WIFI_PASS);

	// Connecting to WiFi...
	Serial.print("Connecting to ");
	Serial.print(WIFI_SSID);
	
	// Loop continuously while WiFi is not connected
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(100);
		Serial.print(".");
	}

	// Connected to WiFi
	Serial.println();
	Serial.println("Connected!");
	Serial.print("IP address for network ");
	Serial.print(WIFI_SSID);
	Serial.print(" : ");
	Serial.println(WiFi.localIP());
	Serial.print("IP address for network ");
	Serial.print(AP_SSID);
	Serial.print(" : ");
	Serial.print(WiFi.softAPIP());
	Serial.println();  
}


void get_leds() {
	
	DynamicJsonDocument doc(512);

	doc["id"] = led_resource.id;
	doc["gpio"] = led_resource.gpio;
	doc["state"] = led_resource.state;
	
	String buf;
	serializeJson(doc, buf);
	http_rest_server.send(200, F("application/json"), buf);
	Serial.println(F("done."));
}


void json_to_resource(JsonObject docJson) {
	
    int id, gpio, state;

    id = docJson["id"];
    gpio = docJson["gpio"];
    state = docJson["state"];

    Serial.println(id);
    Serial.println(gpio);
    Serial.println(state);

    led_resource.id = id;
    led_resource.gpio = gpio;
    led_resource.state = state;
}


void post_put_leds() {
  
	String input = http_rest_server.arg("plain");
	Serial.println(input);
	
	DynamicJsonDocument doc(512);
	DeserializationError error = deserializeJson(doc, input);

	if (error) 
	{
		Serial.print(F("deserializeJson() failed: "));
		Serial.println(error.f_str());
		return;
	}

	JsonObject jsonBody = doc.as<JsonObject>();

	Serial.print("HTTP Method: ");
	Serial.println(http_rest_server.method());

	if (http_rest_server.method() == HTTP_POST) 
	{
		if ((jsonBody["id"] != 0) && (jsonBody["id"] != led_resource.id)) 
		{
			json_to_resource(jsonBody);
			
			http_rest_server.sendHeader("Location", "/leds/" + String(led_resource.id));
			http_rest_server.send(201, F("application/json"), input);
			
			pinMode(led_resource.gpio, OUTPUT);	
			
		}

		else if (jsonBody["id"] == 0)
			http_rest_server.send(404);

		else if (jsonBody["id"] == led_resource.id)
			http_rest_server.send(409);
	}


	else if (http_rest_server.method() == HTTP_PUT) 
	{
		if (jsonBody["id"] == led_resource.id) 
		{
			json_to_resource(jsonBody);
			http_rest_server.sendHeader("Location", "/leds/" + String(led_resource.id));
			http_rest_server.send(200, F("application/json"), input);
			
			pinMode(led_resource.gpio, OUTPUT);	
			digitalWrite(led_resource.gpio, led_resource.state);		
		}
			else
			http_rest_server.send(404);
	}
}


void config_rest_server_routing() {
    http_rest_server.on("/", HTTP_GET, []() {
        http_rest_server.send(200, "text/html",
            "<h1>Welcome to the ESP8266 REST Web Server</h1>");
    });
    http_rest_server.on("/leds", HTTP_GET, get_leds);
    http_rest_server.on("/leds", HTTP_POST, post_put_leds);
    http_rest_server.on("/leds", HTTP_PUT, post_put_leds);
}


void setup() {
    Serial.begin(115200);

    init_led_resource();
   
	network();

    config_rest_server_routing();

    http_rest_server.begin();
    Serial.println("HTTP REST Server Started");
}


void loop() {
    http_rest_server.handleClient();
}
