# PHỤ LỤC 4 - ĐỒNG BỘ THÔNG TIN PHẦN CỨNG

## 📋 MỤC ĐÍCH

File này so sánh và đồng bộ thông tin phần cứng giữa **báo cáo** và **code thực tế** để đảm bảo tính chính xác.

---

## 1. SO SÁNH LINH KIỆN

### 1.1. SIM Module

| Thông tin | Báo cáo | Code thực tế | Đồng bộ |
|-----------|---------|--------------|---------|
| Tên module | **SIM7682** | **SIM7600** | ⚠️ **KHÁC** |
| File code | - | `#define TINY_GSM_MODEM_SIM7600` | Cần cập nhật báo cáo |

**Khuyến nghị:**
- ✅ Code sử dụng **SIM7600** (hoặc tương thích)
- ✅ Cập nhật báo cáo: "SIM7600 (hoặc tương tự)" hoặc "SIM7682/SIM7600"

---

### 1.2. RTC Module

| Thông tin | Báo cáo | Code thực tế | Đồng bộ |
|-----------|---------|--------------|---------|
| Tên module | **DS3231** hoặc **NTP** | **DS1307** | ⚠️ **KHÁC** |
| File code | - | `#include <RTClib.h>`<br>`RTC_DS1307 rtc;` | Cần cập nhật báo cáo |

**Khuyến nghị:**
- ✅ Code sử dụng **DS1307**
- ✅ Cập nhật báo cáo: "DS1307" (thay vì DS3231)

---

### 1.3. Các linh kiện khác

| Linh kiện | Báo cáo | Code thực tế | Đồng bộ |
|-----------|---------|--------------|---------|
| ESP32-S3 | ESP32-S3 | ESP32-S3 | ✅ Đúng |
| MPU9250 | MPU9250 | MPU9250 | ✅ Đúng |
| GPS NEO-8M | GPS NEO-8M | GPS NEO-8M | ✅ Đúng |
| OLED 0.96" | OLED 0.96" | OLED SSD1306 0.96" | ✅ Đúng |
| DHT11 | DHT11 | DHT11 | ✅ Đúng |
| SD Card | SD 64GB | SD Card (không chỉ định dung lượng) | ⚠️ Cần xác nhận |
| INMP441 | INMP441 | INMP441 (tạm thời bỏ) | ⚠️ **Tạm thời bỏ** |

**Lưu ý về INMP441:**
- ✅ Code hiện tại **đã tạm thời bỏ ghi âm**
- ✅ Báo cáo nên ghi chú: "Ghi âm: Tạm thời bỏ, dự kiến tích hợp sau"

---

## 2. SO SÁNH CHÂN GPIO

### 2.1. Chân GPIO chính

| Chức năng | Báo cáo | Code thực tế | Đồng bộ |
|-----------|---------|--------------|---------|
| I2C SDA | - | GPIO 8 | ✅ |
| I2C SCL | - | GPIO 9 | ✅ |
| GPS RX | - | GPIO 6 | ✅ |
| GPS TX | - | GPIO 7 | ✅ |
| SIM RX | - | GPIO 18 | ✅ |
| SIM TX | - | GPIO 17 | ✅ |
| SD MOSI | - | GPIO 11 | ✅ |
| SD MISO | - | GPIO 13 | ✅ |
| SD SCK | - | GPIO 12 | ✅ |
| SD CS | - | GPIO 10 | ✅ |
| DHT11 | - | GPIO 35 | ✅ |
| Button 1 | - | GPIO 36 | ✅ |
| Button 2 | - | GPIO 37 | ✅ |
| Buzzer | - | GPIO 15 | ✅ |

**Kết luận:** ✅ Tất cả chân GPIO đều khớp với code.

**Tham khảo:** File `hardware/SO_DO_KET_NOI_ESP32_S3.md`

---

## 3. SO SÁNH THÔNG SỐ KỸ THUẬT

### 3.1. Thông số chuyển động (MPU9250)

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Ngưỡng gia tốc tuyến tính | 2-3g (va chạm nhẹ)<br>3-5g (va chạm mạnh) | **2.5g** (mặc định) | ⚠️ Cần cập nhật |
| Ngưỡng gia tốc góc | - | **300 deg/s²** | ✅ Mới thêm |
| Tốc độ tối thiểu | - | **10 km/h** | ✅ Mới thêm |
| Tần suất lấy mẫu | 100-200ms | 100-200ms | ✅ Đúng |

**Khuyến nghị:**
- ✅ Báo cáo nên ghi: "Ngưỡng gia tốc tuyến tính: 2.5g (mặc định, có thể điều chỉnh 1.5g - 5.0g)"
- ✅ Thêm: "Kết hợp với gia tốc góc (300 deg/s²) và tốc độ tối thiểu (10 km/h) để loại bỏ false positive"

---

### 3.2. Thông số GPS

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Vĩ độ, Kinh độ | ✅ | ✅ | ✅ Đúng |
| Tốc độ | ✅ | ✅ | ✅ Đúng |
| Tần suất cập nhật | - | Mỗi 30 giây (telemetry) | ✅ |

