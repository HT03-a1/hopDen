/*
 * ESP32-S3 TEST SMS & CALL - Gửi SMS và gọi điện qua SIM 4G
 * 
 * Mục đích: Test chức năng SMS và gọi điện qua SIM 4G module
 * Sử dụng TinyGSM library
 * 
 * Chức năng:
 * 1. Khởi tạo SIM 4G module với TinyGSM
 * 2. Kết nối mạng GPRS/4G
 * 3. Đọc lệnh từ Serial Monitor để gửi SMS hoặc gọi điện
 * 4. Hiển thị trạng thái trên Serial Monitor
 * 
 * Lệnh Serial:
 * - Nhập "SMS" hoặc "sms" để gửi SMS
 * - Nhập "CALL" hoặc "call" để gọi điện
 * - Nhập "STATUS" hoặc "status" để xem trạng thái
 */

// Select your modem
#define TINY_GSM_MODEM_SIM7600
#include <TinyGsmClient.h>

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

// Số điện thoại nhận SMS và gọi điện
// Format: có thể dùng số trong nước (0798169921) hoặc quốc tế (+84798169921)
const char PHONE_NUMBER[] = "0798169921";
const char PHONE_NUMBER_INTL[] = "+84798169921";  // Format quốc tế (thử nếu số trong nước không được)

// ============================================
// BIẾN TOÀN CỤC
// ============================================

TinyGsm modem(SerialAT);

bool sim4gInitialized = false;
bool sim4gNetworkOpen = false;
String serialInput = "";

// ============================================
// SETUP
// ============================================

void setup() {
  SerialMon.begin(115200);
  delay(1000);
  
  SerialMon.println("\n\n=== ESP32-S3 TEST SMS & CALL ===");
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
  
  SerialMon.println("\n=== SYSTEM READY ===");
  SerialMon.println("Commands:");
  SerialMon.println("  - Type 'SMS' or 'sms' to send SMS");
  SerialMon.println("  - Type 'CALL' or 'call' to make a call");
  SerialMon.println("  - Type 'HANGUP' or 'hangup' to end a call");
  SerialMon.println("  - Type 'STATUS' or 'status' to check status");
  SerialMon.println("========================\n");
}

