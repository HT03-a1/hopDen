/*
 * ESP32-S3 TEST MPU9250 (9-Axis IMU)
 * 
 * Mục đích: Test module MPU9250 (Accelerometer, Gyroscope, Magnetometer)
 * 
 * Chức năng:
 * 1. Khởi tạo MPU9250 qua I2C (address 0x69)
 * 2. Đọc gia tốc (ax, ay, az)
 * 3. Đọc góc quay (gx, gy, gz)
 * 4. Đọc từ trường (mx, my, mz)
 * 5. Tính magnitude để phát hiện va chạm
 * 6. Hiển thị dữ liệu real-time
 * 
 * Sơ đồ kết nối:
 * - I2C SDA: GPIO 8
 * - I2C SCL: GPIO 9
 * - MPU9250 Address: 0x69
 */

#include <Wire.h>

// ============================================
// CẤU HÌNH
// ============================================

// I2C Pins (theo sơ đồ mới)
#define I2C_SDA_PIN     8
#define I2C_SCL_PIN     9

// MPU9250 I2C Address
#define MPU9250_ADDRESS 0x69

// MPU9250 Registers
#define MPU9250_WHO_AM_I      0x75
#define MPU9250_PWR_MGMT_1   0x6B
#define MPU9250_ACCEL_XOUT_H 0x3B
#define MPU9250_GYRO_XOUT_H  0x43
#define MPU9250_MAG_XOUT_L   0x03  // Magnetometer (AK8963) - I2C address 0x0C

// ============================================
// BIẾN TOÀN CỤC
// ============================================

// Dữ liệu cảm biến
struct MPU9250Data {
  float ax, ay, az;  // Gia tốc (g)
  float gx, gy, gz;  // Góc quay (deg/s)
  float mx, my, mz;  // Từ trường (uT)
  float magnitude;   // Magnitude của gia tốc
  bool valid;
};

MPU9250Data mpuData;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== ESP32-S3 TEST MPU9250 ===");
  Serial.println("=============================\n");
  
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
  
  // Khởi tạo MPU9250
  Serial.println("=== INITIALIZING MPU9250 ===");
  if (initMPU9250()) {
    Serial.println("✓ MPU9250 initialized successfully!");
  } else {
    Serial.println("❌ ERROR: Failed to initialize MPU9250!");
    Serial.println("Check:");
    Serial.println("  - I2C connections (SDA: GPIO 8, SCL: GPIO 9)");
    Serial.println("  - I2C address: 0x69");
    Serial.println("  - Power supply (3.3V)");
    while (1) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("\n=== READING MPU9250 DATA ===");
  Serial.println("Format: Accel(g) | Gyro(deg/s) | Mag(uT) | Magnitude(g)");
  Serial.println("--------------------------------------------------------\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Đọc dữ liệu từ MPU9250
  readMPU9250();
  
  if (mpuData.valid) {
    // Hiển thị gia tốc
    Serial.print("Accel: ");
    Serial.print(mpuData.ax, 2);
    Serial.print(", ");
    Serial.print(mpuData.ay, 2);
    Serial.print(", ");
    Serial.print(mpuData.az, 2);
    Serial.print(" g");
    
    // Hiển thị góc quay
    Serial.print(" | Gyro: ");
    Serial.print(mpuData.gx, 1);
    Serial.print(", ");
    Serial.print(mpuData.gy, 1);
    Serial.print(", ");
    Serial.print(mpuData.gz, 1);
    Serial.print(" deg/s");
    
    // Hiển thị từ trường
    Serial.print(" | Mag: ");
    Serial.print(mpuData.mx, 1);
    Serial.print(", ");
    Serial.print(mpuData.my, 1);
    Serial.print(", ");
    Serial.print(mpuData.mz, 1);
    Serial.print(" uT");
    
    // Hiển thị magnitude
    Serial.print(" | Magnitude: ");
    Serial.print(mpuData.magnitude, 2);
    Serial.print(" g");
    
    // Cảnh báo nếu magnitude lớn (có thể va chạm)
    if (mpuData.magnitude > 2.5) {
      Serial.print(" ⚠️ COLLISION DETECTED!");
    }
    
    Serial.println();
  } else {
    Serial.println("❌ ERROR: Failed to read MPU9250 data!");
  }
  
  delay(100);  // Đọc mỗi 100ms
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
        Serial.println(" -> DS1307 RTC (0x50)");
      } else if (address == 0x68) {
        Serial.println(" -> DS1307 RTC (0x68)");
      } else if (address == 0x69) {
        Serial.println(" -> MPU9250");
      } else if (address == 0x3C || address == 0x3D) {
        Serial.println(" -> OLED SSD1306");
      } else {
        Serial.println(" -> Unknown device");
      }
      
      nDevices++;
    }
  }
  
  Serial.println();
  if (nDevices == 0) {
    Serial.println("❌ No I2C devices found!");
  } else {
    Serial.print("Found ");
    Serial.print(nDevices);
    Serial.println(" device(s)");
  }
}

// ============================================
// KHỞI TẠO MPU9250
// ============================================

