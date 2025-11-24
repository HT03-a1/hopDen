================================================================================
    HỘP ĐEN Ô TÔ / XE MÁY - ESP32-S3
    HƯỚNG DẪN TEST VÀ KIỂM TRA
================================================================================

📋 MỤC LỤC
----------
1. Chuẩn bị trước khi test
2. Test từng module riêng lẻ
3. Test chức năng chính
4. Test tích hợp
5. Test đồng bộ với server
6. Troubleshooting

================================================================================
1. CHUẨN BỊ TRƯỚC KHI TEST
================================================================================

✓ PHẦN CỨNG
  - ESP32-S3 đã nạp code
  - Tất cả linh kiện đã kết nối đúng sơ đồ
  - SD Card đã format FAT32 (8GB-32GB)
  - Pin backup CR2032 cho DS1307 (nếu có)
  - SIM card 4G đã kích hoạt
  - Nguồn 5V/2A-3A ổn định

✓ PHẦN MỀM
  - Arduino IDE đã cài đặt
  - Thư viện đã cài đặt:
    • RTClib
    • TinyGPS++
    • U8g2
    • ArduinoJson
    • TinyGSM
    • DHT sensor library
  - Serial Monitor mở (115200 baud)
  - Backend server đang chạy (nếu test đồng bộ)

✓ CÔNG CỤ
  - Serial Monitor (Arduino IDE)
  - File Explorer (để xem SD Card)
  - Web browser (để test webserver config)
  - Điện thoại (để test SMS/Call)

================================================================================
2. TEST TỪNG MODULE RIÊNG LẺ
================================================================================

2.1. TEST I2C BUS
-----------------

✓ Kiểm tra kết nối
  - SDA: GPIO 8
  - SCL: GPIO 9
  - VCC: 3.3V
  - GND: GND

✓ Test MPU9250 (Address 0x50)
  - Upload code test I2C scanner
  - Kiểm tra Serial Monitor
  - Phải thấy địa chỉ 0x50
  - Đọc giá trị gia tốc (ax, ay, az)
  - Lắc module → Giá trị thay đổi

✓ Test DS1307 (Address 0x68)
  - Upload code test DS1307
  - Kiểm tra Serial Monitor
  - Phải thấy địa chỉ 0x68
  - Đọc thời gian hiện tại
  - Thời gian phải hợp lệ (2000-2100)

✓ Test OLED (Address 0x3C)
  - Upload code test OLED
  - Màn hình hiển thị text/graphics
  - Không có lỗi I2C

✓ Lưu ý
  - Nếu không thấy địa chỉ → Kiểm tra kết nối
  - Nếu địa chỉ sai → Kiểm tra pull-up resistors
  - Nếu lỗi I2C → Kiểm tra dây dài quá (>30cm)

2.2. TEST GPS NEO-8M
--------------------

✓ Kiểm tra kết nối
  - TX: GPIO 7
  - RX: GPIO 6
  - VCC: 3.3V hoặc 5V
  - GND: GND

✓ Test GPS
  - Upload code test GPS
  - Mở Serial Monitor (9600 baud)
  - Đặt GPS ở ngoài trời hoặc gần cửa sổ
  - Đợi 1-2 phút để GPS lock
  - Phải thấy NMEA data:
    $GPGGA, $GPRMC, ...
  - Kiểm tra:
    • Latitude (lat) hợp lệ
    • Longitude (lon) hợp lệ
    • Speed (nếu có)
    • Số vệ tinh > 4

✓ Lưu ý
  - GPS cần thời gian để lock (cold start: 30s-2 phút)
  - Phải ở ngoài trời hoặc gần cửa sổ
  - Nếu không lock → Kiểm tra nguồn, ăng-ten

2.3. TEST DHT11
---------------

✓ Kiểm tra kết nối
  - DATA: GPIO 35
  - VCC: 3.3V
  - GND: GND

✓ Test DHT11
  - Upload code test DHT11
  - Kiểm tra Serial Monitor
  - Đọc nhiệt độ (0-50°C)
  - Đọc độ ẩm (20-90% RH)
  - Thổi hơi vào sensor → Độ ẩm tăng
  - Sưởi sensor → Nhiệt độ tăng

