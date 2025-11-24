================================================================================
    HỘP ĐEN Ô TÔ / XE MÁY - ESP32-S3
    DANH SÁCH CHỨC NĂNG CHI TIẾT
================================================================================

📋 TỔNG QUAN
-----------
- MCU: ESP32-S3 (dual-core)
- Framework: Arduino + FreeRTOS
- Phân chia Core:
  • Core 0: Audio (INMP441) + SD Card (ghi âm, ghi log)
  • Core 1: Sensors + Logic (MPU, DHT11, GPS, DS1307, SIM 4G, nút, buzzer, OLED)

================================================================================
1. GHI NHẬT KÝ LIÊN TỤC (DATA LOGGING)
================================================================================

✓ Ghi dữ liệu cảm biến vào SD Card
  - Tần suất: Mỗi 30 giây
  - Định dạng: File CSV hoặc JSON
  - Dữ liệu ghi:
    • GPS: Vĩ độ (lat), Kinh độ (lon), Vận tốc (speed)
    • MPU9250: Gia tốc (ax, ay, az), Góc quay (gx, gy, gz)
    • DHT11: Nhiệt độ (temp), Độ ẩm (humi)
    • DS1307: Thời gian thực (timestamp)
    • Trạng thái SOS từ phần cứng (nếu có)
      - SOS tai nạn tự động (accident)
      - SOS cứu hộ thủ công (rescue)
      - Thời gian kích hoạt/hủy
      - Trạng thái: active, cancelled, sent
  
✓ Lưu trữ lâu dài
  - Dữ liệu được lưu trên SD Card
  - Có thể phân tích sau khi xảy ra sự cố
  - Hỗ trợ phân tích hành trình và tai nạn

✓ Tên file log
  - Tự động tạo file mới mỗi ngày
  - Định dạng: LOG_YYYYMMDD.csv
  - Dễ dàng truy xuất và phân tích

✓ Lưu ý quan trọng
  - CHỈ log trạng thái SOS từ phần cứng
  - KHÔNG log SOS được tạo từ web/app
  - Phân biệt rõ nguồn gốc SOS (hardware vs web)

================================================================================
2. GHI ÂM VÒNG TRÒN (CIRCULAR AUDIO RECORDING)
================================================================================

✓ Ghi âm liên tục
  - Sử dụng microphone INMP441 (I2S)
  - Sample rate: 16kHz
  - Bit depth: 16-bit
  - Mono channel

✓ Vòng tròn 50 file
  - Tổng cộng: 50 file audio
  - Mỗi file: 2 phút (120 giây)
  - Tổng thời gian ghi: 100 phút (1h40 phút)
  - Tự động ghi đè file cũ khi đầy

✓ Lưu trữ trên SD Card
  - Định dạng: WAV hoặc RAW
  - Tên file: Theo thời gian kết thúc đoạn ghi âm
  - Định dạng: AUDIO_YYYYMMDD_HHMMSS.wav
    Ví dụ: AUDIO_20250115_143025.wav
    (Ngày 15/01/2025, 14:30:25 - thời gian kết thúc)
  - Tự động quay vòng khi đầy 50 file

✓ Mục đích
  - Ghi lại âm thanh trước, trong và sau tai nạn
  - Hỗ trợ điều tra và phân tích
  - Bằng chứng quan trọng
  - Dễ dàng xác định thời điểm từ tên file

================================================================================
3. PHÁT HIỆN VA CHẠM / TAI NẠN TỰ ĐỘNG
================================================================================

✓ Sử dụng MPU9250
  - Cảm biến gia tốc 3 trục (ax, ay, az)
  - Độ nhạy cao, phản ứng nhanh

✓ Ngưỡng phát hiện
  - Ngưỡng gia tốc: 2.5g (g = 9.8 m/s²)
  - Tính toán: magnitude = √(ax² + ay² + az²)
  - Khi vượt ngưỡng → Phát hiện va chạm

✓ Đếm ngược 30 giây
  - Sau khi phát hiện va chạm
  - Đếm ngược: 30 giây
  - Buzzer bíp liên tục (tiếng bíp) để cảnh báo
  - Hiển thị trên OLED với đếm ngược rõ ràng
  - Mục đích: Tránh cảnh báo nhầm, cho phép người dùng hủy

