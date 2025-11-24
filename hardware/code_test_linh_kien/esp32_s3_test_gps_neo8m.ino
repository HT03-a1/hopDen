/*
 * ESP32-S3 TEST GPS NEO-8M MODULE
 * 
 * Mục đích: Test module GPS NEO-8M
 * 
 * Chức năng:
 * 1. Khởi tạo GPS NEO-8M qua UART
 * 2. Đọc và hiển thị dữ liệu GPS liên tục
 * 3. Hiển thị: vị trí (lat, lon), tốc độ, số vệ tinh, độ chính xác
 * 4. Hiển thị thời gian GPS (UTC)
 * 5. Tính vận tốc từ 2 điểm GPS (nếu GPS không có speed)
 */

#include <TinyGPS++.h>

// ============================================
// CẤU HÌNH
// ============================================

// GPS NEO-8M (UART)
#define GPS_RX_PIN      16
#define GPS_TX_PIN      20  // Đổi sang GPIO 20 để nhường GPIO 17 cho SIM 4G
#define GPS_BAUD        9600

// Serial cho GPS
HardwareSerial SerialGPS(1);

// TinyGPS++ object
TinyGPSPlus gps;

// Biến để tính vận tốc offline
float lastLat = 0;
float lastLon = 0;
unsigned long lastTime = 0;
bool hasLastPoint = false;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 TEST GPS NEO-8M ===");
  Serial.println("Initializing GPS...");
  
  // Khởi tạo GPS Serial
  SerialGPS.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS Serial initialized");
  Serial.printf("GPS RX Pin: %d, TX Pin: %d, Baud: %d\n", GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD);
  
  Serial.println("\nWaiting for GPS signal...");
  Serial.println("(Cần thời gian để GPS lock vệ tinh - có thể mất vài phút)");
  Serial.println("----------------------------------------\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Đọc dữ liệu từ GPS
  while (SerialGPS.available() > 0) {
    if (gps.encode(SerialGPS.read())) {
      // Có dữ liệu GPS mới
      displayGPSInfo();
    }
  }
  
  // Nếu không có dữ liệu GPS trong 5 giây, cảnh báo
  static unsigned long lastGPSData = 0;
  if (millis() - lastGPSData > 5000) {
    if (!gps.location.isValid()) {
      Serial.println("⚠️ No GPS signal - waiting for satellites...");
      Serial.println("   (Đảm bảo GPS module ở ngoài trời hoặc gần cửa sổ)");
      lastGPSData = millis();
    }
  }
  
  delay(100);
}

// ============================================
// HIỂN THỊ THÔNG TIN GPS
// ============================================

void displayGPSInfo() {
  if (gps.location.isValid()) {
    float lat = gps.location.lat();
    float lon = gps.location.lng();
    float speed = 0;
    unsigned long currentTime = millis();
    
    // Ưu tiên dùng speed từ GPS nếu có
    if (gps.speed.isValid() && gps.speed.kmph() > 0) {
      speed = gps.speed.kmph();
    } else if (hasLastPoint && (currentTime - lastTime) > 1000) {
      // Tính vận tốc từ 2 điểm GPS
      speed = calculateSpeedFromGPS(lastLat, lastLon, lastTime, lat, lon, currentTime);
    }
    
    // Cập nhật điểm GPS trước
    lastLat = lat;
    lastLon = lon;
    lastTime = currentTime;
    hasLastPoint = true;
    
    // Hiển thị thông tin
    Serial.println("\n=== GPS DATA ===");
    Serial.printf("📍 Location: %.6f, %.6f\n", lat, lon);
    Serial.printf("   (Lat: %.6f°, Lon: %.6f°)\n", lat, lon);
    
    if (speed > 0) {
      Serial.printf("🚗 Speed: %.2f km/h\n", speed);
    } else {
      Serial.println("🚗 Speed: 0 km/h (đứng yên)");
    }
    
    if (gps.altitude.isValid()) {
      Serial.printf("⛰️  Altitude: %.2f m\n", gps.altitude.meters());
    }
    
    if (gps.satellites.isValid()) {
      Serial.printf("🛰️  Satellites: %d\n", gps.satellites.value());
    }
    
    if (gps.hdop.isValid()) {
      Serial.printf("📊 HDOP (Accuracy): %.2f\n", gps.hdop.hdop());
      if (gps.hdop.hdop() < 1.0) {
        Serial.println("   ✅ Excellent accuracy");
      } else if (gps.hdop.hdop() < 2.0) {
        Serial.println("   ✅ Good accuracy");
      } else if (gps.hdop.hdop() < 5.0) {
        Serial.println("   ⚠️ Moderate accuracy");
      } else {
        Serial.println("   ⚠️ Poor accuracy");
      }
    }
    
    if (gps.date.isValid() && gps.time.isValid()) {
      Serial.printf("🕐 Time (UTC): %04d-%02d-%02d %02d:%02d:%02d\n",
                    gps.date.year(), gps.date.month(), gps.date.day(),
                    gps.time.hour(), gps.time.minute(), gps.time.second());
    }
    
    if (gps.course.isValid()) {
      Serial.printf("🧭 Course: %.2f°\n", gps.course.deg());
    }
    
    Serial.println("================\n");
    
  } else {
    // GPS chưa có tín hiệu
    Serial.print("⏳ Waiting for GPS fix");
    if (gps.satellites.isValid()) {
      Serial.printf(" (%d satellites visible)\n", gps.satellites.value());
    } else {
      Serial.println(" (no satellites)");
    }
  }
}

// ============================================
// TÍNH VẬN TỐC TỪ 2 ĐIỂM GPS
// ============================================

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371000.0;  // Bán kính Trái Đất (mét)
  float dLat = (lat2 - lat1) * PI / 180.0;
  float dLon = (lon2 - lon1) * PI / 180.0;
  float a = sin(dLat / 2.0) * sin(dLat / 2.0) +
            cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
            sin(dLon / 2.0) * sin(dLon / 2.0);
  float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return R * c;
}

float calculateSpeedFromGPS(float lat1, float lon1, unsigned long time1,
                             float lat2, float lon2, unsigned long time2) {
  if (time2 <= time1) {
    return 0.0;
  }
  float distance = calculateDistance(lat1, lon1, lat2, lon2);
  float timeSeconds = (time2 - time1) / 1000.0;
  if (timeSeconds <= 0) {
    return 0.0;
  }
  float speedMs = distance / timeSeconds;
  return speedMs * 3.6;  // Chuyển m/s sang km/h
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

void printGPSStats() {
  Serial.println("\n=== GPS STATISTICS ===");
  Serial.printf("Characters processed: %lu\n", gps.charsProcessed());
  Serial.printf("Sentences with fix: %lu\n", gps.sentencesWithFix());
  Serial.printf("Failed checksum: %lu\n", gps.failedChecksum());
  Serial.println("=====================\n");
}