✓ Lưu ý
  - DHT11 cần 2 giây giữa các lần đọc
  - Nếu đọc lỗi → Kiểm tra pull-up resistor
  - Nếu giá trị không hợp lệ → Kiểm tra kết nối

2.4. TEST SD CARD
-----------------

✓ Kiểm tra kết nối
  - CS: GPIO 10
  - SCK: GPIO 12
  - MOSI: GPIO 11
  - MISO: GPIO 13
  - VCC: 3.3V
  - GND: GND

✓ Test SD Card
  - Upload code test SD Card
  - Kiểm tra Serial Monitor
  - Phải thấy "SD Card initialized"
  - Tạo file test
  - Ghi dữ liệu vào file
  - Đọc lại dữ liệu
  - So sánh dữ liệu

✓ Lưu ý
  - Format SD Card: FAT32
  - Dung lượng: 8GB-32GB (khuyến nghị)
  - Nếu lỗi → Kiểm tra format, dây kết nối

2.5. TEST INMP441 (MICROPHONE)
-------------------------------

✓ Kiểm tra kết nối
  - WS: GPIO 42
  - SD: GPIO 40
  - SCK: GPIO 41
  - VDD: 3.3V
  - GND: GND
  - L/R: GND

✓ Test Microphone
  - Upload code test microphone
  - Kiểm tra Serial Monitor
  - Phải thấy "I2S Audio initialized"
  - Ghi âm vào file WAV
  - Phát lại file → Có âm thanh
  - Nói vào mic → Biên độ thay đổi

✓ Lưu ý
  - I2S cần cấu hình đúng sample rate
  - Kiểm tra buffer size
  - Nếu không có âm → Kiểm tra kết nối, cấu hình

2.6. TEST SIM 4G
-----------------

✓ Kiểm tra kết nối
  - TX: GPIO 17
  - RX: GPIO 18
  - VCC: 3.3V hoặc 5V (tùy module)
  - GND: GND

✓ Test SIM 4G
  - Upload code test SIM 4G
  - Kiểm tra Serial Monitor
  - Gửi lệnh AT → Phải nhận "OK"
  - Kiểm tra SIM card:
    AT+CPIN? → Phải thấy "READY"
  - Kiểm tra tín hiệu:
    AT+CSQ → Phải > 10 (tín hiệu tốt)
  - Kiểm tra mạng:
    AT+CREG? → Phải thấy "0,1" hoặc "0,5" (đã đăng ký)
  - Test SMS:
    AT+CMGS="0123456789" → Gửi SMS test
  - Test Call:
    ATD0123456789; → Gọi điện test

✓ Lưu ý
  - SIM card phải kích hoạt và có tiền
  - APN phải đúng nhà mạng
  - Nếu không kết nối → Kiểm tra APN, SIM card

2.7. TEST NÚT NHẤN VÀ BUZZER
------------------------------

✓ Kiểm tra kết nối
  - Button 1: GPIO 36
  - Button 2: GPIO 37
  - Buzzer: GPIO 15

✓ Test Nút nhấn
  - Upload code test button
  - Nhấn Button 1 → Serial Monitor hiển thị
  - Nhấn Button 2 → Serial Monitor hiển thị
  - Nhấn giữ 5 giây → Serial Monitor hiển thị
  - Nhấn đồng thời 2 nút 10 giây → Vào chế độ config

✓ Test Buzzer
  - Upload code test buzzer
  - Buzzer phát âm thanh
  - Thay đổi tần số → Âm thanh thay đổi
  - Pattern bíp → Hoạt động đúng

✓ Lưu ý
  - Buttons dùng INPUT_PULLUP → Nhấn = LOW
  - Buzzer có thể cần PWM để phát âm
  - Nếu không hoạt động → Kiểm tra kết nối, code

================================================================================
3. TEST CHỨC NĂNG CHÍNH
================================================================================

3.1. TEST GHI NHẬT KÝ (DATA LOGGING)
-------------------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Đợi 30 giây
  - Lấy SD Card ra
  - Mở file LOG_YYYYMMDD.csv
  - Kiểm tra:
    • Có dữ liệu mỗi 30 giây
    • Có đầy đủ: thời gian, GPS, MPU, DHT11
    • Có trạng thái SOS (nếu có)
    • Format CSV đúng

