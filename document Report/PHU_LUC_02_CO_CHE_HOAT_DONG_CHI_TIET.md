# PHỤ LỤC 2 - CƠ CHẾ HOẠT ĐỘNG CHI TIẾT

## 🔄 CƠ CHẾ HOẠT ĐỘNG TỔNG QUAN

Hệ thống hộp đen thông minh hoạt động theo cơ chế **2 chiều** (Bị động và Chủ động), kết hợp với **FreeRTOS multi-tasking** để xử lý đồng thời nhiều nhiệm vụ.

---

## 1. PHẦN BỊ ĐỘNG (Tự động phát hiện tai nạn)

### 1.1. Khởi động hệ thống

**Quy trình khởi động:**

```
1. Cấp nguồn cho thiết bị
   ↓
2. ESP32-S3 khởi động
   - Load firmware từ Flash
   - Khởi tạo FreeRTOS scheduler
   ↓
3. Khởi tạo phần cứng
   - I2C bus (MPU9250, DS1307, OLED)
   - SPI bus (SD Card)
   - UART (GPS, SIM 4G)
   - GPIO (Buttons, Buzzer, DHT11)
   ↓
4. Kiểm tra và khởi tạo cảm biến
   - MPU9250: Kiểm tra WHO_AM_I (0x71)
   - GPS: Chờ tín hiệu vệ tinh
   - DHT11: Đọc nhiệt độ/độ ẩm test
   - DS1307: Đọc thời gian thực
   ↓
5. Khởi tạo SIM 4G
   - AT+CPIN? (Kiểm tra SIM card)
   - AT+CREG? (Đăng ký mạng)
   - AT+CGATT=1 (Kết nối GPRS)
   ↓
6. Load cấu hình từ EEPROM
   - Device ID, User ID
   - Ngưỡng cảm biến
   - Số điện thoại SOS
   ↓
7. Tạo FreeRTOS Tasks
   - Task Sensors (Core 1, priority 5)
   - Task SIM4G (Core 1, priority 3)
   - Task OLED (Core 1, priority 2)
   - Task SD Log (Core 0, priority 4)
   ↓
8. Bắt đầu vận hành
   - Task Sensors: Đọc cảm biến liên tục
   - Task SIM4G: Gửi telemetry mỗi 30s
   - Task OLED: Cập nhật màn hình mỗi 100ms
   - Task SD Log: Ghi log mỗi 30s
```

---

### 1.2. Phát hiện va chạm/tai nạn tự động

**Quy trình phát hiện:**

```
Task Sensors chạy mỗi 100-200ms
  ↓
Đọc dữ liệu MPU9250 qua I2C
  - Gia tốc 3 trục (ax, ay, az)
  - Góc quay 3 trục (gx, gy, gz)
  ↓
Tính gia tốc tuyến tính
  accelMagnitude = √(ax² + ay² + az²)
  Đơn vị: g (1g = 9.8 m/s²)
  ↓
Tính gia tốc góc (từ gyroscope)
  - Lưu giá trị trước: lastGyroX, lastGyroY, lastGyroZ, lastGyroTime
  - Tính delta: Δgx = gx - lastGyroX
                Δgy = gy - lastGyroY
                Δgz = gz - lastGyroZ
                Δt = currentTime - lastGyroTime
  - Tính magnitude: angularAccelMagnitude = √(Δgx² + Δgy² + Δgz²) / Δt
  Đơn vị: deg/s²
  ↓
Đọc tốc độ từ GPS
  currentSpeed = gps.speed.kmph()
  Nếu không có → Tính từ 2 điểm GPS liên tiếp
  ↓
Kiểm tra điều kiện va chạm:
  ĐIỀU KIỆN 1: currentSpeed >= 10 km/h
  ĐIỀU KIỆN 2: accelMagnitude > 2.5g HOẶC angularAccelMagnitude > 300 deg/s²
  ↓
CẢ 2 ĐIỀU KIỆN ĐÚNG?
  ├─ CÓ → Phát hiện va chạm
  │        ↓
  │   Chuyển sang STATUS_ACCIDENT_COUNTDOWN
  │   - Lưu thời gian bắt đầu: countdownStart = millis()
  │   - countdownRemaining = 30 giây
  │   - Bật buzzer bíp
  │   - Hiển thị đếm ngược trên OLED
  │   - Lưu log vào SD Card: "[ACCIDENT] Detected - Speed: X km/h, Accel: Y g"
  │
  └─ KHÔNG → Tiếp tục đọc cảm biến
     (Nếu chỉ có điều kiện 2 mà không có điều kiện 1 → Bỏ qua)
     Lưu log: "[ACCIDENT] Ignored - Speed too low: X km/h"
```

