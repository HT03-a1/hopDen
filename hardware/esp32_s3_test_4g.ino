/*
 * ESP32-S3 TEST SIM 4G - Gửi dữ liệu random lên server
 * 
 * Mục đích: Test kết nối SIM 4G và gửi dữ liệu telemetry random lên backend
 * Sử dụng TinyGSM library (giống code chính)
 * 
 * Chức năng:
 * 1. Khởi tạo SIM 4G module với TinyGSM
 * 2. Kết nối mạng GPRS/4G
 * 3. Gửi dữ liệu GPS random lên server mỗi 30 giây
 * 4. Hiển thị trạng thái trên Serial Monitor
 */

// Select your modem
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

#define SerialMon Serial
#define SerialAT Serial2  // HardwareSerial 2 cho SIM 4G

// ============================================
// CẤU HÌNH
// ============================================

// Chân SIM 4G (UART)
#define SIM_RX_PIN  18
#define SIM_TX_PIN  19

// APN (tùy nhà mạng)
const char apn[] = "m3-world";  // Đổi theo nhà mạng (CMNET, m3-world, v.v.)
const char user[] = "";
const char pass[] = "";

// Backend server
const char server[] = "192.168.1.18";  // Đổi theo IP backend thực tế
const int serverPort = 3000;
const char telemetryEndpoint[] = "/api/telemetry";

// Thông tin thiết bị
const char USER_ID[] = "U0001";      // ID user trong hệ thống
const char DEVICE_ID[] = "DHW001";   // ID thiết bị

// Interval gửi dữ liệu
const unsigned long SEND_INTERVAL_MS = 30000;  // 30 giây

// ============================================
// BIẾN TOÀN CỤC
// ============================================

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient httpClient(client, server, serverPort);

bool sim4gInitialized = false;
bool sim4gNetworkOpen = false;
unsigned long lastSendTime = 0;

// Random GPS data (quanh Hà Nội)
struct GPSData {
  double lat;
  double lon;
  float speed;
};

GPSData currentGPS = {21.0285, 105.8542, 0.0};  // Vị trí mặc định

// ============================================
// SETUP
// ============================================

void setup() {
  SerialMon.begin(115200);
  delay(1000);
  
  SerialMon.println("\n\n=== ESP32-S3 TEST SIM 4G ===");
  SerialMon.println("Initializing...");
  
  // Khởi tạo Serial cho SIM 4G
  SerialAT.begin(115200, SERIAL_8N1, SIM_RX_PIN, SIM_TX_PIN);
  delay(2000);
  
  // Reset modem
  SerialMon.println("Modem Reset, Please Wait");
  SerialAT.println("AT+CRESET");
  delay(2000);
  SerialAT.flush();
  
  // Echo off
  SerialMon.println("Echo Off");
  SerialAT.println("ATE0");
  delay(1000);
  String rxString = SerialAT.readString();
  SerialMon.print("Got: ");
  SerialMon.println(rxString);
  
  // Khởi tạo SIM 4G module
  initSIMModule();
  
  SerialMon.println("\nSystem ready! Starting to send data...");
}

void loop() {
  // Kiểm tra và gửi dữ liệu định kỳ
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = millis();
    
    // Tạo dữ liệu GPS random
    generateRandomGPS();
    
    // Gửi dữ liệu lên server
    sendTelemetryToServer();
  }
  
  delay(1000);
}

// ============================================
// KHỞI TẠO SIM 4G
// ============================================