bool initMPU9250() {
  // Kiểm tra WHO_AM_I register
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_WHO_AM_I);
  if (Wire.endTransmission() != 0) {
    Serial.println("  ✗ Cannot communicate with MPU9250");
    return false;
  }
  
  Wire.requestFrom(MPU9250_ADDRESS, 1);
  if (Wire.available()) {
    byte whoAmI = Wire.read();
    Serial.print("  WHO_AM_I register: 0x");
    if (whoAmI < 16) Serial.print("0");
    Serial.println(whoAmI, HEX);
    
    // MPU9250: 0x71, MPU6500: 0x70
    if (whoAmI != 0x71 && whoAmI != 0x70) {
      Serial.print("  ✗ Unexpected WHO_AM_I value: 0x");
      Serial.println(whoAmI, HEX);
      Serial.println("  Expected: 0x71 (MPU9250) or 0x70 (MPU6500)");
      return false;
    }
  } else {
    Serial.println("  ✗ No response from MPU9250");
    return false;
  }
  
  // Reset MPU9250
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_PWR_MGMT_1);
  Wire.write(0x80);  // Reset bit
  if (Wire.endTransmission() != 0) {
    Serial.println("  ✗ Failed to reset MPU9250");
    return false;
  }
  delay(100);
  
  // Wake up MPU9250 (clear sleep mode)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_PWR_MGMT_1);
  Wire.write(0x00);  // Clear sleep bit
  if (Wire.endTransmission() != 0) {
    Serial.println("  ✗ Failed to wake up MPU9250");
    return false;
  }
  delay(10);
  
  // Cấu hình Accelerometer: ±2g, 1kHz
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(0x1C);  // ACCEL_CONFIG
  Wire.write(0x00);  // ±2g (0b00000000)
  Wire.endTransmission();
  delay(10);
  
  // Cấu hình Gyroscope: ±250deg/s, 1kHz
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(0x1B);  // GYRO_CONFIG
  Wire.write(0x00);  // ±250deg/s (0b00000000)
  Wire.endTransmission();
  delay(10);
  
  Serial.println("  ✓ MPU9250 configured");
  
  return true;
}

// ============================================
// ĐỌC DỮ LIỆU MPU9250
// ============================================

void readMPU9250() {
  mpuData.valid = false;
  
  // Đọc Accelerometer (14 bytes: 6 bytes accel + 1 byte temp + 6 bytes gyro + 1 byte reserved)
  Wire.beginTransmission(MPU9250_ADDRESS);
  Wire.write(MPU9250_ACCEL_XOUT_H);
  if (Wire.endTransmission() != 0) {
    return;
  }
  
  Wire.requestFrom(MPU9250_ADDRESS, 14);
  if (Wire.available() < 14) {
    return;
  }
  
  // Đọc Accelerometer (16-bit, big-endian)
  int16_t accelX = (Wire.read() << 8) | Wire.read();
  int16_t accelY = (Wire.read() << 8) | Wire.read();
  int16_t accelZ = (Wire.read() << 8) | Wire.read();
  
  // Bỏ qua Temperature (2 bytes)
  Wire.read();
  Wire.read();
  
  // Đọc Gyroscope (16-bit, big-endian)
  int16_t gyroX = (Wire.read() << 8) | Wire.read();
  int16_t gyroY = (Wire.read() << 8) | Wire.read();
  int16_t gyroZ = (Wire.read() << 8) | Wire.read();
  
  // Chuyển đổi sang đơn vị thực
  // Accelerometer: ±2g = 16384 LSB/g
  mpuData.ax = accelX / 16384.0;
  mpuData.ay = accelY / 16384.0;
  mpuData.az = accelZ / 16384.0;
  
  // Gyroscope: ±250deg/s = 131 LSB/deg/s
  mpuData.gx = gyroX / 131.0;
  mpuData.gy = gyroY / 131.0;
  mpuData.gz = gyroZ / 131.0;
  
  // Tính magnitude của gia tốc
  mpuData.magnitude = sqrt(mpuData.ax * mpuData.ax + 
                          mpuData.ay * mpuData.ay + 
                          mpuData.az * mpuData.az);
  
  // Đọc Magnetometer (nếu cần)
  // MPU9250 có magnetometer tích hợp (AK8963) ở địa chỉ 0x0C
  // Cần cấu hình thêm để đọc magnetometer
  mpuData.mx = 0;
  mpuData.my = 0;
  mpuData.mz = 0;
  
  mpuData.valid = true;
}

// ============================================
// HÀM ĐỌC MAGNETOMETER (TÙY CHỌN)
// ============================================

void readMagnetometer() {
  // AK8963 (magnetometer) có địa chỉ 0x0C
  // Cần cấu hình MPU9250 để truy cập magnetometer qua I2C passthrough
  // Hoặc dùng thư viện chuyên dụng
  
  // TODO: Implement magnetometer reading if needed
}

// ============================================
// HÀM PHÁT HIỆN VA CHẠM
// ============================================

bool detectCollision(float threshold) {
  if (!mpuData.valid) {
    return false;
  }
  
  return mpuData.magnitude > threshold;
}

// ============================================
// HÀM HIỂN THỊ DỮ LIỆU DẠNG BẢNG
// ============================================

void printDataTable() {
  Serial.println("\n=== MPU9250 DATA TABLE ===");
  Serial.println("Accel X\tAccel Y\tAccel Z\tGyro X\tGyro Y\tGyro Z\tMagnitude");
  Serial.println("--------------------------------------------------------");
  
  for (int i = 0; i < 10; i++) {
    readMPU9250();
    if (mpuData.valid) {
      Serial.print(mpuData.ax, 2);
      Serial.print("\t");
      Serial.print(mpuData.ay, 2);
      Serial.print("\t");
      Serial.print(mpuData.az, 2);
      Serial.print("\t");
      Serial.print(mpuData.gx, 1);
      Serial.print("\t");
      Serial.print(mpuData.gy, 1);
      Serial.print("\t");
      Serial.print(mpuData.gz, 1);
      Serial.print("\t");
      Serial.println(mpuData.magnitude, 2);
    }
    delay(100);
  }
}