**Ngưỡng cấu hình:**
- **Gia tốc tuyến tính:** 2.5g (mặc định), có thể điều chỉnh 1.5g - 5.0g
- **Gia tốc góc:** 300 deg/s² (mặc định)
- **Tốc độ tối thiểu:** 10 km/h (mặc định)

---

### 1.3. Đếm ngược và cảnh báo

**Quy trình đếm ngược 30 giây:**

```
Trạng thái: STATUS_ACCIDENT_COUNTDOWN
  ↓
Mỗi lần Task Sensors chạy (100-200ms):
  1. Tính thời gian còn lại
     countdownRemaining = 30 - (millis() - countdownStart) / 1000
  2. Cập nhật buzzer
     - Nếu countdownRemaining > 10s: Bíp mỗi 200ms
     - Nếu 5s < countdownRemaining <= 10s: Bíp mỗi 100ms
     - Nếu countdownRemaining <= 5s: Bíp mỗi 50ms
  3. Cập nhật OLED
     - Hiển thị "ACCIDENT!"
     - Hiển thị đếm ngược: "00:XX"
     - Hiển thị "Press B1 to Cancel"
  4. Kiểm tra Button 1
     - Nếu nhấn giữ 5 giây → Hủy SOS
  5. Kiểm tra thời gian
     - Nếu countdownRemaining <= 0 → Gửi SOS
```

**Hủy SOS:**

```
Người dùng nhấn Button 1 (GPIO 36)
  ↓
Debounce 50ms
  ↓
Nhấn giữ 5 giây?
  ├─ CÓ → Hủy SOS
  │        ↓
  │   Chuyển sang STATUS_NORMAL
  │   - Tắt buzzer
  │   - Xóa đếm ngược
  │   - Cập nhật OLED: "Normal"
  │   - Lưu log: "[ACCIDENT] Cancelled by user"
  │
  └─ KHÔNG → Tiếp tục đếm ngược
```

---

### 1.4. Gửi SOS tự động

**Quy trình gửi SOS:**

```
Sau 30 giây đếm ngược (không bị hủy)
  ↓
Chuyển sang STATUS_ACCIDENT_SOS_SENT
  ↓
Tạo JSON payload:
{
  "userId": "U0001",
  "deviceId": "DHW001",
  "type": "accident",
  "severity": "critical",
  "location": {
    "lat": ...,
    "lon": ...
  },
  "speed": ...,
  "accelMagnitude": ...,
  "note": "Auto-detected accident. Speed: X km/h, Accel: Y g",
  "source": "hardware"
}
  ↓
Gửi HTTP POST đến /api/sos
  - Server: hopdenthongminh.cloud
  - Endpoint: /api/sos
  - Retry 3 lần nếu lỗi
  - Timeout 10 giây
  ↓
Nhận response từ server
  - Parse JSON để lấy sosId
  - Lưu sosId vào systemState.currentSOSId
  ↓
Gửi SMS đến số điện thoại SOS
  - AT+CMGS="0964380284"
  - Nội dung: "SOS Tai nạn! Vị trí: lat,lon. Thời gian: ..."
  ↓
Gọi điện tự động
  - ATD0964380284;
  - Gọi trong 10 giây, sau đó ngắt
  ↓
Lưu log vào SD Card
  - "[SOS] Sent - Type: accident, ID: {sosId}, Time: ..."
  ↓
Bắt đầu đồng bộ với server (mỗi 5 giây)
```

---

### 1.5. Đồng bộ 2 chiều với server

**Quy trình đồng bộ:**

```
Task SIM4G chạy mỗi 5 giây (nếu có SOS active)
  ↓
Kiểm tra systemState.currentSOSId
  - Nếu có → Đồng bộ
  ↓
Gửi HTTP GET đến /api/sos/{sosId}
  - Server: hopdenthongminh.cloud
  - Endpoint: /api/sos/{sosId}
  - Timeout 10 giây
  ↓
Nhận response JSON:
{
  "id": "...",
  "status": "active|done|cancelled",
  ...
}
  ↓
Kiểm tra status:
  ├─ "done" → SOS đã hoàn thành
  │        ↓
  │   Chuyển sang STATUS_NORMAL
  │   - Tắt buzzer
  │   - Xóa sosId
  │   - Cập nhật OLED
  │   - Lưu log: "[SOS] Completed by server"
  │
  ├─ "cancelled" → SOS đã bị hủy
  │        ↓
  │   Chuyển sang STATUS_NORMAL
  │   - Tắt buzzer
  │   - Xóa sosId
  │   - Cập nhật OLED
  │   - Lưu log: "[SOS] Cancelled by server"
  │
  └─ "active" → SOS vẫn đang active
     → Tiếp tục đồng bộ (lặp lại sau 5 giây)
```

