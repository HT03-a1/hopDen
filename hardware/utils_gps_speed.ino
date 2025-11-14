/*
 * UTILITY: Tính vận tốc từ 2 điểm GPS
 * 
 * Hàm tiện ích để tính khoảng cách và vận tốc giữa 2 điểm GPS
 * Sử dụng công thức Haversine để tính khoảng cách trên mặt cầu
 * 
 * Cách sử dụng:
 * 1. Lưu điểm GPS trước (lat1, lon1, time1)
 * 2. Khi có điểm GPS mới (lat2, lon2, time2)
 * 3. Gọi calculateSpeed() để tính vận tốc
 */

// ============================================
// HÀM TÍNH KHOẢNG CÁCH GIỮA 2 ĐIỂM GPS
// ============================================

/**
 * Tính khoảng cách giữa 2 điểm GPS (Haversine formula)
 * @param lat1 Vĩ độ điểm 1 (độ)
 * @param lon1 Kinh độ điểm 1 (độ)
 * @param lat2 Vĩ độ điểm 2 (độ)
 * @param lon2 Kinh độ điểm 2 (độ)
 * @return Khoảng cách tính bằng mét (m)
 */
float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  // Bán kính Trái Đất (mét)
  const float R = 6371000.0;  // 6371 km = 6371000 m
  
  // Chuyển độ sang radian
  float dLat = (lat2 - lat1) * PI / 180.0;
  float dLon = (lon2 - lon1) * PI / 180.0;
  
  float a = sin(dLat / 2.0) * sin(dLat / 2.0) +
            cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
            sin(dLon / 2.0) * sin(dLon / 2.0);
  
  float c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  float distance = R * c;
  
  return distance;
}

/**
 * Tính vận tốc từ 2 điểm GPS và thời gian
 * @param lat1 Vĩ độ điểm 1 (độ)
 * @param lon1 Kinh độ điểm 1 (độ)
 * @param time1 Thời gian điểm 1 (milliseconds)
 * @param lat2 Vĩ độ điểm 2 (độ)
 * @param lon2 Kinh độ điểm 2 (độ)
 * @param time2 Thời gian điểm 2 (milliseconds)
 * @return Vận tốc tính bằng km/h (0 nếu thời gian <= 0)
 */
float calculateSpeed(float lat1, float lon1, unsigned long time1,
                     float lat2, float lon2, unsigned long time2) {
  // Kiểm tra thời gian hợp lệ
  if (time2 <= time1) {
    return 0.0;  // Không di chuyển hoặc thời gian không hợp lệ
  }
  
  // Tính khoảng cách (mét)
  float distance = calculateDistance(lat1, lon1, lat2, lon2);
  
  // Tính thời gian (giây)
  float timeSeconds = (time2 - time1) / 1000.0;
  
  if (timeSeconds <= 0) {
    return 0.0;
  }
  
  // Tính vận tốc (m/s)
  float speedMs = distance / timeSeconds;
  
  // Chuyển sang km/h
  float speedKmh = speedMs * 3.6;
  
  return speedKmh;
}

/**
 * Tính vận tốc từ khoảng cách và thời gian (đã biết)
 * @param distance Khoảng cách (mét)
 * @param timeMs Thời gian (milliseconds)
 * @return Vận tốc tính bằng km/h
 */
float calculateSpeedFromDistance(float distance, unsigned long timeMs) {
  if (timeMs <= 0) {
    return 0.0;
  }
  
  float timeSeconds = timeMs / 1000.0;
  float speedMs = distance / timeSeconds;
  float speedKmh = speedMs * 3.6;
  
  return speedKmh;
}

// ============================================
// VÍ DỤ SỬ DỤNG
// ============================================

/*
// Lưu điểm GPS trước
float lastLat = 0;
float lastLon = 0;
unsigned long lastTime = 0;
bool hasLastPoint = false;

void updateGPSAndCalculateSpeed(float currentLat, float currentLon) {
  unsigned long currentTime = millis();
  
  if (hasLastPoint) {
    // Tính vận tốc từ điểm trước và điểm hiện tại
    float speed = calculateSpeed(
      lastLat, lastLon, lastTime,
      currentLat, currentLon, currentTime
    );
    
    Serial.printf("Speed: %.2f km/h\n", speed);
  }
  
  // Cập nhật điểm trước
  lastLat = currentLat;
  lastLon = currentLon;
  lastTime = currentTime;
  hasLastPoint = true;
}
*/

