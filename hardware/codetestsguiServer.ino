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

 https://webhook.site/108bad31-d139-4ff5-9e49-7ea499d7f14a
 */

// Select your modem
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#define SerialMon Serial
#define SerialAT Serial2  // HardwareSerial 2 cho SIM 4G

// ============================================
// CẤU HÌNH
// ============================================

// Chân SIM 4G (UART)
#define SIM_RX_PIN  18
#define SIM_TX_PIN  17

// APN (tùy nhà mạng)
const char apn[] = "m3-world";  // Đổi theo nhà mạng (CMNET, m3-world, v.v.)
const char user[] = "";
const char pass[] = "";

// Backend server
const char server[] = "hopdenthongminh.cloud";  // Đổi theo IP backend thực tế
const int serverPort = 80;
const char telemetryEndpoint[] = "/api/telemetry";

// Thông tin thiết bị (không dùng nữa vì JSON cố định, nhưng giữ lại để tham khảo)

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
unsigned long lastCheckSOSTime = 0;  // Thời gian lần cuối kiểm tra SOS status

// Biến để thay đổi event_type và severity qua Serial Monitor
String currentEventType = "CRASH";  // Mặc định: CRASH, có thể đổi: NORMAL, CRASH, BREAKDOWN
String currentSeverity = "HIGH";     // Mặc định: HIGH, có thể đổi: LOW, MEDIUM, HIGH, CRITICAL

// Biến để lưu trạng thái SOS
bool hasActiveSOS = false;
String currentSOSId = "";
String currentSOSStatus = "";

// Interval kiểm tra SOS status từ server (mỗi 10 giây)
const unsigned long CHECK_SOS_INTERVAL_MS = 10000;

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
  printMenu();
}

void loop() {
  // Đọc lệnh từ Serial Monitor
  handleSerialCommands();
  
  // Kiểm tra SOS status từ server (để biết có bị hủy trên web không)
  if (millis() - lastCheckSOSTime >= CHECK_SOS_INTERVAL_MS) {
    lastCheckSOSTime = millis();
    checkSOSStatus();
  }
  
  // Kiểm tra và gửi dữ liệu định kỳ
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = millis();
    
    // Gửi dữ liệu lên server
    sendTelemetryToServer();
  }
  
  delay(100);
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
// GỬI TELEMETRY LÊN SERVER
// ============================================