---

## 2. PHẦN CHỦ ĐỘNG (SOS cứu hộ thủ công)

### 2.1. Kích hoạt SOS cứu hộ

**Quy trình kích hoạt:**

```
Người dùng nhấn Button 2 (GPIO 37)
  ↓
Debounce 50ms
  ↓
Nhấn giữ 5 giây?
  ├─ CÓ → Kích hoạt SOS cứu hộ
  │        ↓
  │   Chuyển sang STATUS_RESCUE_COUNTDOWN
  │   - countdownStart = millis()
  │   - countdownRemaining = 30 giây
  │   - Bật buzzer bíp
  │   - Hiển thị đếm ngược trên OLED
  │   - Lưu log: "[RESCUE] Activated by user"
  │
  └─ KHÔNG → Bỏ qua
```

---

### 2.2. Đếm ngược và gửi SOS cứu hộ

**Quy trình tương tự SOS tai nạn:**

```
Trạng thái: STATUS_RESCUE_COUNTDOWN
  ↓
Đếm ngược 30 giây (tương tự SOS tai nạn)
  - Buzzer bíp
  - Hiển thị trên OLED
  - Có thể hủy bằng Button 2
  ↓
Sau 30 giây (không bị hủy)
  ↓
Chuyển sang STATUS_RESCUE_SOS_SENT
  ↓
Tạo JSON payload:
{
  "userId": "U0001",
  "deviceId": "DHW001",
  "type": "breakdown",
  "severity": "high",
  "location": {
    "lat": ...,
    "lon": ...
  },
  "speed": ...,
  "note": "Manual rescue request from hardware",
  "source": "hardware"
}
  ↓
Gửi HTTP POST đến /api/sos
  - Tương tự SOS tai nạn
  ↓
Gửi SMS và gọi điện
  ↓
Đồng bộ với server (mỗi 5 giây)
```

---

## 3. GỬI DỮ LIỆU TELEMETRY

### 3.1. Quy trình gửi telemetry định kỳ

```
Task SIM4G chạy mỗi 30 giây
  ↓
Kiểm tra kết nối SIM 4G
  - sim4gInitialized?
  - sim4gNetworkOpen?
  - modem.isGprsConnected()?
  ↓
Nếu chưa kết nối → Khởi tạo lại
  ↓
Đọc dữ liệu cảm biến từ SystemState
  - GPS (lat, lon, speed) - Ưu tiên GPS hiện tại, nếu không có → dùng GPS cuối cùng
  - Thời gian (từ DS1307)
  - Trạng thái SOS (none|accident|rescue)
  ↓
Tạo JSON payload:
{
  "userId": "U0001",
  "deviceId": "DHW001",
  "lat": ...,
  "lon": ...,
  "speed": ...,
  "timestamp": "2025-01-15T14:30:25",
  "source": "hardware",
  "sosStatus": "none|accident|rescue"
}
  ↓
Gửi HTTP POST đến /api/telemetry
  - Retry 3 lần nếu lỗi
  - Timeout 10 giây
  ↓
Nhận response (200 OK)
  ↓
Lưu log vào SD Card
  - "[TELEMETRY] Sent - Lat: X, Lon: Y, Speed: Z"
```

---

## 4. GHI NHẬT KÝ LIÊN TỤC

### 4.1. Quy trình ghi log

```
Task SD Log chạy mỗi 30 giây (Core 0)
  ↓
Kiểm tra SD Card
  - SD card đã mount?
  ↓
Tạo tên file log theo ngày
  - Format: LOG_YYYYMMDD.csv
  - Ví dụ: LOG_20250115.csv
  ↓
Mở file log (append mode)
  ↓
Đọc dữ liệu cảm biến
  - GPS (lat, lon, speed, valid)
  - MPU9250 (ax, ay, az, gx, gy, gz)
  - DHT11 (temp, humi)
  - DS1307 (timestamp)
  - Trạng thái SOS (nếu có)
  ↓
Ghi vào file CSV:
timestamp,lat,lon,speed,ax,ay,az,gx,gy,gz,temp,humi,sosStatus
2025-01-15 14:30:25,10.762622,106.660172,45.5,0.1,0.2,9.8,0.5,0.3,0.1,28.5,65.0,none
  ↓
Đóng file
  ↓
Chờ 30 giây → Lặp lại
```

---

## 5. HIỂN THỊ TRÊN OLED

### 5.1. Quy trình cập nhật màn hình

