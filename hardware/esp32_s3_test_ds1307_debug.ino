/*
 * ESP32-S3 TEST DS1307 RTC - DEBUG VERSION
 * 
 * Mục đích: Debug DS1307 khi quét được địa chỉ nhưng code mẫu báo lỗi
 * 
 * Chức năng:
 * 1. Quét I2C để tìm tất cả thiết bị
 * 2. Test kết nối DS1307 chi tiết
 * 3. Đọc/ghi register trực tiếp
 * 4. Debug thông tin lỗi
 */

#include <Wire.h>
#include <RTClib.h>

// ============================================
// CẤU HÌNH
// ============================================

// I2C Pins (theo sơ đồ mới)
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// DS1307 I2C Address: 0x68
#define DS1307_ADDRESS  0x68

// ============================================
// BIẾN TOÀN CỤC
// ============================================

RTC_DS1307 rtc;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(2000);  // Đợi Serial Monitor mở
  
  Serial.println("\n\n=== ESP32-S3 TEST DS1307 - DEBUG ===");
  Serial.println("=====================================\n");
  
  // Khởi tạo I2C
  Serial.print("Initializing I2C... ");
  Serial.print("SDA: GPIO ");
  Serial.print(I2C_SDA_PIN);
  Serial.print(", SCL: GPIO ");
  Serial.println(I2C_SCL_PIN);
  
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(100);  // Đợi I2C ổn định
  Serial.println("✓ I2C initialized\n");
  
  // Quét I2C bus
  Serial.println("=== SCANNING I2C BUS ===");
  scanI2C();
  Serial.println();
  
  // Test kết nối DS1307 trực tiếp
  Serial.println("=== TESTING DS1307 CONNECTION ===");
  testDS1307Connection();
  Serial.println();
  
  // Test với thư viện RTClib
  Serial.println("=== TESTING WITH RTCLIB ===");
  testRTClib();
  Serial.println();
  
  Serial.println("=== TEST COMPLETE ===");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Đọc thời gian mỗi giây
  if (rtc.begin() && rtc.isrunning()) {
    DateTime now = rtc.now();
    
    Serial.print("Time: ");
    Serial.print(now.year());
    Serial.print("-");
    Serial.print(now.month());
    Serial.print("-");
    Serial.print(now.day());
    Serial.print(" ");
    Serial.print(now.hour());
    Serial.print(":");
    Serial.print(now.minute());
    Serial.print(":");
    Serial.println(now.second());
  } else {
    Serial.println("RTC not available");
  }
  
  delay(1000);
}

// ============================================
// QUÉT I2C BUS
// ============================================

void scanI2C() {
  byte error, address;
  int nDevices = 0;
  
  Serial.println("Scanning I2C bus...");
  Serial.println("Address range: 0x08 - 0x77");
  Serial.println();
  
  for (address = 0x08; address < 0x78; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("✓ I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      
      // Xác định thiết bị
      if (address == 0x50) {
        Serial.println(" -> MPU9250");
      } else if (address == 0x68) {
        Serial.println(" -> DS1307 RTC");
      } else if (address == 0x3C || address == 0x3D) {
        Serial.println(" -> OLED SSD1306");
      } else {
        Serial.println(" -> Unknown device");
      }
      
      nDevices++;
    } else if (error == 4) {
      Serial.print("✗ Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  Serial.println();
  if (nDevices == 0) {
    Serial.println("❌ No I2C devices found!");
    Serial.println("Check:");
    Serial.println("  - I2C connections (SDA, SCL)");
    Serial.println("  - Power supply");
    Serial.println("  - Pull-up resistors (4.7kΩ)");
  } else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)");
  }
}

// ============================================
// TEST KẾT NỐI DS1307 TRỰC TIẾP
// ============================================

void testDS1307Connection() {
  Serial.print("Testing DS1307 at address 0x");
  Serial.print(DS1307_ADDRESS, HEX);
  Serial.print("... ");
  
  Wire.beginTransmission(DS1307_ADDRESS);
  byte error = Wire.endTransmission();
  
  if (error == 0) {
    Serial.println("✓ Device responds!");
    
    // Đọc register 0x00 (seconds)
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write(0x00);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Wire.requestFrom(DS1307_ADDRESS, 1);
      if (Wire.available()) {
        byte seconds = Wire.read();
        Serial.print("  Register 0x00 (seconds): 0x");
        if (seconds < 16) Serial.print("0");
        Serial.println(seconds, HEX);
        
        // Kiểm tra CH bit (bit 7) - nếu = 1 thì RTC bị dừng
        if (seconds & 0x80) {
          Serial.println("  ⚠️ WARNING: CH bit is set (RTC is halted)");
        } else {
          Serial.println("  ✓ CH bit is clear (RTC is running)");
        }
      }
    }
    
    // Đọc register 0x03 (day of week)
    Wire.beginTransmission(DS1307_ADDRESS);
    Wire.write(0x03);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Wire.requestFrom(DS1307_ADDRESS, 1);
      if (Wire.available()) {
        byte day = Wire.read();
        Serial.print("  Register 0x03 (day): 0x");
        if (day < 16) Serial.print("0");
        Serial.println(day, HEX);
      }
    }
    
  } else if (error == 2) {
    Serial.println("✗ Error: Received NACK on transmit of address");
    Serial.println("  -> Device not found or address wrong");
  } else if (error == 3) {
    Serial.println("✗ Error: Received NACK on transmit of data");
  } else if (error == 4) {
    Serial.println("✗ Error: Other error");
  } else if (error == 5) {
    Serial.println("✗ Error: Timeout");
  } else {
    Serial.print("✗ Error code: ");
    Serial.println(error);
  }
}

// ============================================
// TEST VỚI THƯ VIỆN RTCLIB
// ============================================

void testRTClib() {
  Serial.print("Testing rtc.begin()... ");
  
  if (rtc.begin()) {
    Serial.println("✓ SUCCESS!");
    
    Serial.print("Testing rtc.isrunning()... ");
    if (rtc.isrunning()) {
      Serial.println("✓ RTC is running");
      
      // Đọc thời gian
      DateTime now = rtc.now();
      Serial.print("Current time: ");
      printDateTime(now);
      Serial.println();
      
      // Kiểm tra tính hợp lệ
      if (now.year() < 2000 || now.year() > 2100) {
        Serial.println("⚠️ WARNING: Invalid year!");
        Serial.println("  Setting time from compile time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("  Time set!");
      }
      
    } else {
      Serial.println("✗ RTC is NOT running");
      Serial.println("  Setting time from compile time...");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("  Time set!");
    }
    
  } else {
    Serial.println("✗ FAILED!");
    Serial.println();
    Serial.println("Possible causes:");
    Serial.println("  1. Wrong I2C address (should be 0x68)");
    Serial.println("  2. Wrong I2C pins (should be GPIO 8, 9)");
    Serial.println("  3. No pull-up resistors on SDA/SCL");
    Serial.println("  4. Power supply issue");
    Serial.println("  5. RTClib library version incompatible");
    Serial.println();
    Serial.println("Try:");
    Serial.println("  - Check I2C scanner results above");
    Serial.println("  - Verify connections");
    Serial.println("  - Try different RTClib version");
  }
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

void printDateTime(DateTime dt) {
  char dateStr[32];
  snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year(), dt.month(), dt.day(),
           dt.hour(), dt.minute(), dt.second());
  Serial.print(dateStr);
  
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  Serial.print(" (");
  Serial.print(days[dt.dayOfTheWeek()]);
  Serial.print(")");
}