✓ Hủy tự động
  - Nhấn giữ nút Button 1 trong 5 giây
  - Hủy đếm ngược và SOS
  - Buzzer dừng bíp
  - Trở về trạng thái bình thường

✓ Gửi SOS tự động
  - Sau 30 giây nếu không hủy
  - Tự động gửi SOS "Tai nạn"
  - Gửi qua SIM 4G (HTTP + SMS + Gọi điện)

================================================================================
4. CẢNH BÁO SOS (EMERGENCY ALERT)
================================================================================

4.1. SOS TAI NẠN (TỰ ĐỘNG)
----------------------------
✓ Phát hiện tự động từ MPU9250
✓ Đếm ngược 30 giây
✓ Gửi tự động nếu không hủy
✓ Loại: "accident"
✓ Mức độ: "critical"

4.2. SOS CỨU HỘ (THỦ CÔNG)
----------------------------
✓ Kích hoạt
  - Nhấn nút Button 2 (GPIO 37)
  - Nhấn giữ 5 giây để kích hoạt
  - Hiển thị trạng thái trên màn hình OLED

✓ Đếm ngược 30 giây
  - Sau khi kích hoạt
  - Hiển thị đếm ngược trên OLED
  - Buzzer bíp cảnh báo

✓ Hủy
  - Nhấn giữ nút Button 2 trong 5 giây để hủy
  - Hủy đếm ngược và SOS
  - Trở về trạng thái bình thường

✓ Log vào nhật ký
  - Ghi lại thời điểm kích hoạt
  - Ghi lại thời điểm hủy (nếu có)
  - Trạng thái: active, cancelled, sent
  - Lưu vào file log cùng với dữ liệu cảm biến

✓ Loại: "breakdown"
✓ Mức độ: "high"

4.3. THÔNG TIN GỬI KÈM
----------------------
✓ ID thiết bị (Device ID)
✓ ID người dùng (User ID)
✓ Vị trí GPS (lat, lon)
✓ Vận tốc tại thời điểm SOS
✓ Thời gian (từ DS1307)
✓ Trạng thái SOS/cứu hộ tại box
  - accident (tai nạn tự động)
  - rescue (cứu hộ thủ công)
  - active, cancelled, sent
✓ Ghi chú tự động
  - Tự động tạo từ thông tin cảm biến
  - Vận tốc, gia tốc, nhiệt độ, v.v.

4.4. KÊNH GỬI SOS
-----------------
✓ HTTP POST lên server backend
  - Endpoint: /api/sos
  - Đồng bộ với server
  - Server tự động liên hệ với trạm cứu hộ
✓ SMS đến số điện thoại cứu hộ
✓ Gọi điện tự động
✓ WebSocket notification (qua backend)

4.5. ĐỒNG BỘ 2 CHIỀU VỚI SERVER
--------------------------------
✓ Đồng bộ từ Box → Server
  - Khi box gửi SOS → Server nhận và tự động liên hệ trạm
  - Trạng thái SOS được cập nhật real-time

✓ Đồng bộ từ Server → Box
  - Khi server hủy SOS → Box tự động hủy trạng thái SOS
  - Khi trạm hoàn thành nhiệm vụ → Box tự động hủy trạng thái SOS
  - Đồng bộ qua HTTP polling hoặc WebSocket
  - Đảm bảo trạng thái nhất quán giữa box và server

================================================================================
5. GỬI SMS VÀ GỌI ĐIỆN QUA SIM 4G
================================================================================

✓ Kết nối SIM 4G
  - Module: SIM7600 hoặc tương tự
  - Giao thức: 4G/LTE
  - APN: m3-world (có thể thay đổi theo nhà mạng)

✓ Gửi SMS
  - Khi phát hiện tai nạn
  - Khi nhấn nút SOS cứu hộ
  - Nội dung: Vị trí GPS, vận tốc, thời gian
  - Đến số: SOS_PHONE_NUMBER

✓ Gọi điện tự động
  - Gọi ngay sau khi gửi SMS
  - Tự động kết nối
  - Cảnh báo khẩn cấp

