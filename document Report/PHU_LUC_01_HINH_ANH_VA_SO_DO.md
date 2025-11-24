# PHỤ LỤC 1 - HÌNH ẢNH VÀ SƠ ĐỒ

## 📸 HÌNH ẢNH SẢN PHẨM

### Hình 1.1 - Sản phẩm hoàn thiện
**Mô tả:** Thiết bị hộp đen thông minh đã được lắp ráp hoàn chỉnh trong vỏ nhựa PLA.

**Vị trí file:** `images/san_pham_hoan_thien.jpg`

**Đặc điểm:**
- Kích thước: Dài 14.0 cm × Rộng 12.0 cm × Cao 7.0 cm
- Vỏ nhựa PLA được in 3D
- Nhỏ gọn, dễ lắp đặt trên xe máy/ô tô

---

### Hình 1.2 - Cấu tạo bên trong
**Mô tả:** Mạch điều khiển trung tâm và các linh kiện bên trong hộp đen.

**Vị trí file:** `images/cau_tao_ben_trong.jpg`

**Các thành phần hiển thị:**
- ESP32-S3 (MCU chính)
- Module SIM 4G (SIM7600)
- Module GPS NEO-8M
- Màn hình OLED 0.96"
- Các cảm biến và module khác

---

### Hình 1.3 - Sản phẩm lắp đặt trên xe máy
**Mô tả:** Thiết bị hộp đen được lắp đặt trên xe máy, kết nối với nguồn điện 12V.

**Vị trí file:** `images/lap_dat_tren_xe.jpg`

---

## 🔌 SƠ ĐỒ KHỐI HỆ THỐNG

### Hình 2.1 - Sơ đồ khối tổng quan hệ thống
**Mô tả:** Sơ đồ khối mô tả tổng quan về hệ thống hộp đen thông minh.

**Vị trí file:** `images/so_do_khoi_tong_quan.png` hoặc `images/so_do_khoi_tong_quan.svg`

**Các khối chính:**
```
┌─────────────────────────────────────────────────────┐
│           THIẾT BỊ HỘP ĐEN (ESP32-S3)               │
│                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────┐  │
│  │  CẢM BIẾN    │  │  XỬ LÝ       │  │  HIỂN THỊ│  │
│  │  - MPU9250   │→ │  - ESP32-S3  │→ │  - OLED  │  │
│  │  - GPS       │  │  - FreeRTOS  │  │  - Buzzer│  │
│  │  - DHT11     │  │              │  │          │  │
│  │  - DS1307    │  │  ┌────────┐  │  │          │  │
│  └──────────────┘  │  │  LƯU   │  │  │          │  │
│                    │  │  TRỮ   │  │  │          │  │
│                    │  │  - SD   │  │  │          │  │
│                    │  └────────┘  │  │          │  │
│                    └──────────────┘  └──────────┘  │
│                             │                       │
│                             ↓                       │
│                    ┌─────────────────┐             │
│                    │  TRUYỀN DỮ LIỆU│             │
│                    │  - SIM 4G       │             │
│                    │  - HTTP         │             │
│                    │  - SMS/Call     │             │
│                    └─────────────────┘             │
└─────────────────────────────────────────────────────┘
                            │
                            ↓
┌─────────────────────────────────────────────────────┐
│           SERVER WEB TRUNG TÂM                      │
│  - Backend API                                       │
│  - Database                                          │
│  - WebSocket Real-time                               │
│  - Điều phối SOS                                     │
└─────────────────────────────────────────────────────┘
                            │
                            ↓
┌─────────────────────────────────────────────────────┐
│           NGƯỜI DÙNG                                │
│  - Web Application                                   │
│  - Mobile App                                        │
│  - Trạm y tế / Cứu hộ                               │
└─────────────────────────────────────────────────────┘
```

---

### Hình 2.2 - Sơ đồ kết nối linh kiện với ESP32-S3
**Mô tả:** Sơ đồ chi tiết kết nối các linh kiện với ESP32-S3.

**Vị trí file:** `images/so_do_ket_noi_linh_kien.png` hoặc `images/so_do_ket_noi_linh_kien.svg`

