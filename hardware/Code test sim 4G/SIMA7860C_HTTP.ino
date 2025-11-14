// Select your modem:
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <Arduino_JSON.h>
#define SerialAT Serial2

char apn[] = "m3-world";  // APN of your mobile network provider
char user[] = "";         // Usually empty
char pass[] = "";         // Usually empty
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
int rx = -1;
String rxString;
int _timeout;
String _buffer;



//// JSON ////
String jsonBuffer;
JSONVar myObject;

// Your OpenWeatherMap API settings
const char server[] = "api.openweathermap.org";
const char apiPath[] = "/data/2.5/weather?lat=21.0094962&lon=105.7898408&appid=e6fa53bea61f102d5f3b16a80d468d22";
HttpClient httpClient(client, server, 80);  // Use port 80 for HTTP

void setup() {
  Serial.begin(115200);
  SerialAT.begin(115200, SERIAL_8N1, 19, 18);  // Initialize modem with RX and TX pins
  delay(2000);

  // Modem initialization
  Serial.println("Modem Reset, Please Wait");
  SerialAT.println("AT+CRESET");
  delay(2000);
  SerialAT.flush();

  Serial.println("Echo Off");
  SerialAT.println("ATE0");
  delay(1000);
  rxString = SerialAT.readString();
  Serial.print("Got: ");
  Serial.println(rxString);

  Serial.println("SIM card check");
  SerialAT.println("AT+CPIN?");
  rxString = SerialAT.readString();
  Serial.print("Got: ");
  Serial.println(rxString);

  String name = modem.getModemName();
  delay(500);
  Serial.println("Modem Name: " + name);
  Serial.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" success");

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

  // Connecting to GPRS
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    Serial.println(" fail");
    delay(1000);
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }

  // Fetching weather data
  getWeatherData();
}

void getWeatherData() {
  Serial.println("Fetching weather data...");
  httpClient.get(apiPath);  // Send GET request

  // Wait for the response
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  // Print the response
  Serial.print("Status Code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
  jsonBuffer = response;
  // Serial.println(jsonBuffer);
  myObject = JSON.parse(jsonBuffer);
  // Check received JSON packet has data?
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println(F("Parsing input failed!"));
    return;
  }
  /* ----------------------------------------------------- */
  Serial.print("Weather: ");
  Serial.print(myObject["weather"][0]["main"]);
  Serial.print(" - ");
  Serial.println(myObject["weather"][0]["description"]);
  /* ----------------------------------------------------- */
  Serial.print("Temperature: ");
  Serial.print(myObject["main"]["temp"]);
  Serial.println("°C");
  //
  Serial.print("Humidity: ");
  Serial.print(myObject["main"]["humidity"]);
  Serial.println("%");
  //
  Serial.print("Feels Like: ");
  Serial.print(myObject["main"]["feels_like"]);
  Serial.println("°C");
  /* ----------------------------------------------------- */
  Serial.print("Wind Speed: ");
  Serial.print(myObject["wind"]["speed"]);
  Serial.println("m/s");
  //
  Serial.print("Wind Direction: ");
  Serial.print(myObject["wind"]["deg"]);
  Serial.println("°");
  /* ----------------------------------------------------- */
  Serial.print("Cloudiness: ");
  Serial.print(myObject["clouds"]["all"]);
  Serial.println("%");
  /* ----------------------------------------------------- */
  Serial.println(F(""));
  // Close the connection
  httpClient.stop();
}

void loop() {
  // Main code to run repeatedly
  getWeatherData();
  delay(10000);
}