void sendTelemetryToServer() {
  if (!sim4gNetworkOpen || !modem.isGprsConnected()) {
    SerialMon.println("[ERROR] Network not open, cannot send data");
    return;
  }
  
  SerialMon.println("\n[SEND] Preparing to send telemetry data...");
  
  // JSON sử dụng biến event_type và severity (có thể thay đổi qua Serial Monitor)
  String jsonData = "{"
    "\"device_id\":\"U0001\","
    "\"gmail\":\"bacquandaibay@gmail.com\","
    "\"pass\":\"Quan2004\","
    "\"timestamp\":\"2024-01-15 14:30:25\","
    "\"event_type\":\"" + currentEventType + "\","
    "\"severity\":\"" + currentSeverity + "\","
    "\"location\":{"
      "\"latitude\":10.123456,"
      "\"longitude\":106.654321"
    "}"
  "}";
  
  SerialMon.print("[INFO] Event Type: ");
  SerialMon.print(currentEventType);
  SerialMon.print(" | Severity: ");
  SerialMon.println(currentSeverity);
  
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

void printMenu() {
  SerialMon.println("\n========================================");
  SerialMon.println("=== MENU ĐIỀU KHIỂN QUA SERIAL ===");
  SerialMon.println("========================================");
  SerialMon.println("Lệnh để thay đổi:");
  SerialMon.println("  event:CRASH     - Đặt event_type = CRASH");
  SerialMon.println("  event:NORMAL    - Đặt event_type = NORMAL");
  SerialMon.println("  event:BREAKDOWN - Đặt event_type = BREAKDOWN");
  SerialMon.println("  severity:HIGH   - Đặt severity = HIGH");
  SerialMon.println("  severity:MEDIUM - Đặt severity = MEDIUM");
  SerialMon.println("  severity:LOW    - Đặt severity = LOW");
  SerialMon.println("  severity:CRITICAL - Đặt severity = CRITICAL");
  SerialMon.println("  checksos         - Kiểm tra SOS status ngay lập tức");
  SerialMon.println("  status           - Hiển thị trạng thái hiện tại");
  SerialMon.println("  menu             - Hiển thị menu này");
  SerialMon.println("========================================");
  SerialMon.print("Trạng thái hiện tại: ");
  SerialMon.print("event_type=");
  SerialMon.print(currentEventType);
  SerialMon.print(", severity=");
  SerialMon.println(currentSeverity);
  SerialMon.println("========================================\n");
}

void printStatus() {
  SerialMon.println("\n=== SYSTEM STATUS ===");
  SerialMon.printf("SIM Initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  SerialMon.printf("Network Open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  SerialMon.printf("GPRS Connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  SerialMon.printf("Last Send: %lu ms ago\n", millis() - lastSendTime);
  SerialMon.printf("Last SOS Check: %lu ms ago\n", millis() - lastCheckSOSTime);
  SerialMon.printf("Event Type: %s\n", currentEventType.c_str());
  SerialMon.printf("Severity: %s\n", currentSeverity.c_str());
  SerialMon.printf("Has Active SOS: %s\n", hasActiveSOS ? "YES" : "NO");
  if (hasActiveSOS) {
    SerialMon.printf("SOS ID: %s\n", currentSOSId.c_str());
    SerialMon.printf("SOS Status: %s\n", currentSOSStatus.c_str());
  }
  SerialMon.println("====================\n");
}

void checkSOSStatus() {
  if (!sim4gNetworkOpen || !modem.isGprsConnected()) {
    return; // Không kiểm tra nếu chưa kết nối
  }
  
  SerialMon.println("\n[CHECK] Kiểm tra SOS status từ server...");
  
  // Tạo HttpClient mới
  HttpClient httpClient(client, server, serverPort);
  
  // Gửi GET request đến /api/sos/check/U0001
  String endpoint = "/api/sos/check/U0001";
  httpClient.get(endpoint);
  
  // Đọc status code
  int statusCode = httpClient.responseStatusCode();
  String response = httpClient.responseBody();
  
  httpClient.stop();
  
  if (statusCode == 200) {
    // Parse JSON response (đơn giản, không dùng thư viện JSON)
    if (response.indexOf("\"hasActiveSOS\":true") >= 0) {
      // Có SOS active
      hasActiveSOS = true;
      
      // Lấy SOS ID
      int sosIdStart = response.indexOf("\"sosId\":\"") + 9;
      int sosIdEnd = response.indexOf("\"", sosIdStart);
      if (sosIdStart > 8 && sosIdEnd > sosIdStart) {
        currentSOSId = response.substring(sosIdStart, sosIdEnd);
      }
      
      // Lấy status
      int statusStart = response.indexOf("\"status\":\"") + 10;
      int statusEnd = response.indexOf("\"", statusStart);
      if (statusStart > 9 && statusEnd > statusStart) {
        currentSOSStatus = response.substring(statusStart, statusEnd);
      }
      
      SerialMon.print("[SOS] ✅ Có SOS active: ");
      SerialMon.print(currentSOSId);
      SerialMon.print(" (status: ");
      SerialMon.print(currentSOSStatus);
      SerialMon.println(")");
    } else {
      // Không có SOS active
      if (hasActiveSOS) {
        SerialMon.println("[SOS] ⚠️ SOS đã bị hủy trên web! Dừng cảnh báo...");
        SerialMon.println("[SOS] 🔄 Tự động đổi event_type về NORMAL...");
        
        // Tự động đổi event_type về NORMAL khi SOS bị hủy
        currentEventType = "NORMAL";
        hasActiveSOS = false;
        currentSOSId = "";
        currentSOSStatus = "";
        
        SerialMon.print("[SOS] ✅ Đã đổi event_type = ");
        SerialMon.println(currentEventType);
        SerialMon.println("[SOS] 💡 Lần gửi tiếp theo sẽ gửi NORMAL lên server");
        // Ở đây có thể thêm code để tắt buzzer, OLED, v.v.
      } else {
        hasActiveSOS = false;
        SerialMon.println("[SOS] ℹ️ Không có SOS active");
      }
    }
  } else {
    SerialMon.print("[SOS] ❌ Lỗi kiểm tra SOS: ");
    SerialMon.println(statusCode);
  }
}

void handleSerialCommands() {
  if (SerialMon.available() > 0) {
    String command = SerialMon.readStringUntil('\n');
    command.trim();
    command.toUpperCase();
    
    if (command.length() == 0) {
      return;
    }
    
    SerialMon.print("[CMD] Nhận lệnh: ");
    SerialMon.println(command);
    
    // Xử lý lệnh thay đổi event_type
    if (command.startsWith("EVENT:")) {
      String newEventType = command.substring(6);
      newEventType.trim();
      
      if (newEventType == "CRASH" || newEventType == "NORMAL" || newEventType == "BREAKDOWN") {
        currentEventType = newEventType;
        SerialMon.print("[OK] Đã đặt event_type = ");
        SerialMon.println(currentEventType);
      } else {
        SerialMon.println("[ERROR] Event type không hợp lệ! Chọn: CRASH, NORMAL, BREAKDOWN");
      }
    }
    // Xử lý lệnh thay đổi severity
    else if (command.startsWith("SEVERITY:")) {
      String newSeverity = command.substring(9);
      newSeverity.trim();
      
      if (newSeverity == "LOW" || newSeverity == "MEDIUM" || newSeverity == "HIGH" || newSeverity == "CRITICAL") {
        currentSeverity = newSeverity;
        SerialMon.print("[OK] Đã đặt severity = ");
        SerialMon.println(currentSeverity);
      } else {
        SerialMon.println("[ERROR] Severity không hợp lệ! Chọn: LOW, MEDIUM, HIGH, CRITICAL");
      }
    }
    // Kiểm tra SOS ngay lập tức
    else if (command == "CHECKSOS") {
      checkSOSStatus();
    }
    // Hiển thị trạng thái
    else if (command == "STATUS") {
      printStatus();
    }
    // Hiển thị menu
    else if (command == "MENU") {
      printMenu();
    }
    // Lệnh không hợp lệ
    else {
      SerialMon.println("[ERROR] Lệnh không hợp lệ! Gõ 'menu' để xem hướng dẫn");
    }
    
    SerialMon.print("[INFO] Trạng thái hiện tại: event_type=");
    SerialMon.print(currentEventType);
    SerialMon.print(", severity=");
    SerialMon.print(currentSeverity);
    SerialMon.print(", hasActiveSOS=");
    SerialMon.println(hasActiveSOS ? "YES" : "NO");
  }
}
