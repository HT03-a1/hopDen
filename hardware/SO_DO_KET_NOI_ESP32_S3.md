# SƠ ĐỒ KẾT NỐI LINH KIỆN VỚI ESP32-S3

## 📋 DANH SÁCH LINH KIỆN

1. **ESP32-S3** (MCU chính)
2. **MPU9250** (Accelerometer/Gyroscope/Magnetometer - I2C)
3. **DS1307 RTC** (Real-Time Clock - I2C)
4. **OLED SSD1306 0.96"** (Màn hình - I2C)
5. **DHT11** (Cảm biến nhiệt độ/độ ẩm - GPIO)
6. **GPS NEO-8M** (GPS module - UART)
7. **SIM 4G Module** (SIM7600 hoặc tương tự - UART)
8. **INMP441** (Microphone - I2S)
9. **SD Card Module** (Thẻ nhớ - SPI)
10. **Button 1** (Nút SOS tai nạn - GPIO)
11. **Button 2** (Nút SOS cứu hộ - GPIO)
12. **Buzzer** (Còi báo - GPIO)

---

## 🔌 SƠ ĐỒ KẾT NỐI CHI TIẾT

### 1. I2C BUS (Chung cho MPU9250, DS1307, OLED)

```
ESP32-S3          MPU9250          DS1307          OLED SSD1306
--------          -------          ------          -------------
GPIO 8 (SDA)  ---- SDA ------------ SDA ------------ SDA
GPIO 9 (SCL)  ---- SCL ------------ SCL ------------ SCL
3.3V             -- VCC ----------- VCC ------------ VCC
3.3V             -- AD0 ----------- (không dùng)     (không dùng)
GND              -- GND ----------- GND ------------ GND
```

**Lưu ý:**
- Tất cả các module I2C kết nối song song trên cùng 1 bus
- **MPU9250**: Address **0x69**
- **DS1307**: Address **0x50** hoặc **0x68** (tùy module)
- **OLED SSD1306**: Address **0x3C**
- Cần pull-up resistors 4.7kΩ trên SDA và SCL (thường có sẵn trên module)

---

### 2. GPS NEO-8M (UART)

```
ESP32-S3          GPS NEO-8M
--------          -----------
GPIO 6 (RX)   ---- TX
GPIO 7 (TX)   ---- RX
3.3V             -- VCC
GND            -- GND
```

**Lưu ý:**
- GPS TX kết nối với ESP32 RX (GPIO 6)
- GPS RX kết nối với ESP32 TX (GPIO 7)
- GPS cần nguồn 3.3V hoặc 5V (tùy module)
- Đặt GPS ở ngoài trời hoặc gần cửa sổ để nhận tín hiệu tốt

---

### 3. SIM 4G MODULE (UART) - GPIO 17, 18

```
ESP32-S3          SIM 4G Module
--------          -------------
GPIO 18 (RX) ---- TX
GPIO 17 (TX) ---- RX
3.3V/5V          -- VCC (tùy module)
GND              -- GND
GPIO (optional)  -- RESET (nếu có)
```

**Lưu ý:**
- SIM module TX kết nối với ESP32 RX (GPIO 18)
- SIM module RX kết nối với ESP32 TX (GPIO 17)
- Kiểm tra điện áp của module (3.3V hoặc 5V)
- Cần SIM card và ăng-ten 4G
- Module cần dòng lớn (500mA-2A), dùng nguồn riêng nếu cần

---

### 4. INMP441 MICROPHONE (I2S)

```
ESP32-S3          INMP441
--------          -------
GPIO 42 (WS) ---- WS (LRCL)
GPIO 40 (SD) ---- SD (DOUT)
GPIO 41 (SCK) ---- SCK (BCLK)
3.3V          -- VDD
GND           -- GND
GND           -- L/R (chọn channel)
```

**Lưu ý:**
- INMP441 là microphone I2S digital
- WS = Word Select (Left/Right Clock)
- SD = Serial Data (Data Output)
- SCK = Serial Clock (Bit Clock)
- L/R nối GND = chọn channel trái

---

### 5. SD CARD MODULE (SPI)