✓ Test case
  1. Bình thường → Log dữ liệu cảm biến
  2. Có SOS tai nạn → Log trạng thái "accident"
  3. Có SOS cứu hộ → Log trạng thái "rescue"
  4. Không có SOS → Log trạng thái "none"

✓ Kết quả mong đợi
  - File log được tạo mỗi ngày
  - Dữ liệu ghi đúng định dạng
  - Trạng thái SOS được log đúng

3.2. TEST GHI ÂM VÒNG TRÒN
----------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Đợi 2 phút
  - Lấy SD Card ra
  - Kiểm tra file audio:
    • Tên file: AUDIO_YYYYMMDD_HHMMSS.wav
    • Thời gian kết thúc đúng
    • File có thể phát được
    • Có âm thanh

✓ Test case
  1. Ghi âm 2 phút → File được tạo
  2. Ghi đầy 50 file → File cũ bị ghi đè
  3. Tên file theo thời gian kết thúc
  4. File WAV có header đúng

✓ Kết quả mong đợi
  - File audio được tạo mỗi 2 phút
  - Tên file đúng format
  - Có thể phát lại được
  - Tự động ghi đè khi đầy 50 file

3.3. TEST PHÁT HIỆN VA CHẠM TỰ ĐỘNG
------------------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Lắc mạnh module MPU9250 (vượt ngưỡng 2.5g)
  - Quan sát:
    • Serial Monitor: "ACCIDENT DETECTED"
    • OLED: Hiển thị đếm ngược 30 giây
    • Buzzer: Bíp liên tục
    • Đếm ngược: 30 → 0

✓ Test case
  1. Lắc nhẹ (< 2.5g) → Không phát hiện
  2. Lắc mạnh (> 2.5g) → Phát hiện, đếm ngược
  3. Nhấn giữ Button 1 (5s) → Hủy SOS
  4. Không hủy → Gửi SOS sau 30 giây

✓ Kết quả mong đợi
  - Phát hiện va chạm đúng ngưỡng
  - Đếm ngược 30 giây
  - Buzzer bíp liên tục
  - Có thể hủy bằng nút
  - Tự động gửi SOS nếu không hủy

3.4. TEST SOS CỨU HỘ THỦ CÔNG
-------------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Nhấn giữ Button 2 (5 giây)
  - Quan sát:
    • Serial Monitor: "RESCUE SOS ACTIVATED"
    • OLED: Hiển thị đếm ngược
    • Buzzer: Bíp cảnh báo
    • Log vào nhật ký

✓ Test case
  1. Nhấn giữ Button 2 (5s) → Kích hoạt SOS cứu hộ
  2. Đếm ngược 30 giây → Hiển thị trên OLED
  3. Nhấn giữ Button 2 (5s) lần nữa → Hủy SOS
  4. Không hủy → Gửi SOS sau 30 giây
  5. Kiểm tra log → Có ghi lại kích hoạt/hủy

✓ Kết quả mong đợi
  - Kích hoạt bằng nhấn giữ 5 giây
  - Đếm ngược 30 giây
  - Có thể hủy
  - Log vào nhật ký
  - Hiển thị trên OLED

3.5. TEST CẢNH BÁO NHIỆT ĐỘ/ĐỘ ẨM
------------------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Sưởi DHT11 (nhiệt độ > ngưỡng)
  - Hoặc tăng độ ẩm (độ ẩm > ngưỡng)
  - Quan sát:
    • Serial Monitor: "WARNING: Temp/Humi"
    • Buzzer: Bíp cảnh báo (pattern khác SOS)

✓ Test case
  1. Nhiệt độ < ngưỡng → Không cảnh báo
  2. Nhiệt độ > ngưỡng → Buzzer bíp
  3. Độ ẩm < ngưỡng → Không cảnh báo
  4. Độ ẩm > ngưỡng → Buzzer bíp
  5. Pattern cảnh báo khác pattern SOS

✓ Kết quả mong đợi
  - Cảnh báo khi vượt ngưỡng
  - Buzzer bíp với pattern riêng
  - Không ảnh hưởng đến SOS

3.6. TEST GỬI DỮ LIỆU LÊN SERVER
----------------------------------