**Thông tin chi tiết:** Xem file `hardware/SO_DO_KET_NOI_ESP32_S3.md` trong thư mục dự án.

---

## 📊 SƠ ĐỒ THUẬT TOÁN

### Hình 3.1 - Sơ đồ thuật toán hộp đen (Main Loop)
**Mô tả:** Sơ đồ thuật toán chính của thiết bị hộp đen.

**Vị trí file:** `images/so_do_thuat_toan_hop_den.png` hoặc `images/so_do_thuat_toan_hop_den.svg`

**Mô tả thuật toán:**
```
Bắt đầu
  ↓
Khởi tạo hệ thống
  - Init I2C, SPI, UART
  - Init cảm biến (MPU9250, GPS, DHT11, DS1307)
  - Init SD Card
  - Init SIM 4G
  - Init OLED
  - Load cấu hình từ EEPROM
  ↓
Tạo FreeRTOS Tasks
  - Task Sensors (Core 1)
  - Task SIM4G (Core 1)
  - Task OLED (Core 1)
  - Task SD Log (Core 0)
  ↓
Loop chính (FreeRTOS scheduler)
  ↓
[Kết thúc - Chạy vô hạn]
```

---

### Hình 3.2 - Sơ đồ thuật toán ESP gửi dữ liệu (Telemetry)
**Mô tả:** Sơ đồ thuật toán gửi dữ liệu telemetry lên server.

**Vị trí file:** `images/so_do_thuat_toan_esp_gui_du_lieu.png` hoặc `images/so_do_thuat_toan_esp_gui_du_lieu.svg`

**Mô tả thuật toán:**
```
Task SIM4G chạy mỗi 30 giây
  ↓
Kiểm tra kết nối SIM 4G
  - SIM đã khởi tạo? → Nếu chưa: Khởi tạo lại
  - Network đã mở? → Nếu chưa: Mở network
  ↓
Đọc dữ liệu cảm biến từ SystemState
  - GPS (lat, lon, speed)
  - Thời gian (từ DS1307)
  - Trạng thái SOS (nếu có)
  ↓
Tạo JSON payload
  {
    "userId": "...",
    "deviceId": "...",
    "lat": ...,
    "lon": ...,
    "speed": ...,
    "timestamp": "...",
    "sosStatus": "none|accident|rescue"
  }
  ↓
Gửi HTTP POST đến /api/telemetry
  - Retry 3 lần nếu lỗi
  - Timeout 10 giây
  ↓
Lưu log vào SD Card
  ↓
Chờ 30 giây → Lặp lại
```

---

### Hình 3.3 - Sơ đồ thuật toán tìm trạm gần nhất
**Mô tả:** Sơ đồ thuật toán tìm trạm y tế/cứu hộ gần nhất trên server.

**Vị trí file:** `images/so_do_thuat_toan_tim_tram_gan_nhat.png` hoặc `images/so_do_thuat_toan_tim_tram_gan_nhat.svg`

**Mô tả thuật toán:**
```
Nhận SOS mới từ thiết bị/người dùng
  ↓
Xác định loại SOS
  - Tai nạn / Y tế → Tìm trạm y tế
  - Hỏng xe / Khác → Tìm trạm cứu hộ
  ↓
Lấy danh sách trạm phù hợp từ database
  ↓
Tính khoảng cách từ vị trí SOS đến từng trạm
  - Sử dụng công thức Haversine
  - Khoảng cách = haversine(lat1, lon1, lat2, lon2)
  ↓
Sắp xếp trạm theo khoảng cách (gần → xa)
  ↓
Ưu tiên trạm có trạng thái "sẵn sàng" (ready = true)
  ↓
Chọn trạm gần nhất và sẵn sàng
  ↓
Gán SOS cho trạm
  - Cập nhật database
  - Gửi WebSocket notification đến trạm
  ↓
Nếu trạm từ chối → Chọn trạm tiếp theo
  ↓
Hoàn thành
```

---

### Hình 3.4 - Sơ đồ thuật toán điều phối SOS tự động
**Mô tả:** Sơ đồ thuật toán điều phối SOS tự động khi phát hiện tai nạn.

**Vị trí file:** `images/so_do_thuat_toan_dieu_phoi_sos_tu_dong.png` hoặc `images/so_do_thuat_toan_dieu_phoi_sos_tu_dong.svg`