```
ESP32-S3          SD Card Module
--------          --------------
GPIO 11 (MOSI) -- MOSI
GPIO 13 (MISO) -- MISO
GPIO 12 (SCK)  -- SCK
GPIO 10 (CS)   -- CS
3.3V            -- VCC
GND             -- GND
```

**Lưu ý:**
- SD card module dùng SPI protocol
- CS (Chip Select) có thể dùng GPIO khác nếu cần
- Format SD card: FAT32
- Dung lượng khuyến nghị: 8GB-32GB

---

### 6. DHT11 (GPIO)

```
ESP32-S3          DHT11
--------          -----
GPIO 35          -- DATA
3.3V             -- VCC
GND              -- GND
```

**Lưu ý:**
- DHT11 cần pull-up resistor 4.7kΩ-10kΩ trên chân DATA
- Nhiều module DHT11 đã có sẵn pull-up resistor

---

### 7. BUTTONS (GPIO với Pull-up)

```
ESP32-S3          Button 1          Button 2
--------          --------          --------
GPIO 36          -- Một chân       --
GND              -- Chân còn lại    --
GPIO 37          --                -- Một chân
GND              --                -- Chân còn lại
```

**Lưu ý:**
- Buttons dùng INPUT_PULLUP (ESP32 có sẵn)
- Khi nhấn = LOW (0), khi thả = HIGH (1)
- Button 1: GPIO 36 (SOS tai nạn)
- Button 2: GPIO 37 (SOS cứu hộ)

---

### 8. BUZZER (GPIO)

```
ESP32-S3          Buzzer
--------          ------
GPIO 15           -- Chân dương (+)
GND               -- Chân âm (-)
```

**Lưu ý:**
- Buzzer có thể là active (có sẵn mạch) hoặc passive (cần PWM)
- Nếu passive, dùng PWM để tạo âm thanh
- Nếu active, chỉ cần digitalWrite HIGH/LOW

---

## 🔋 NGUỒN ĐIỆN

### Yêu cầu nguồn:
- **ESP32-S3**: 3.3V, ~200-500mA (peak)
- **SIM 4G Module**: 3.3V-5V, ~500mA-2A (peak khi gửi dữ liệu)
- **Tổng cộng**: Khuyến nghị nguồn 5V, 2A-3A ổn định

### Kết nối nguồn:
```
Nguồn 5V/2A
    |
    +---> ESP32-S3 (VIN hoặc 5V pin)
    |
    +---> SIM 4G Module (VCC)
    |
    +---> Các module khác (3.3V từ ESP32 hoặc nguồn riêng)
```

**Lưu ý:**
- Dùng nguồn ổn định, có tụ lọc
- Thêm tụ 100µF-1000µF gần ESP32 để ổn định nguồn
- SIM 4G module có thể cần nguồn riêng nếu tiêu thụ cao

---

## 📐 SƠ ĐỒ TỔNG QUAN

```
                    ┌─────────────┐
                    │  ESP32-S3  │
                    └──────┬──────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
    ┌───┴───┐          ┌───┴───┐         ┌───┴───┐
    │  I2C  │          │ UART  │         │  SPI  │
    └───┬───┘          └───┬───┘         └───┬───┘
        │                  │                  │
   ┌────┴────┐        ┌────┴────┐       ┌────┴────┐
   │ MPU9250│        │  GPS    │       │ SD Card │
   │ DS1307 │        │  NEO-8M │       └─────────┘
   │ OLED   │        │ SIM 4G  │
   └────────┘        └─────────┘
        │
    ┌───┴───┐
    │ I2S  │
    └───┬───┘
        │
    ┌───┴───┐
    │INMP441│
    └───────┘

GPIO:
- GPIO 35: DHT11
- GPIO 36: Button 1
- GPIO 37: Button 2
- GPIO 15: Buzzer
```

---

## ⚠️ LƯU Ý QUAN TRỌNG

### 1. Điện áp:
- **ESP32-S3**: 3.3V logic
- **SIM 4G**: Kiểm tra module (3.3V hoặc 5V)
- **GPS**: Thường 3.3V hoặc 5V
- **SD Card**: 3.3V
- **Các module khác**: Thường 3.3V hoặc 5V