---

### 3.3. Thông số thời gian thực

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Module | DS3231 hoặc NTP | **DS1307** | ⚠️ Cần cập nhật |
| Đồng bộ tự động | Nếu có internet | Không (chỉ dùng DS1307) | ⚠️ Cần cập nhật |

**Khuyến nghị:**
- ✅ Cập nhật báo cáo: "DS1307 RTC" (thay vì DS3231/NTP)

---

### 3.4. Thông số cảnh báo

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Ngưỡng nhiệt độ | 50°C | 50°C | ✅ Đúng |
| Ngưỡng độ ẩm | 90% RH | 90% RH | ✅ Đúng |

---

### 3.5. Thông số lưu trữ SD Card

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Chu kỳ ghi | 0.1s - 0.2s - 1s | **30 giây** | ⚠️ Cần cập nhật |
| Tên file | DATA_dd-mm-yyyy.csv | **LOG_YYYYMMDD.csv** | ⚠️ Cần cập nhật |
| Dung lượng | - | Tùy SD Card (8GB-32GB) | ✅ |

**Khuyến nghị:**
- ✅ Cập nhật báo cáo: "Chu kỳ ghi: 30 giây"
- ✅ Cập nhật: "Tên file: LOG_YYYYMMDD.csv"

---

## 4. SO SÁNH CHỨC NĂNG

### 4.1. Chức năng đã thực hiện

| Chức năng | Báo cáo | Code thực tế | Đồng bộ |
|-----------|---------|--------------|---------|
| Ghi nhật ký | ✅ | ✅ | ✅ Đúng |
| Ghi âm | ✅ | ❌ **Tạm thời bỏ** | ⚠️ Cần cập nhật |
| Phát hiện va chạm | ✅ | ✅ | ✅ Đúng |
| Đếm ngược 30s | ✅ | ✅ | ✅ Đúng |
| Gửi SOS HTTP | ✅ | ✅ | ✅ Đúng |
| Gửi SMS | ✅ | ✅ | ✅ Đúng |
| Gọi điện | ✅ | ✅ | ✅ Đúng |
| Hiển thị OLED | ✅ | ✅ | ✅ Đúng |
| Buzzer | ✅ | ✅ | ✅ Đúng |
| Webserver cấu hình | ✅ | ❌ **Đã xóa** | ⚠️ Cần cập nhật |

**Khuyến nghị:**
- ✅ Cập nhật báo cáo: "Webserver cấu hình: Đã xóa, không còn chức năng này"
- ✅ Cập nhật: "Ghi âm: Tạm thời bỏ, dự kiến tích hợp sau"

---

### 4.2. Chức năng mới (không có trong báo cáo ban đầu)

| Chức năng | Báo cáo | Code thực tế | Ghi chú |
|-----------|---------|--------------|---------|
| Đồng bộ 2 chiều với server | ❌ | ✅ | ✅ Cần thêm vào báo cáo |
| Kết hợp gia tốc góc | ❌ | ✅ | ✅ Cần thêm vào báo cáo |
| Kiểm tra tốc độ tối thiểu | ❌ | ✅ | ✅ Cần thêm vào báo cáo |
| Gửi telemetry định kỳ | ❌ | ✅ | ✅ Cần thêm vào báo cáo |
| GPS fallback (dùng GPS cuối cùng) | ❌ | ✅ | ✅ Cần thêm vào báo cáo |

---

## 5. SO SÁNH CẤU HÌNH

### 5.1. Cấu hình mặc định

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Device ID | - | **DHW001** | ✅ |
| User ID | - | **U0001** | ✅ |
| Accident threshold | 2-5g | **2.5g** | ⚠️ Cần cập nhật |
| Angular accel threshold | - | **300 deg/s²** | ✅ Cần thêm |
| Min speed for accident | - | **10 km/h** | ✅ Cần thêm |
| Temp threshold | 50°C | 50°C | ✅ Đúng |
| Humi threshold | 90% | 90% | ✅ Đúng |
| SOS phone | - | **0964380284** | ✅ |

---

### 5.2. Cấu hình server

| Thông số | Báo cáo | Code thực tế | Đồng bộ |
|----------|---------|--------------|---------|
| Server URL | - | **hopdenthongminh.cloud** | ✅ |
| Telemetry endpoint | - | `/api/telemetry` | ✅ |
| SOS endpoint | - | `/api/sos` | ✅ |
| Telemetry interval | - | **30 giây** | ✅ |
| SOS sync interval | - | **5 giây** | ✅ |

---

## 6. DANH SÁCH CẦN CẬP NHẬT BÁO CÁO

### 6.1. Cần sửa (thông tin sai)