✓ Kết nối Internet
  - Gửi dữ liệu lên server backend
  - HTTP POST requests
  - Real-time data sync

================================================================================
6. HIỂN THỊ TRẠNG THÁI TRÊN OLED
================================================================================

✓ Màn hình OLED SSD1306 0.96"
  - Độ phân giải: 128x64 pixels
  - Giao tiếp: I2C (address 0x3C)
  - Cập nhật: Mỗi 100ms

✓ Thông tin hiển thị
  - Trạng thái hệ thống
  - Thời gian (từ DS1307)
  - Vị trí GPS (lat, lon)
  - Vận tốc
  - Nhiệt độ / Độ ẩm
  - Đếm ngược SOS (nếu có)
  - Trạng thái kết nối SIM 4G

✓ Cảnh báo trực quan
  - Hiển thị khi phát hiện va chạm
  - Đếm ngược rõ ràng
  - Icon và text cảnh báo

================================================================================
7. THU THẬP DỮ LIỆU CẢM BIẾN
================================================================================

7.1. GPS NEO-8M
---------------
✓ Vị trí GPS
  - Vĩ độ (latitude)
  - Kinh độ (longitude)
  - Độ chính xác cao

✓ Vận tốc
  - Từ GPS (nếu có)
  - Hoặc tính từ thay đổi vị trí
  - Đơn vị: km/h

✓ Cập nhật: Liên tục

7.2. MPU9250 (9-Axis IMU)
--------------------------
✓ Gia tốc (Accelerometer)
  - Trục X, Y, Z
  - Phát hiện va chạm
  - Đơn vị: g (9.8 m/s²)

✓ Góc quay (Gyroscope)
  - Trục X, Y, Z
  - Phát hiện xoay, lật
  - Đơn vị: độ/s

✓ Từ trường (Magnetometer)
  - La bàn số
  - Hướng di chuyển

7.3. DHT11
----------
✓ Nhiệt độ
  - Phạm vi: 0-50°C
  - Độ chính xác: ±2°C
  - Ngưỡng cảnh báo: Có thể thiết lập (mặc định 50°C)
  - Khi vượt ngưỡng → Buzzer cảnh báo

✓ Độ ẩm
  - Phạm vi: 20-90% RH
  - Độ chính xác: ±5% RH
  - Ngưỡng cảnh báo: Có thể thiết lập (mặc định 90% RH)
  - Khi vượt ngưỡng → Buzzer cảnh báo

✓ Cảnh báo môi trường
  - Phát hiện xe quá nhiệt
  - Phát hiện độ ẩm quá cao
  - Buzzer bíp với pattern khác SOS
  - Hiển thị trên OLED

7.4. DS1307 RTC
---------------
✓ Thời gian thực
  - Ngày, giờ, phút, giây
  - Pin backup (CR2032)
  - Không mất thời gian khi mất điện

================================================================================
8. GỬI DỮ LIỆU TELEMETRY LÊN SERVER
================================================================================

✓ Gửi định kỳ
  - Tần suất: Mỗi 30 giây
  - Endpoint: /api/telemetry
  - Giao thức: HTTP POST

✓ Dữ liệu gửi
  - User ID
  - Device ID
  - Vị trí GPS (lat, lon)
  - Vận tốc
  - Timestamp
  - Source: "hardware"
  - Trạng thái SOS/cứu hộ tại box (nếu có)
    • accident: SOS tai nạn tự động đang active
    • rescue: SOS cứu hộ thủ công đang active
    • none: Không có SOS

✓ Kết nối
  - Qua SIM 4G
  - Tự động reconnect nếu mất kết nối
  - Xác thực (nếu cần)

✓ Mục đích
  - Theo dõi real-time trên web
  - Lưu trữ trên server
  - Phân tích và báo cáo

================================================================================
9. XỬ LÝ NÚT NHẤN (BUTTON INPUT)
================================================================================

9.1. BUTTON 1 (GPIO 36) - SOS TAI NẠN
---------------------------------------
✓ Chức năng
  - Hủy đếm ngược SOS tai nạn
  - Nhấn giữ 5 giây để hủy hoàn toàn
  - Cảnh báo thủ công (nếu cần)