void initSIMModule() {
  SerialMon.println("\n[INIT] Initializing SIM 4G module with TinyGSM...");
  
  // Kiểm tra SIM card
  SerialMon.println("[INFO] SIM card check");
  SerialAT.println("AT+CPIN?");
  delay(1000);
  String rxString = SerialAT.readString();
  SerialMon.print("Got: ");
  SerialMon.println(rxString);
  
  // Lấy tên modem
  String name = modem.getModemName();
  delay(500);
  SerialMon.println("Modem Name: " + name);
  
  // Chờ mạng
  SerialMon.print("[INFO] Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(1000);
    return;
  }
  SerialMon.println(" success");
  
  if (modem.isNetworkConnected()) {
    SerialMon.println("[OK] Network connected");
  }
  
  // Kết nối GPRS
  SerialMon.print(F("[INFO] Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" fail");
    delay(1000);
    return;
  }
  SerialMon.println(" success");
  
  if (modem.isGprsConnected()) {
    SerialMon.println("[OK] GPRS connected");
    sim4gNetworkOpen = true;
    sim4gInitialized = true;
  } else {
    SerialMon.println("[ERROR] GPRS connection failed");
  }
}

// ============================================
// TẠO DỮ LIỆU GPS RANDOM
// ============================================

void generateRandomGPS() {
  // Tạo dữ liệu GPS random quanh Hà Nội
  // Lat: 20.8 - 21.3, Lon: 105.6 - 106.0
  currentGPS.lat = 21.0285 + (random(-2000, 2000) / 10000.0);  // ±0.2 độ
  currentGPS.lon = 105.8542 + (random(-2000, 2000) / 10000.0);  // ±0.2 độ
  currentGPS.speed = random(0, 80);  // 0-80 km/h
  
  SerialMon.printf("[GPS] Generated: Lat=%.6f, Lon=%.6f, Speed=%.1f km/h\n",
                   currentGPS.lat, currentGPS.lon, currentGPS.speed);
}

// ============================================
// GỬI TELEMETRY LÊN SERVER
// ============================================

void sendTelemetryToServer() {
  if (!sim4gNetworkOpen || !modem.isGprsConnected()) {
    SerialMon.println("[ERROR] Network not open, cannot send data");
    return;
  }
  
  SerialMon.println("\n[SEND] Preparing to send telemetry data...");
  
  // Tạo JSON data
  StaticJsonDocument<512> doc;
  doc["userId"] = USER_ID;
  doc["deviceId"] = DEVICE_ID;
  doc["lat"] = currentGPS.lat;
  doc["lon"] = currentGPS.lon;
  doc["speed"] = currentGPS.speed;
  doc["source"] = "hardware";
  
  // Timestamp
  unsigned long now = millis() / 1000;
  int hours = (now / 3600) % 24;
  int minutes = (now / 60) % 60;
  int seconds = now % 60;
  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "2025-01-13T%02d:%02d:%02d.000Z", hours, minutes, seconds);
  doc["timestamp"] = timestamp;
  
  String jsonData;
  serializeJson(doc, jsonData);
  
  SerialMon.print("[JSON] ");
  SerialMon.println(jsonData);
  
  // Gửi HTTP POST
  SerialMon.printf("[HTTP] Connecting to %s:%d%s\n", server, serverPort, telemetryEndpoint);
  
  // Tạo HttpClient mới cho mỗi request
  HttpClient httpClient(client, server, serverPort);
  
  // Gửi POST request
  httpClient.post(telemetryEndpoint, "application/json", jsonData);
  
  // Đọc status code
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  
  SerialMon.print("[HTTP] Status Code: ");
  SerialMon.println(statusCode);
  SerialMon.print("[HTTP] Response: ");
  SerialMon.println(response);
  
  // Đóng connection
  httpClient.stop();
  
  if (statusCode == 200 || statusCode == 201) {
    SerialMon.println("[SUCCESS] Data sent successfully!");
  } else {
    SerialMon.println("[WARNING] Unexpected response");
  }
  
  SerialMon.println();
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

void printStatus() {
  SerialMon.println("\n=== SYSTEM STATUS ===");
  SerialMon.printf("SIM Initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  SerialMon.printf("Network Open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  SerialMon.printf("GPRS Connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  SerialMon.printf("Last Send: %lu ms ago\n", millis() - lastSendTime);
  SerialMon.printf("Current GPS: Lat=%.6f, Lon=%.6f, Speed=%.1f\n",
                   currentGPS.lat, currentGPS.lon, currentGPS.speed);
  SerialMon.println("====================\n");
}