1. ✅ **SIM Module:** SIM7682 → SIM7600 (hoặc "SIM7682/SIM7600")
2. ✅ **RTC Module:** DS3231/NTP → DS1307
3. ✅ **Ngưỡng gia tốc:** 2-5g → 2.5g (mặc định, có thể điều chỉnh 1.5g - 5.0g)
4. ✅ **Chu kỳ ghi SD:** 0.1s-1s → 30 giây
5. ✅ **Tên file log:** DATA_dd-mm-yyyy.csv → LOG_YYYYMMDD.csv

---

### 6.2. Cần thêm (chức năng mới)

1. ✅ **Đồng bộ 2 chiều với server:**
   - Thiết bị gửi SOS → Server nhận → Trạm xử lý → Server cập nhật → Thiết bị nhận và hủy SOS
   - Kiểm tra trạng thái SOS mỗi 5 giây

2. ✅ **Kết hợp gia tốc góc:**
   - Phát hiện va chạm dựa trên: Gia tốc tuyến tính HOẶC Gia tốc góc
   - Ngưỡng gia tốc góc: 300 deg/s²

3. ✅ **Kiểm tra tốc độ tối thiểu:**
   - Chỉ phát hiện va chạm khi tốc độ ≥ 10 km/h
   - Loại bỏ false positive khi xe đứng yên

4. ✅ **Gửi telemetry định kỳ:**
   - Gửi GPS + trạng thái lên server mỗi 30 giây
   - Endpoint: `/api/telemetry`

5. ✅ **GPS fallback:**
   - Nếu GPS không valid, dùng GPS cuối cùng (không gửi 0,0)

6. ✅ **Webserver cấu hình: Đã xóa**
   - Không còn chức năng webserver local để cấu hình
   - Cấu hình được lưu trong EEPROM

7. ✅ **Ghi âm: Tạm thời bỏ**
   - Code đã comment out toàn bộ phần ghi âm
   - Dự kiến tích hợp sau

---

## 7. KHUYẾN NGHỊ

### 7.1. Cập nhật báo cáo

✅ **Thêm vào mục 6.2.7 (Cài đặt các thông số):**

```
a. Thông số chuyển động (từ cảm biến MPU9250)
- Gia tốc 3 trục: Ax, Ay, Az
- Gia tốc tổng: √(Ax² + Ay² + Az²)
- Gia tốc góc (con quay hồi chuyển): Gx, Gy, Gz
- Ngưỡng phát hiện va chạm:
  • Gia tốc tuyến tính: 2.5g (mặc định, có thể điều chỉnh 1.5g - 5.0g)
  • Gia tốc góc: 300 deg/s² (mặc định)
  • Tốc độ tối thiểu: 10 km/h (mặc định)
  • Va chạm được phát hiện khi: Tốc độ ≥ 10 km/h VÀ (Gia tốc > 2.5g HOẶC Gia tốc góc > 300 deg/s²)
- Tần suất lấy mẫu: 100–200 ms
```

✅ **Sửa mục 6.2.7 (Cài đặt lưu trữ trên thẻ SD):**

```
e. Cài đặt lưu trữ trên thẻ SD
- Chu kỳ ghi: 30 giây
- Tên file dạng: LOG_YYYYMMDD.csv (ví dụ: LOG_20250115.csv)
- Dung lượng tối đa: Tùy SD Card (8GB-32GB, khuyến nghị 8GB cho 1 tháng)
```

✅ **Sửa mục 4.3 (Phương pháp thu thập thông tin):**
- DS3231 → DS1307
- SIM7682 → SIM7600 (hoặc "SIM7682/SIM7600")

✅ **Thêm vào mục 6.3 (Cơ chế hoạt động):**
- Mô tả chi tiết về đồng bộ 2 chiều với server
- Mô tả về gửi telemetry định kỳ

---

### 7.2. Thêm vào Phụ lục 3 (Linh kiện)

| TT | Tên mã | Báo cáo | Code thực tế | Cập nhật |
|----|--------|---------|--------------|----------|
| 10 | SIM7682 | SIM7682 | SIM7600 | SIM7682/SIM7600 |
| 6 | RTC module | DS3231 | DS1307 | DS1307 |

---

## 8. TÓM TẮT

✅ **Đã đồng bộ:**
- Chân GPIO
- Thông số cảnh báo (nhiệt độ, độ ẩm)
- Chức năng chính (phát hiện va chạm, gửi SOS, v.v.)

⚠️ **Cần cập nhật báo cáo:**
- SIM Module: SIM7682 → SIM7600
- RTC: DS3231 → DS1307
- Ngưỡng gia tốc: Chi tiết hơn
- Chu kỳ ghi SD: 30 giây
- Tên file log: LOG_YYYYMMDD.csv

✅ **Cần thêm vào báo cáo:**
- Đồng bộ 2 chiều với server
- Kết hợp gia tốc góc
- Kiểm tra tốc độ tối thiểu
- Gửi telemetry định kỳ
- Webserver: Đã xóa
- Ghi âm: Tạm thời bỏ

---

**Ngày cập nhật:** 2025-01-XX
**Tác giả:** Nhóm nghiên cứu
**Phiên bản:** 1.0