**Mô tả thuật toán:**
```
Task Sensors đọc MPU9250 mỗi 100-200ms
  ↓
Tính gia tốc tuyến tính
  accelMagnitude = √(ax² + ay² + az²)
  ↓
Tính gia tốc góc
  angularAccelMagnitude = |Δ(gx, gy, gz)| / Δt
  ↓
Kiểm tra ngưỡng va chạm
  - accelMagnitude > 2.5g HOẶC
  - angularAccelMagnitude > 300 deg/s²
  ↓
Đọc tốc độ từ GPS
  currentSpeed = gps.speed.kmph()
  ↓
Kiểm tra tốc độ tối thiểu
  currentSpeed >= 10 km/h?
  ↓
CÓ → Phát hiện va chạm
  - Chuyển sang STATUS_ACCIDENT_COUNTDOWN
  - Bật buzzer bíp
  - Hiển thị đếm ngược 30s trên OLED
  ↓
Đếm ngược 30 giây
  - Buzzer bíp liên tục
  - Người dùng có thể nhấn Button 1 để hủy
  ↓
Hủy?
  CÓ → Trở về STATUS_NORMAL
  KHÔNG → Tiếp tục đếm
  ↓
Sau 30 giây
  - Gửi SOS lên server (HTTP POST /api/sos)
  - Gửi SMS đến số SOS
  - Gọi điện tự động
  - Lưu log vào SD Card
  - Chuyển sang STATUS_ACCIDENT_SOS_SENT
  ↓
Đồng bộ với server (mỗi 5 giây)
  - GET /api/sos/{sosId}
  - Nếu status = "done" hoặc "cancelled" → Hủy SOS
```

---

### Hình 3.5 - Sơ đồ thuật toán xử lý SOS của trạm
**Mô tả:** Sơ đồ thuật toán xử lý SOS trên phía trạm y tế/cứu hộ.

**Vị trí file:** `images/so_do_thuat_toan_xu_ly_sos_tram.png` hoặc `images/so_do_thuat_toan_xu_ly_sos_tram.svg`

**Mô tả thuật toán:**
```
Trạm đăng nhập vào hệ thống
  ↓
Nhận SOS mới qua WebSocket
  - Hiển thị trên bản đồ
  - Hiển thị thông tin chi tiết
  ↓
Trạm quyết định
  ↓
┌─────────────────┬─────────────────┐
│   NHẬN NHIỆM VỤ │   TỪ CHỐI       │
└─────────────────┴─────────────────┘
         │                  │
         ↓                  ↓
   Claim SOS          Chuyển sang trạm khác
   status = "claimed"     ↓
         │            Server tự động tìm
         │            trạm tiếp theo
         ↓
   Cập nhật trạng thái
   "Đã nhận" → "Đang đi" → "Hoàn thành"
         │
         ↓
   Người dùng nhận thông báo
   real-time qua WebSocket
         │
         ↓
   Hoàn thành nhiệm vụ
   status = "done"
         │
         ↓
   Thiết bị hộp đen nhận thông báo
   → Hủy SOS tự động
```

---

## 📋 LƯU Ý VỀ HÌNH ẢNH

**Thư mục hình ảnh:**
- Tạo thư mục `document Report/images/` để chứa tất cả hình ảnh
- Tên file nên theo chuẩn: `hinh_1.1_san_pham_hoan_thien.jpg`
- Định dạng khuyến nghị: PNG hoặc JPG cho ảnh, SVG cho sơ đồ

**Cách chèn hình ảnh vào Markdown:**
```markdown
![Mô tả hình ảnh](images/ten_file_hinh.jpg)
```

**Cách chèn sơ đồ:**
- Sơ đồ có thể vẽ bằng:
  - Draw.io / diagrams.net
  - Microsoft Visio
  - Inkscape
  - Công cụ vẽ online khác
- Xuất ra định dạng PNG hoặc SVG
- Chèn vào file Markdown như hình ảnh thông thường

---

**Ngày cập nhật:** 2025-01-XX
**Tác giả:** Nhóm nghiên cứu
**Phiên bản:** 1.0