void loop() {
  // Đọc lệnh từ Serial Monitor
  if (SerialMon.available()) {
    char c = SerialMon.read();
    
    if (c == '\n' || c == '\r') {
      if (serialInput.length() > 0) {
        processCommand(serialInput);
        serialInput = "";
      }
    } else {
      serialInput += c;
    }
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
  
  // Kết nối GPRS (cần cho một số chức năng)
  SerialMon.print(F("[INFO] Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" fail");
    delay(1000);
    // Vẫn tiếp tục dù GPRS fail (SMS và Call không cần GPRS)
  } else {
    SerialMon.println(" success");
    if (modem.isGprsConnected()) {
      SerialMon.println("[OK] GPRS connected");
      sim4gNetworkOpen = true;
    }
  }
  
  // Cấu hình SMS text mode
  SerialMon.println("[INFO] Configuring SMS text mode...");
  SerialAT.println("AT+CMGF=1");
  delay(1000);
  rxString = SerialAT.readString();
  SerialMon.print("Got: ");
  SerialMon.println(rxString);
  
  // Kiểm tra SMS center number (có thể cần cấu hình)
  SerialMon.println("[INFO] Checking SMS center number...");
  SerialAT.println("AT+CSCA?");
  delay(1000);
  rxString = SerialAT.readString();
  SerialMon.print("SMS Center: ");
  SerialMon.println(rxString);
  
  // Kiểm tra tín hiệu mạng
  SerialMon.println("[INFO] Checking signal strength...");
  SerialAT.println("AT+CSQ");
  delay(1000);
  rxString = SerialAT.readString();
  SerialMon.print("Signal: ");
  SerialMon.println(rxString);
  
  sim4gInitialized = true;
  SerialMon.println("[OK] SIM 4G initialized successfully");
}

// ============================================
// XỬ LÝ LỆNH TỪ SERIAL
// ============================================

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  
  SerialMon.println("\n[CMD] Processing command: " + cmd);
  
  if (cmd == "SMS") {
    sendSMS();
  } else if (cmd == "CALL") {
    makeCall();
  } else if (cmd == "HANGUP") {
    hangupCall();
  } else if (cmd == "STATUS") {
    printStatus();
  } else {
    SerialMon.println("[ERROR] Unknown command. Use: SMS, CALL, HANGUP, or STATUS");
  }
}

// ============================================
// GỬI SMS
// ============================================

void sendSMS() {
  if (!sim4gInitialized) {
    SerialMon.println("[ERROR] SIM 4G not initialized!");
    return;
  }
  
  SerialMon.println("\n[SMS] Preparing to send SMS...");
  SerialMon.printf("[SMS] To: %s\n", PHONE_NUMBER);
  
  // Nội dung SMS (giới hạn 160 ký tự cho SMS 7-bit)
  String message = "Test SMS tu ESP32-S3. Thoi gian: " + getCurrentTime();
  if (message.length() > 160) {
    message = message.substring(0, 160);
  }
  
  SerialMon.println("[SMS] Message: " + message);
  SerialMon.printf("[SMS] Message length: %d characters\n", message.length());
  SerialMon.println("[SMS] Sending...");
  
  // Thử format số trong nước trước
  String phoneNumber = String(PHONE_NUMBER);
  
  // Gửi lệnh AT để gửi SMS
  SerialAT.print("AT+CMGS=\"");
  SerialAT.print(phoneNumber);
  SerialAT.println("\"");
  delay(1500);
  
  // Đọc prompt "> " từ modem
  String prompt = SerialAT.readString();
  if (prompt.length() > 0) {
    SerialMon.print("[SMS] Prompt: ");
    SerialMon.println(prompt);
  }
  
  // Gửi nội dung SMS
  SerialAT.print(message);
  delay(500);
  SerialAT.write(0x1A);  // Ctrl+Z để kết thúc
  delay(3000);  // Tăng delay để đợi response
  
  // Đọc phản hồi với timeout
  String response = "";
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {  // Timeout 5 giây
    if (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;
      if (response.length() > 512) {
        response = response.substring(response.length() - 512);  // Giới hạn độ dài
      }
    }
    delay(10);
  }
  
  SerialMon.print("[SMS] Response: ");
  SerialMon.println(response);
  
  // Kiểm tra response chính xác hơn
  if (response.indexOf("+CMS ERROR") != -1) {
    SerialMon.println("[ERROR] SMS sending failed! CMS ERROR detected.");
    // Tìm mã lỗi nếu có
    int errorStart = response.indexOf("+CMS ERROR:");
    if (errorStart != -1) {
      int errorEnd = response.indexOf("\n", errorStart);
      if (errorEnd == -1) errorEnd = response.length();
      String errorMsg = response.substring(errorStart, errorEnd);
      SerialMon.print("[ERROR] Error details: ");
      SerialMon.println(errorMsg);
    }
  } else if (response.indexOf("+CMGS:") != -1 && response.indexOf("OK") != -1) {
    SerialMon.println("[SUCCESS] SMS sent successfully!");
    // Lấy message reference number
    int refStart = response.indexOf("+CMGS:");
    if (refStart != -1) {
      int refEnd = response.indexOf("\n", refStart);
      if (refEnd == -1) refEnd = response.length();
      String refMsg = response.substring(refStart, refEnd);
      SerialMon.print("[INFO] Message reference: ");
      SerialMon.println(refMsg);
    }
  } else if (response.indexOf("+CMGS:") != -1) {
    SerialMon.println("[WARNING] SMS command sent but no OK confirmation.");
  } else {
    SerialMon.println("[ERROR] SMS sending failed! No valid response.");
  }
  
  SerialMon.println();
}

// ============================================
// GỌI ĐIỆN
// ============================================

void makeCall() {
  if (!sim4gInitialized) {
    SerialMon.println("[ERROR] SIM 4G not initialized!");
    return;
  }
  
  SerialMon.println("\n[CALL] Preparing to make a call...");
  SerialMon.printf("[CALL] To: %s\n", PHONE_NUMBER);
  SerialMon.println("[CALL] Dialing...");
  
  // Gửi lệnh AT để gọi điện
  SerialAT.print("ATD");
  SerialAT.print(PHONE_NUMBER);
  SerialAT.println(";");  // Dấu ; để gọi voice call
  delay(2000);
  
  // Đọc phản hồi
  String response = SerialAT.readString();
  SerialMon.print("[CALL] Response: ");
  SerialMon.println(response);
  
  if (response.indexOf("OK") != -1 || response.indexOf("CONNECT") != -1) {
    SerialMon.println("[SUCCESS] Call initiated!");
    SerialMon.println("[INFO] Call is in progress...");
    SerialMon.println("[INFO] Type 'HANGUP' to end the call (if supported)");
  } else if (response.indexOf("BUSY") != -1) {
    SerialMon.println("[WARNING] Number is busy!");
  } else if (response.indexOf("NO ANSWER") != -1) {
    SerialMon.println("[WARNING] No answer!");
  } else if (response.indexOf("NO CARRIER") != -1) {
    SerialMon.println("[ERROR] No carrier!");
  } else {
    SerialMon.println("[INFO] Call status unknown. Check response above.");
  }
  
  SerialMon.println();
}

// ============================================
// NGẮT CUỘC GỌI
// ============================================

void hangupCall() {
  SerialMon.println("\n[HANGUP] Ending call...");
  
  SerialAT.println("ATH");  // Hang up command
  delay(1000);
  
  String response = SerialAT.readString();
  SerialMon.print("[HANGUP] Response: ");
  SerialMon.println(response);
  
  if (response.indexOf("OK") != -1) {
    SerialMon.println("[SUCCESS] Call ended!");
  } else {
    SerialMon.println("[INFO] Hangup command sent.");
  }
  
  SerialMon.println();
}

// ============================================
// HIỂN THỊ TRẠNG THÁI
// ============================================

void printStatus() {
  SerialMon.println("\n=== SYSTEM STATUS ===");
  SerialMon.printf("SIM Initialized: %s\n", sim4gInitialized ? "YES" : "NO");
  SerialMon.printf("Network Open: %s\n", sim4gNetworkOpen ? "YES" : "NO");
  SerialMon.printf("Network Connected: %s\n", modem.isNetworkConnected() ? "YES" : "NO");
  SerialMon.printf("GPRS Connected: %s\n", modem.isGprsConnected() ? "YES" : "NO");
  SerialMon.printf("Phone Number: %s\n", PHONE_NUMBER);
  SerialMon.println("====================\n");
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

String getCurrentTime() {
  // Lấy thời gian từ modem (nếu có)
  SerialAT.println("AT+CCLK?");
  delay(500);
  String timeStr = SerialAT.readString();
  
  if (timeStr.indexOf("+CCLK:") != -1) {
    // Parse time from response
    int start = timeStr.indexOf("\"") + 1;
    int end = timeStr.indexOf("\"", start);
    if (start > 0 && end > start) {
      return timeStr.substring(start, end);
    }
  }
  
  // Fallback: return uptime
  unsigned long seconds = millis() / 1000;
  int hours = (seconds / 3600) % 24;
  int minutes = (seconds / 60) % 60;
  int secs = seconds % 60;
  
  char timeBuffer[20];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hours, minutes, secs);
  return String(timeBuffer);
}