9.2. BUTTON 2 (GPIO 37) - SOS CỨU HỘ
---------------------------------------
✓ Chức năng
  - Kích hoạt SOS cứu hộ: Nhấn giữ 5 giây
  - Đếm ngược 30 giây sau khi kích hoạt
  - Hủy: Nhấn giữ 5 giây khi đang đếm ngược
  - Log vào nhật ký khi kích hoạt/hủy

✓ Xử lý
  - Debounce để tránh nhấn nhầm
  - Phát hiện nhấn giữ (5 giây)
  - Phản hồi tức thì
  - Hiển thị trạng thái trên OLED

9.3. NHẤN ĐỒNG THỜI 2 NÚT (10 GIÂY) - CHẾ ĐỘ THIẾT LẬP
-------------------------------------------------------
✓ Kích hoạt
  - Nhấn đồng thời Button 1 + Button 2
  - Giữ trong 10 giây
  - Vào chế độ thiết lập (Config Mode)

✓ Webserver tự phát
  - ESP32 tạo Access Point (AP)
  - SSID: "HOPDEN_CONFIG_XXXX"
  - IP: 192.168.4.1
  - Mở trình duyệt: http://192.168.4.1

✓ Các thông số có thể thiết lập:
  • ID thiết bị (Device ID)
    - Để đồng bộ với tài khoản người dùng theo box
    - Ví dụ: DHW001, DHW002, ...
  
  • ID người dùng (User ID)
    - ID tài khoản người dùng trên hệ thống
    - Ví dụ: U0001, U0002, ...
  
  • Độ nhạy cảm biến gia tốc
    - Ngưỡng phát hiện tai nạn (g)
    - Mặc định: 2.5g
    - Có thể điều chỉnh: 1.5g - 5.0g
  
  • Ngưỡng nhiệt độ cảnh báo
    - Nhiệt độ tối đa cho phép (°C)
    - Mặc định: 50°C
    - Khi vượt ngưỡng → Buzzer cảnh báo
  
  • Ngưỡng độ ẩm cảnh báo
    - Độ ẩm tối đa cho phép (%)
    - Mặc định: 90% RH
    - Khi vượt ngưỡng → Buzzer cảnh báo
  
  • Số điện thoại SOS
    - Số điện thoại để gọi và nhắn tin khi gặp nạn
    - Ví dụ: 0335587155
    - Có thể thêm nhiều số (phân cách bằng dấu phẩy)

✓ Lưu cấu hình
  - Lưu vào EEPROM hoặc file trên SD Card
  - Tự động load khi khởi động lại
  - Có thể reset về mặc định

✓ Thoát chế độ thiết lập
  - Nhấn nút Reset hoặc restart ESP32
  - Hoặc nhấn nút "Lưu và Thoát" trên web

================================================================================
10. CẢNH BÁO BẰNG BUZZER
================================================================================

✓ Kích hoạt
  - Khi phát hiện va chạm (SOS tai nạn)
    • Buzzer bíp liên tục trong 30 giây đếm ngược
    • Pattern: Bíp ngắn, lặp lại
  - Trong thời gian đếm ngược SOS cứu hộ
    • Buzzer bíp cảnh báo
  - Cảnh báo nhiệt độ/độ ẩm vượt ngưỡng
    • Buzzer bíp khi nhiệt độ > ngưỡng
    • Buzzer bíp khi độ ẩm > ngưỡng
    • Pattern khác với SOS để phân biệt

✓ Điều khiển
  - GPIO 15
  - PWM hoặc Digital
  - Tần số và pattern có thể điều chỉnh
  - Pattern SOS: Bíp nhanh, liên tục
  - Pattern cảnh báo nhiệt độ/độ ẩm: Bíp chậm, ngắt quãng

✓ Mục đích
  - Cảnh báo người dùng về tai nạn
  - Cho phép người dùng hủy SOS (tránh cảnh báo nhầm)
  - Cảnh báo điều kiện môi trường bất thường
  - Hỗ trợ trong trường hợp khẩn cấp

================================================================================
11. QUẢN LÝ NGUỒN VÀ TIẾT KIỆM NĂNG LƯỢNG
================================================================================

✓ Tối ưu hóa
  - Dual-core processing
  - Task scheduling với FreeRTOS
  - Sleep mode khi không hoạt động (tùy chọn)