### 2. Dòng điện:
- **ESP32-S3**: ~200-500mA
- **SIM 4G**: ~500mA-2A (peak)
- **Tổng**: Cần nguồn 2A-3A ổn định

### 3. Pull-up Resistors:
- **I2C**: 4.7kΩ trên SDA và SCL (thường có sẵn)
- **DHT11**: 4.7kΩ-10kΩ trên DATA (thường có sẵn)
- **Buttons**: Dùng INPUT_PULLUP của ESP32

### 4. Khoảng cách dây:
- **I2C**: Tối đa ~1m (tốt nhất < 30cm)
- **SPI**: Tối đa ~30cm
- **UART**: Tối đa vài mét (tùy tốc độ baud)

### 5. Nhiễu:
- Đặt module xa nguồn nhiễu (motor, relay, v.v.)
- Dùng dây bọc chống nhiễu nếu cần
- Thêm tụ lọc nguồn

---

## 🔧 KIỂM TRA SAU KHI KẾT NỐI

1. **Kiểm tra nguồn:**
   - Đo điện áp tại các điểm: 3.3V, 5V
   - Kiểm tra dòng tiêu thụ

2. **Kiểm tra I2C:**
   - Quét địa chỉ I2C bằng I2C scanner
   - Xác nhận: MPU9250 (0x69), DS1307 (0x50 hoặc 0x68), OLED (0x3C)

3. **Kiểm tra UART:**
   - GPS: Gửi lệnh AT qua Serial, nhận NMEA data
   - SIM 4G: Gửi "AT", nhận "OK"

4. **Kiểm tra SPI:**
   - SD Card: Đọc/ghi file test

5. **Kiểm tra GPIO:**
   - Buttons: Đọc trạng thái HIGH/LOW
   - Buzzer: Phát âm thanh test
   - DHT11: Đọc nhiệt độ/độ ẩm

---

## 📝 TÓM TẮT CHÂN GPIO ESP32-S3

| Chức năng | GPIO | Hướng | Ghi chú |
|-----------|------|-------|---------|
| I2C SDA | 8 | I/O | MPU9250 (0x69), DS1307 (0x50/0x68), OLED (0x3C) |
| I2C SCL | 9 | I/O | MPU9250 (0x69), DS1307 (0x50/0x68), OLED (0x3C) |
| GPS RX | 6 | Input | Nhận từ GPS TX |
| GPS TX | 7 | Output | Gửi đến GPS RX |
| SIM RX | 18 | Input | Nhận từ SIM TX |
| SIM TX | 17 | Output | Gửi đến SIM RX |
| I2S WS | 42 | Output | INMP441 |
| I2S SD | 40 | Input | INMP441 |
| I2S SCK | 41 | Output | INMP441 |
| SPI MOSI | 11 | Output | SD Card |
| SPI MISO | 13 | Input | SD Card |
| SPI SCK | 12 | Output | SD Card |
| SPI CS | 10 | Output | SD Card |
| DHT11 | 35 | I/O | Cảm biến nhiệt độ/độ ẩm |
| Button 1 | 36 | Input | SOS tai nạn |
| Button 2 | 37 | Input | SOS cứu hộ |
| Buzzer | 15 | Output | Còi báo |

---

## 🎯 KHUYẾN NGHỊ

1. **Breadboard/PCB:**
   - Dùng breadboard để test trước
   - Sau đó làm PCB để ổn định hơn

2. **Nguồn:**
   - Dùng nguồn ổn định, có tụ lọc
   - Thêm fuse bảo vệ nếu cần

3. **Dây nối:**
   - Dùng dây jumper chất lượng
   - Giữ dây ngắn để giảm nhiễu

4. **Vỏ bọc:**
   - Bọc vỏ để bảo vệ mạch
   - Để lỗ thông gió cho GPS và SIM 4G

5. **Test từng phần:**
   - Test từng module riêng trước
   - Sau đó tích hợp dần

---

**Chúc bạn kết nối thành công! 🚀**

