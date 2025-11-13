#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ======== Cấu hình WiFi & backend ========
const char* WIFI_SSID     = "MSI-Gaming";
const char* WIFI_PASSWORD = "12345678";

const char* BACKEND_BASE_URL    = "http://192.168.1.18:3000";   // Đổi theo IP/backend thực tế
const char* LOGIN_ENDPOINT      = "/api/auth/login";
const char* TELEMETRY_ENDPOINT  = "/api/telemetry";

// ======== Tài khoản user để login ========
const char* USER_EMAIL    = "bacquandaibay@gmail.com";
const char* USER_PASSWORD = "Quan2004";

// ======== Thông tin thiết bị giả định ========
const char* USER_ID   = "U0001";   // ID đúng với user trong hệ thống
const char* DEVICE_ID = "DHW001";  // Tự quy ước ID thiết bị

String authToken;   // lưu token sau khi login thành công

// ======== Fake GPS ========
typedef struct {
  double lat;
  double lon;
} GpsPoint;

GpsPoint fakeRoute[] = {
  {21.0285, 105.8542}
};
const size_t ROUTE_SIZE = sizeof(fakeRoute) / sizeof(GpsPoint);
size_t routeIndex = 0;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL_MS = 30 * 1000UL;  // gửi 10 giây/lần

// ======== Hàm tiện ích ========
void connectWiFi() {
  Serial.printf("Connecting to WiFi %s ...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint8_t attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++attempt > 60) {  // ~30s
      Serial.println("\nWiFi connect timeout, restarting...");
      ESP.restart();
    }
  }
  Serial.printf("\nWiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

bool loginToBackend() {
  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + LOGIN_ENDPOINT;

  Serial.printf("[LOGIN] POST %s\n", url.c_str());
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["email"] = USER_EMAIL;
  doc["password"] = USER_PASSWORD;
  doc["role"] = "user";  // backend hiện tại mong nhận role, kiểm tra tùy dự án
  String body;
  serializeJson(doc, body);

  int httpCode = http.POST(body);
  if (httpCode > 0) {
    Serial.printf("[LOGIN] HTTP %d\n", httpCode);
    if (httpCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<512> res;
      DeserializationError err = deserializeJson(res, payload);
      if (!err) {
        authToken = res["token"].as<String>();
        Serial.println("[LOGIN] Token lấy được: " + authToken);
        http.end();
        return true;
      } else {
        Serial.printf("[LOGIN] JSON error: %s\n", err.c_str());
      }
    } else {
      Serial.println("[LOGIN] Response: " + http.getString());
    }
  } else {
    Serial.printf("[LOGIN] Request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return false;
}

bool postTelemetry(double lat, double lon, bool fromHardware = true) {
  if (authToken.isEmpty()) {
    Serial.println("[TELEMETRY] Chưa có token, bỏ qua");
    return false;
  }

  HTTPClient http;
  String url = String(BACKEND_BASE_URL) + TELEMETRY_ENDPOINT;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + authToken);

  StaticJsonDocument<512> doc;
  doc["userId"] = USER_ID;
  doc["deviceId"] = DEVICE_ID;
  doc["lat"] = lat;
  doc["lon"] = lon;
  doc["speed"] = 25.0;          // giả sử 25 km/h
  doc["accuracy"] = 8.5;        // độ chính xác giả
  doc["source"] = fromHardware ? "hardware" : "mobile";
  doc["timestamp"] = "2025-11-13T10:15:00.000Z";  // timestamp giả để test

  String body;
  serializeJson(doc, body);
  Serial.println("[TELEMETRY] Body: " + body);

  int httpCode = http.POST(body);
  if (httpCode > 0) {
    Serial.printf("[TELEMETRY] HTTP %d\n", httpCode);
    if (httpCode == 201 || httpCode == 200) {
      Serial.println("[TELEMETRY] Gửi thành công!");
      http.end();
      return true;
    } else {
      Serial.println("[TELEMETRY] Response: " + http.getString());
    }
  } else {
    Serial.printf("[TELEMETRY] Request failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return false;
}

// ======== setup & loop ========
void setup() {
  Serial.begin(115200);
  delay(1000);

  connectWiFi();

  uint8_t retries = 0;
  while (!loginToBackend()) {
    retries++;
    if (retries > 5) {
      Serial.println("[LOGIN] Fail >5 lần, restart...");
      ESP.restart();
    }
    delay(3000);
  }
}

void loop() {
  if (millis() - lastSend >= SEND_INTERVAL_MS) {
    lastSend = millis();

    // Fake dữ liệu GPS: lấy từ mảng route
    double lat = fakeRoute[routeIndex].lat;
    double lon = fakeRoute[routeIndex].lon;
    routeIndex = (routeIndex + 1) % ROUTE_SIZE;

    bool ok = postTelemetry(lat, lon, true);
    if (!ok) {
      Serial.println("[TELEMETRY] Gửi thất bại, thử login lại...");
      if (loginToBackend()) {
        postTelemetry(lat, lon, true);
      }
    }
  }
}