✓ Kiểm tra
  - Backend server đang chạy
  - SIM 4G đã kết nối Internet
  - Upload code hộp đen
  - Đợi 30 giây
  - Kiểm tra server:
    • Nhận được telemetry
    • Có đầy đủ: userId, deviceId, lat, lon, speed
    • Có trạng thái SOS (nếu có)
    • Source: "hardware"

✓ Test case
  1. Bình thường → Gửi telemetry mỗi 30 giây
  2. Có SOS tai nạn → Trạng thái "accident"
  3. Có SOS cứu hộ → Trạng thái "rescue"
  4. Không có SOS → Trạng thái "none"

✓ Kết quả mong đợi
  - Gửi telemetry đúng định kỳ
  - Dữ liệu đầy đủ và chính xác
  - Trạng thái SOS được gửi kèm

3.7. TEST GỬI SOS LÊN SERVER
------------------------------

✓ Kiểm tra
  - Backend server đang chạy
  - SIM 4G đã kết nối Internet
  - Kích hoạt SOS (tự động hoặc thủ công)
  - Kiểm tra server:
    • Nhận được SOS request
    • Có đầy đủ: userId, deviceId, type, location
    • Có ghi chú tự động
    • Server tự động liên hệ trạm

✓ Test case
  1. SOS tai nạn tự động → Server nhận "accident"
  2. SOS cứu hộ thủ công → Server nhận "breakdown"
  3. Ghi chú tự động → Có thông tin cảm biến
  4. Server liên hệ trạm → Trạm nhận được SOS

✓ Kết quả mong đợi
  - SOS được gửi lên server
  - Server tự động điều phối
  - Trạm nhận được SOS

3.8. TEST SMS VÀ GỌI ĐIỆN
---------------------------

✓ Kiểm tra
  - SIM 4G đã kích hoạt
  - Số điện thoại SOS đã cấu hình
  - Kích hoạt SOS
  - Kiểm tra điện thoại:
    • Nhận được SMS
    • Nhận được cuộc gọi

✓ Test case
  1. SOS tai nạn → Gửi SMS + Gọi điện
  2. SOS cứu hộ → Gửi SMS + Gọi điện
  3. SMS có đầy đủ: vị trí, vận tốc, thời gian
  4. Cuộc gọi tự động kết nối

✓ Kết quả mong đợi
  - SMS được gửi đến số SOS
  - Cuộc gọi tự động
  - Nội dung SMS đầy đủ

================================================================================
4. TEST TÍCH HỢP
================================================================================

4.1. TEST ĐỒNG BỘ 2 CHIỀU VỚI SERVER
--------------------------------------

✓ Kiểm tra
  - Backend server đang chạy
  - Box đã gửi SOS lên server
  - Server có ID SOS
  - Test đồng bộ:
    1. Server hủy SOS → Box tự động hủy
    2. Trạm hoàn thành → Box tự động hủy

✓ Test case
  1. Box gửi SOS → Server nhận, tạo SOS ID
  2. Box lưu SOS ID
  3. Box polling server mỗi 5 giây
  4. Server hủy SOS → Box nhận, hủy trạng thái
  5. Trạm hoàn thành → Box nhận, hủy trạng thái

✓ Kết quả mong đợi
  - Box và server đồng bộ trạng thái
  - Box tự động hủy khi server/trạm hủy
  - Trạng thái nhất quán

4.2. TEST WEBSERVER THIẾT LẬP
------------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Nhấn đồng thời Button 1 + Button 2
  - Giữ trong 10 giây
  - Quan sát:
    • Serial Monitor: "CONFIG MODE ACTIVE"
    • WiFi AP: "HOPDEN_CONFIG_XXXX"
    • IP: 192.168.4.1

✓ Test case
  1. Nhấn đồng thời 2 nút 10s → Vào config mode
  2. Kết nối WiFi AP
  3. Mở trình duyệt: http://192.168.4.1
  4. Xem cấu hình hiện tại
  5. Thay đổi cấu hình:
     • ID thiết bị
     • ID người dùng
     • Ngưỡng gia tốc
     • Ngưỡng nhiệt độ/độ ẩm
     • Số điện thoại SOS
  6. Lưu cấu hình
  7. Reset ESP32
  8. Kiểm tra cấu hình đã lưu

