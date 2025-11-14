/*
 * ESP32-S3 TEST OLED SSD1306 0.96" DISPLAY
 * 
 * Mục đích: Test màn hình OLED SSD1306 0.96 inch
 * 
 * Chức năng:
 * 1. Khởi tạo OLED qua I2C
 * 2. Hiển thị text, số, hình vẽ
 * 3. Test các font khác nhau
 * 4. Test animation và hiệu ứng
 * 5. Hiển thị thông tin hệ thống
 */

#include <Wire.h>
#include <U8g2lib.h>

// ============================================
// CẤU HÌNH
// ============================================

// I2C Pins
#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22

// OLED SSD1306 0.96" (128x64) - I2C address 0x3C
U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8G2_I2C_OPT_NONE);

// ============================================
// BIẾN TOÀN CỤC
// ============================================

unsigned long startTime = 0;
int frameCount = 0;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 TEST OLED SSD1306 ===");
  
  // Khởi tạo I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized");
  
  // Khởi tạo OLED
  oled.begin();
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.clearBuffer();
  
  // Hiển thị màn hình khởi động
  oled.drawStr(0, 20, "OLED TEST");
  oled.drawStr(0, 40, "Initializing...");
  oled.sendBuffer();
  Serial.println("OLED initialized");
  
  delay(2000);
  
  startTime = millis();
  Serial.println("\nStarting OLED test...");
  Serial.println("You should see various displays on OLED");
}

// ============================================
// LOOP
// ============================================

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsed = (currentTime - startTime) / 1000;  // Giây
  
  // Chuyển đổi giữa các test mỗi 3 giây
  int testNumber = (elapsed / 3) % 6;
  
  switch (testNumber) {
    case 0:
      testBasicText();
      break;
    case 1:
      testNumbers();
      break;
    case 2:
      testGraphics();
      break;
    case 3:
      testSystemInfo();
      break;
    case 4:
      testAnimation();
      break;
    case 5:
      testFonts();
      break;
  }
  
  frameCount++;
  delay(100);  // Update mỗi 100ms
}

// ============================================
// TEST: TEXT CƠ BẢN
// ============================================

void testBasicText() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  
  oled.drawStr(0, 12, "Test 1: Basic Text");
  oled.drawStr(0, 28, "Hello World!");
  oled.drawStr(0, 44, "ESP32-S3");
  oled.drawStr(0, 60, "OLED Display");
  
  oled.sendBuffer();
}

// ============================================
// TEST: HIỂN THỊ SỐ
// ============================================

void testNumbers() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  
  oled.drawStr(0, 12, "Test 2: Numbers");
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "Frame: %lu", frameCount);
  oled.drawStr(0, 28, buffer);
  
  unsigned long uptime = millis() / 1000;
  snprintf(buffer, sizeof(buffer), "Uptime: %lu s", uptime);
  oled.drawStr(0, 44, buffer);
  
  float voltage = 3.3;  // Giả sử
  snprintf(buffer, sizeof(buffer), "Voltage: %.2fV", voltage);
  oled.drawStr(0, 60, buffer);
  
  oled.sendBuffer();
}

// ============================================
// TEST: ĐỒ HỌA
// ============================================

void testGraphics() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  
  oled.drawStr(0, 12, "Test 3: Graphics");
  
  // Vẽ đường thẳng
  oled.drawLine(0, 20, 127, 20);
  oled.drawLine(0, 30, 127, 30);
  
  // Vẽ hình chữ nhật
  oled.drawFrame(10, 35, 30, 20);
  oled.drawBox(50, 35, 30, 20);
  
  // Vẽ hình tròn
  oled.drawCircle(100, 45, 10);
  
  oled.sendBuffer();
}

// ============================================
// TEST: THÔNG TIN HỆ THỐNG
// ============================================

void testSystemInfo() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  
  oled.drawStr(0, 12, "Test 4: System Info");
  
  char buffer[32];
  
  // Free heap
  snprintf(buffer, sizeof(buffer), "Heap: %d KB", ESP.getFreeHeap() / 1024);
  oled.drawStr(0, 28, buffer);
  
  // CPU frequency
  snprintf(buffer, sizeof(buffer), "CPU: %d MHz", ESP.getCpuFreqMHz());
  oled.drawStr(0, 44, buffer);
  
  // Chip model
  oled.drawStr(0, 60, "ESP32-S3");
  
  oled.sendBuffer();
}

// ============================================
// TEST: ANIMATION
// ============================================

void testAnimation() {
  oled.clearBuffer();
  oled.setFont(u8g2_font_ncenB08_tr);
  
  oled.drawStr(0, 12, "Test 5: Animation");
  
  // Thanh tiến trình di chuyển
  int progress = (frameCount % 100);
  int barWidth = (progress * 100) / 100;
  
  oled.drawFrame(10, 30, 100, 10);
  oled.drawBox(10, 30, barWidth, 10);
  
  // Văn bản di chuyển
  int textX = (frameCount * 2) % 128;
  oled.drawStr(textX, 50, "Moving text");
  
  // Hình tròn nhấp nháy
  if ((frameCount / 10) % 2 == 0) {
    oled.drawCircle(100, 60, 5);
  }
  
  oled.sendBuffer();
}

// ============================================
// TEST: FONTS KHÁC NHAU
// ============================================

void testFonts() {
  oled.clearBuffer();
  
  // Font nhỏ
  oled.setFont(u8g2_font_ncenB08_tr);
  oled.drawStr(0, 12, "Test 6: Fonts");
  
  // Font lớn hơn
  oled.setFont(u8g2_font_ncenB14_tr);
  oled.drawStr(0, 35, "BIG");
  
  // Font nhỏ
  oled.setFont(u8g2_font_ncenR08_tr);
  oled.drawStr(0, 50, "Small text");
  
  // Font số lớn
  oled.setFont(u8g2_font_ncenB18_tn);
  char numStr[8];
  snprintf(numStr, sizeof(numStr), "%d", frameCount % 100);
  oled.drawStr(80, 50, numStr);
  
  oled.sendBuffer();
}

// ============================================
// HÀM TIỆN ÍCH
// ============================================

void clearScreen() {
  oled.clearBuffer();
  oled.sendBuffer();
}

void drawCenteredText(const char* text, int y) {
  int textWidth = oled.getStrWidth(text);
  int x = (128 - textWidth) / 2;
  oled.drawStr(x, y, text);
}