```
Task OLED chạy mỗi 100ms (Core 1)
  ↓
Kiểm tra trạng thái hệ thống
  ↓
┌─────────────────────────────────────┐
│ STATUS_NORMAL                        │
│  - Hiển thị thời gian (DS1307)      │
│  - Hiển thị GPS (lat, lon)          │
│  - Hiển thị tốc độ (km/h)           │
│  - Hiển thị nhiệt độ/độ ẩm          │
│  - Hiển thị trạng thái SIM 4G       │
├─────────────────────────────────────┤
│ STATUS_ACCIDENT_COUNTDOWN            │
│  - Hiển thị "ACCIDENT!"             │
│  - Hiển thị đếm ngược: "00:XX"      │
│  - Hiển thị "Press B1 to Cancel"    │
├─────────────────────────────────────┤
│ STATUS_ACCIDENT_SOS_SENT             │
│  - Hiển thị "SOS SENT!"             │
│  - Hiển thị trạng thái đồng bộ      │
├─────────────────────────────────────┤
│ STATUS_RESCUE_COUNTDOWN              │
│  - Hiển thị "RESCUE!"                │
│  - Hiển thị đếm ngược: "00:XX"      │
│  - Hiển thị "Press B2 to Cancel"    │
├─────────────────────────────────────┤
│ STATUS_RESCUE_SOS_SENT               │
│  - Hiển thị "SOS SENT!"             │
│  - Hiển thị trạng thái đồng bộ      │
└─────────────────────────────────────┘
  ↓
Vẽ lên OLED buffer
  ↓
Gửi buffer đến OLED qua I2C
  ↓
Chờ 100ms → Lặp lại
```

---

## 6. CẢNH BÁO NHIỆT ĐỘ/ĐỘ ẨM

### 6.1. Quy trình cảnh báo

```
Task Sensors đọc DHT11 mỗi 2 giây
  ↓
Đọc nhiệt độ và độ ẩm
  - temp = dht.readTemperature()
  - humi = dht.readHumidity()
  ↓
Kiểm tra ngưỡng:
  ├─ temp > 50°C HOẶC humi > 90%?
  │  ├─ CÓ (và không có SOS countdown) → Cảnh báo
  │  │        ↓
  │  │   Bật buzzer bíp (pattern khác SOS)
  │  │   - Bíp mỗi 500ms (chậm hơn SOS)
  │  │   - Hiển thị trên OLED: "TEMP HIGH!" hoặc "HUMI HIGH!"
  │  │   - Lưu log: "[ALERT] Temperature: X°C" hoặc "[ALERT] Humidity: X%"
  │  │
  │  └─ KHÔNG → Tắt buzzer (nếu đang bật do cảnh báo)
  │
  └─ Nếu có SOS countdown → Bỏ qua cảnh báo nhiệt độ/độ ẩm
     (Ưu tiên SOS)
```

---

## 7. XỬ LÝ LỖI VÀ PHỤC HỒI

### 7.1. Tự động phục hồi

**SIM 4G mất kết nối:**
```
Kiểm tra kết nối mỗi lần gửi dữ liệu
  ↓
Mất kết nối?
  ├─ CÓ → Khởi tạo lại SIM 4G
  │        - AT+CPIN?
  │        - AT+CREG?
  │        - AT+CGATT=1
  │        - Retry 3 lần
  │
  └─ KHÔNG → Tiếp tục
```

**GPS mất tín hiệu:**
```
Đọc GPS mỗi lần gửi telemetry
  ↓
GPS không valid?
  ├─ CÓ → Sử dụng GPS cuối cùng (displayedGPSLat/Lon)
  │        - Không gửi tọa độ 0,0
  │
  └─ KHÔNG → Sử dụng GPS hiện tại
```

**SD Card lỗi:**
```
Kiểm tra SD Card khi ghi log
  ↓
Lỗi?
  ├─ CÓ → Log vào Serial (Serial.println)
  │        - Retry mount SD Card
  │
  └─ KHÔNG → Ghi log bình thường
```

---

## 📊 TÓM TẮT QUY TRÌNH

1. **Khởi động:** Khởi tạo phần cứng, cảm biến, SIM 4G → Tạo FreeRTOS tasks
2. **Phát hiện va chạm:** MPU9250 → Kiểm tra ngưỡng + tốc độ → Đếm ngược 30s → Gửi SOS
3. **Gửi SOS:** HTTP POST → SMS → Gọi điện → Đồng bộ với server
4. **Gửi telemetry:** Mỗi 30s gửi GPS + trạng thái lên server
5. **Ghi log:** Mỗi 30s ghi dữ liệu cảm biến vào SD Card
6. **Hiển thị:** Mỗi 100ms cập nhật OLED với trạng thái hiện tại
7. **Đồng bộ:** Mỗi 5s kiểm tra trạng thái SOS trên server

---

**Ngày cập nhật:** 2025-01-XX
**Tác giả:** Nhóm nghiên cứu
**Phiên bản:** 1.0