✓ Kết quả mong đợi
  - Vào config mode đúng cách
  - Web interface hoạt động
  - Lưu cấu hình vào EEPROM
  - Load cấu hình khi khởi động lại

4.3. TEST HIỂN THỊ OLED
------------------------

✓ Kiểm tra
  - Upload code hộp đen
  - Quan sát OLED:
    • Thời gian (từ DS1307)
    • Vị trí GPS
    • Nhiệt độ/Độ ẩm
    • Trạng thái hệ thống
    • Đếm ngược SOS (nếu có)

✓ Test case
  1. Bình thường → Hiển thị "NORMAL"
  2. Đếm ngược SOS → Hiển thị "SOS: 30", "SOS: 29", ...
  3. SOS đã gửi → Hiển thị "SOS SENT"
  4. Cập nhật mỗi 100ms

✓ Kết quả mong đợi
  - OLED hiển thị đầy đủ thông tin
  - Cập nhật real-time
  - Trạng thái rõ ràng

================================================================================
5. TEST ĐỒNG BỘ VỚI SERVER
================================================================================

5.1. TEST GỬI SOS VÀ NHẬN ID
------------------------------

✓ Quy trình
  1. Box gửi SOS lên server (POST /api/sos)
  2. Server tạo SOS và trả về ID
  3. Box lưu SOS ID
  4. Box polling server để kiểm tra trạng thái

✓ Kiểm tra
  - Serial Monitor: "SOS sent, ID: SOS123456"
  - Box lưu ID vào systemState.currentSOSId
  - Box polling server mỗi 5 giây

5.2. TEST SERVER HỦY SOS
--------------------------

✓ Quy trình
  1. Box có SOS active
  2. Server hủy SOS (PATCH /api/sos/{id}/status → cancelled)
  3. Box polling và phát hiện trạng thái "cancelled"
  4. Box tự động hủy trạng thái SOS

✓ Kiểm tra
  - Serial Monitor: "SOS cancelled by server"
  - systemState.status = STATUS_NORMAL
  - systemState.currentSOSSource = SOS_SOURCE_NONE
  - Buzzer dừng
  - OLED hiển thị "NORMAL"

5.3. TEST TRẠM HOÀN THÀNH
---------------------------

✓ Quy trình
  1. Box có SOS active
  2. Trạm nhận và hoàn thành (status = "done")
  3. Box polling và phát hiện trạng thái "done"
  4. Box tự động hủy trạng thái SOS

✓ Kiểm tra
  - Serial Monitor: "SOS completed by station"
  - systemState.status = STATUS_NORMAL
  - systemState.currentSOSSource = SOS_SOURCE_NONE
  - Buzzer dừng
  - OLED hiển thị "NORMAL"

================================================================================
6. TROUBLESHOOTING
================================================================================

6.1. LỖI THƯỜNG GẶP
--------------------

❌ I2C không tìm thấy thiết bị
   → Kiểm tra:
     • Dây SDA, SCL kết nối đúng
     • Pull-up resistors (4.7kΩ)
     • Địa chỉ I2C đúng (0x50, 0x68, 0x3C)
     • Nguồn cấp cho module

❌ GPS không lock
   → Kiểm tra:
     • GPS ở ngoài trời hoặc gần cửa sổ
     • Nguồn cấp đủ (3.3V hoặc 5V)
     • Ăng-ten GPS
     • Đợi đủ thời gian (30s-2 phút)

❌ SD Card không đọc được
   → Kiểm tra:
     • Format FAT32
     • Dung lượng 8GB-32GB
     • Dây kết nối SPI
     • Chip select (CS) đúng

❌ SIM 4G không kết nối
   → Kiểm tra:
     • SIM card đã kích hoạt
     • APN đúng nhà mạng
     • Tín hiệu (AT+CSQ > 10)
     • Đăng ký mạng (AT+CREG?)

❌ SOS không gửi được
   → Kiểm tra:
     • SIM 4G đã kết nối Internet
     • Backend server đang chạy
     • URL server đúng
     • JSON format đúng

❌ Buzzer không kêu
   → Kiểm tra:
     • Kết nối GPIO 15
     • Code điều khiển buzzer
     • Buzzer active hay passive
     • Nếu passive → Cần PWM

❌ OLED không hiển thị
   → Kiểm tra:
     • Kết nối I2C
     • Địa chỉ 0x3C
     • Code khởi tạo OLED
     • Font được load