✓ Quản lý tài nguyên
  - Mutex cho shared data
  - Queue cho inter-task communication
  - Memory management

================================================================================
12. XỬ LÝ ĐA NHIỆM (MULTI-TASKING)
================================================================================

✓ FreeRTOS Tasks
  - Task Audio (Core 0): Ghi âm
  - Task SD Log (Core 0): Ghi log
  - Task Sensors (Core 1): Đọc cảm biến
  - Task SIM 4G (Core 1): Gửi dữ liệu
  - Task OLED (Core 1): Hiển thị

✓ Đồng bộ hóa
  - Mutex cho shared data
  - Queue cho message passing
  - Semaphore cho synchronization

✓ Ưu điểm
  - Xử lý song song
  - Không block các task khác
  - Hiệu suất cao

================================================================================
13. XỬ LÝ LỖI VÀ PHỤC HỒI
================================================================================

✓ Tự động phục hồi
  - Reconnect SIM 4G nếu mất kết nối
  - Retry khi gửi dữ liệu thất bại
  - Kiểm tra và khởi tạo lại cảm biến

✓ Xử lý ngoại lệ
  - Try-catch cho các thao tác quan trọng
  - Fallback khi thiết bị lỗi
  - Log lỗi để debug

================================================================================
14. TỔNG KẾT CHỨC NĂNG
================================================================================

✅ GHI VÀ LƯU TRỮ
   • Ghi nhật ký cảm biến liên tục (30s)
   • Log trạng thái SOS từ phần cứng (không log SOS từ web)
   • Ghi âm vòng tròn 50 file x 2 phút
   • Tên file audio: Theo thời gian kết thúc (YYYYMMDD_HHMMSS)
   • Lưu trên SD Card

✅ PHÁT HIỆN VÀ CẢNH BÁO
   • Phát hiện va chạm tự động (MPU9250)
   • Đếm ngược 30 giây + Buzzer bíp (tránh cảnh báo nhầm)
   • Cảnh báo nhiệt độ/độ ẩm vượt ngưỡng
   • Hiển thị trên OLED

✅ SOS VÀ CỨU HỘ
   • SOS tai nạn tự động (đếm ngược 30s + buzzer bíp)
   • SOS cứu hộ thủ công (nhấn giữ 5s, log vào nhật ký)
   • Gửi ID + vị trí + trạng thái SOS + ghi chú tự động
   • Đồng bộ với server (tự động liên hệ trạm cứu hộ)
   • Đồng bộ 2 chiều (server hủy → box hủy, trạm hoàn thành → box hủy)
   • Gửi SMS + Gọi điện
   • Gửi HTTP lên server

✅ THU THẬP DỮ LIỆU
   • GPS (vị trí, vận tốc)
   • MPU9250 (gia tốc, góc quay)
   • DHT11 (nhiệt độ, độ ẩm)
   • DS1307 (thời gian thực)

✅ KẾT NỐI VÀ GỬI DỮ LIỆU
   • SIM 4G (Internet, SMS, Call)
   • Gửi telemetry mỗi 30 giây
   • Real-time sync với server

✅ HIỂN THỊ VÀ TƯƠNG TÁC
   • OLED 128x64 (hiển thị trạng thái SOS/cứu hộ)
   • 2 nút nhấn (Button 1: SOS tai nạn, Button 2: SOS cứu hộ)
   • Nhấn đồng thời 2 nút 10s → Chế độ thiết lập (Webserver)
   • Buzzer cảnh báo (SOS + nhiệt độ/độ ẩm)

✅ CẤU HÌNH VÀ THIẾT LẬP
   • Webserver tự phát khi nhấn đồng thời 2 nút 10s
   • Thiết lập: ID thiết bị, ID người dùng, độ nhạy cảm biến
   • Thiết lập: Ngưỡng nhiệt độ/độ ẩm, số điện thoại SOS
   • Lưu cấu hình vào EEPROM/SD Card

✅ XỬ LÝ ĐA NHIỆM
   • FreeRTOS dual-core
   • Task scheduling
   • Đồng bộ hóa

================================================================================
                    KẾT THÚC DANH SÁCH CHỨC NĂNG
================================================================================