6.2. DEBUG TIPS
---------------

✓ Serial Monitor
  - Bật Serial Monitor (115200 baud)
  - Xem log để debug
  - Kiểm tra lỗi

✓ SD Card
  - Lấy SD Card ra
  - Mở file log/audio
  - Kiểm tra dữ liệu

✓ OLED
  - Xem thông tin hiển thị
  - Kiểm tra trạng thái
  - Debug trực quan

✓ Server
  - Kiểm tra log server
  - Xem API requests
  - Kiểm tra database

6.3. CHECKLIST TEST
--------------------

□ I2C bus hoạt động (MPU9250, DS1307, OLED)
□ GPS lock và đọc được vị trí
□ DHT11 đọc được nhiệt độ/độ ẩm
□ SD Card ghi/đọc được
□ Microphone ghi âm được
□ SIM 4G kết nối Internet
□ Nút nhấn hoạt động
□ Buzzer phát âm thanh
□ Ghi nhật ký mỗi 30 giây
□ Ghi âm 2 phút/file, 50 file
□ Phát hiện va chạm tự động
□ SOS tai nạn hoạt động
□ SOS cứu hộ hoạt động
□ Cảnh báo nhiệt độ/độ ẩm
□ Gửi telemetry lên server
□ Gửi SOS lên server
□ SMS và gọi điện hoạt động
□ Đồng bộ SOS với server
□ Webserver config mode hoạt động
□ OLED hiển thị đúng
□ Tất cả chức năng tích hợp

================================================================================
7. TEST KỊCH BẢN THỰC TẾ
================================================================================

7.1. KỊCH BẢN 1: TAI NẠN THẬT
-------------------------------

✓ Mô phỏng
  1. Lắc mạnh box (vượt ngưỡng 2.5g)
  2. Quan sát:
     • Buzzer bíp liên tục
     • OLED đếm ngược 30 giây
     • Không hủy
  3. Sau 30 giây:
     • SOS được gửi lên server
     • SMS được gửi
     • Cuộc gọi được thực hiện
     • Log vào nhật ký
     • Audio được ghi

✓ Kết quả mong đợi
  - Tất cả chức năng hoạt động
  - Dữ liệu được lưu đầy đủ
  - Server nhận được SOS
  - Trạm được thông báo

7.2. KỊCH BẢN 2: HỎNG XE
-------------------------

✓ Mô phỏng
  1. Nhấn giữ Button 2 (5 giây)
  2. Quan sát:
     • OLED hiển thị "RESCUE SOS"
     • Đếm ngược 30 giây
     • Buzzer bíp
  3. Sau 30 giây:
     • SOS được gửi
     • Log vào nhật ký

✓ Kết quả mong đợi
  - SOS cứu hộ hoạt động
  - Log ghi lại kích hoạt
  - Server nhận được SOS

7.3. KỊCH BẢN 3: CẢNH BÁO NHẦM
--------------------------------

✓ Mô phỏng
  1. Lắc mạnh box (phát hiện va chạm)
  2. Quan sát đếm ngược
  3. Nhấn giữ Button 1 (5 giây) để hủy
  4. Quan sát:
     • Buzzer dừng
     • Trạng thái về NORMAL
     • Không gửi SOS

✓ Kết quả mong đợi
  - Có thể hủy SOS
  - Không gửi SOS khi hủy
  - Trạng thái về bình thường

7.4. KỊCH BẢN 4: ĐỒNG BỘ VỚI SERVER
------------------------------------

✓ Mô phỏng
  1. Box gửi SOS lên server
  2. Server nhận và tạo SOS
  3. Trạm nhận và hoàn thành
  4. Box polling và phát hiện "done"
  5. Box tự động hủy SOS

✓ Kết quả mong đợi
  - Box và server đồng bộ
  - Box tự động hủy khi trạm hoàn thành
  - Trạng thái nhất quán

================================================================================
                    KẾT THÚC HƯỚNG DẪN TEST
================================================================================

Lưu ý:
- Test từng module riêng trước khi test tích hợp
- Ghi lại kết quả test để so sánh
- Nếu có lỗi, kiểm tra từng bước một
- Tham khảo Serial Monitor để debug



