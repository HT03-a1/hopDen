# Thiết lập nạp ESP32-S3 N16R8 trong Cursor

Tài liệu này giúp bạn chuẩn bị đầy đủ công cụ để biên dịch và nạp các sketch trong thư mục `hardware/` (ví dụ `esp32_s3_blackbox_v2.ino`) trực tiếp từ Terminal của Cursor trên Windows.

## 1. Chuẩn bị thiết bị & driver
- Cắm cáp USB-C có hỗ trợ data giữa ESP32-S3 và PC.
- Cài driver USB-to-UART (Silicon Labs CP210x hoặc FTDI) nếu thiết bị không hiển thị trong Device Manager → Ports (COM & LPT).

## 2. Cài Arduino CLI (ưu tiên cho automation)
1. Tải file `.zip` tại https://arduino.github.io/arduino-cli/latest/installation/ và giải nén, thêm thư mục chứa `arduino-cli.exe` vào `PATH`.
2. Trong Cursor Terminal (PowerShell):
   ```powershell
   arduino-cli config init
   arduino-cli config set board_manager.additional_urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
   arduino-cli core update-index
   arduino-cli core install esp32:esp32@3.0.2
   ```
3. Xác nhận bo mạch nhận diện:
   ```powershell
   arduino-cli board list
   ```
   Ghi lại cổng COM (ví dụ `COM7`) của ESP32-S3.

### Tuỳ chọn ESP32-S3 N16R8 (16MB flash, 8MB PSRAM)
Sử dụng FQBN sau khi cài core:
```
esp32:esp32:esp32s3:CDCOnBoot=default,FlashMode=qio,PartitionScheme=default,USBMode=cdc,PSRAM=enabled,FlashSize=16M
```
Bạn có thể điều chỉnh `PartitionScheme` (ví dụ `huge_app`) khi cần thêm không gian chương trình.

## 3. Biên dịch & nạp từ Cursor
Đứng tại thư mục gốc dự án (`C:\Users\admin\Downloads\hopDen`) và chạy:
```powershell
# Biên dịch
arduino-cli compile --fqbn esp32:esp32:esp32s3:CDCOnBoot=default,FlashMode=qio,PartitionScheme=default,USBMode=cdc,PSRAM=enabled,FlashSize=16M hardware/esp32_s3_blackbox_v2.ino

# Nạp (thay COM7 bằng cổng thực tế)
arduino-cli upload -p COM7 --fqbn esp32:esp32:esp32s3:CDCOnBoot=default,FlashMode=qio,PartitionScheme=default,USBMode=cdc,PSRAM=enabled,FlashSize=16M hardware/esp32_s3_blackbox_v2.ino
```
Nếu gặp lỗi “Failed to connect”, giữ nút `BOOT` trên ESP32-S3, nhấn `RESET`, thả `RESET` rồi thả `BOOT`.

## 4. Cài thư viện bắt buộc
Sketch `esp32_s3_blackbox_v2.ino` yêu cầu các thư viện Arduino sau (ngoài thư viện lõi ESP32 đã có sẵn):

| Thư viện | Công dụng | Lệnh cài (Arduino CLI) |
|----------|-----------|------------------------|
| `DHT sensor library` | Cảm biến nhiệt độ/độ ẩm DHT11 | `arduino-cli lib install "DHT sensor library"` |
| `RTClib` | Giao tiếp DS1307 | `arduino-cli lib install "RTClib"` |
| `TinyGPS++` | Đọc NEO-8M | `arduino-cli lib install "TinyGPS++"` |
| `U8g2` | OLED SSD1306 I2C | `arduino-cli lib install "U8g2"` |
| `ArduinoJson` (>=6) | Đóng gói JSON telemetry | `arduino-cli lib install "ArduinoJson"` |
| `TinyGSM` | Giao tiếp module SIM7600 | `arduino-cli lib install "TinyGSM"` |
| `ArduinoHttpClient` | Gửi HTTP qua TinyGSM | `arduino-cli lib install "ArduinoHttpClient"` |

Các thư viện lõi như `SD`, `SPI`, `Wire`, `EEPROM`, `FreeRTOS` đã nằm trong core ESP32 nên không cần cài thêm.

> Mẹo: chạy `arduino-cli lib list` để kiểm tra tình trạng thư viện đã cài.

## 5. Tạo script tiện lợi (khuyến nghị)
Thêm script PowerShell vào dự án (ví dụ `hardware/flash_blackbox.ps1`):
```powershell
$fqbn = "esp32:esp32:esp32s3:CDCOnBoot=default,FlashMode=qio,PartitionScheme=default,USBMode=cdc,PSRAM=enabled,FlashSize=16M"
$port = "COM7"
arduino-cli compile --fqbn $fqbn hardware/esp32_s3_blackbox_v2.ino
if ($LASTEXITCODE -eq 0) {
    arduino-cli upload -p $port --fqbn $fqbn hardware/esp32_s3_blackbox_v2.ino
}
```
Khi dùng Cursor, chỉ cần `pwsh hardware/flash_blackbox.ps1`.

## 6. Kiểm tra sau khi nạp
- Mở Serial Monitor (`arduino-cli monitor -p COM7 -c baudrate=115200`) để xem log khởi động.
- Đảm bảo các task FreeRTOS (`taskSensors`, `taskSDLog`, `taskSIM4G`, `taskOLED`) log trạng thái “READY”.

## 7. Tuỳ chọn khác
- **ESP-IDF + Arduino component:** nếu muốn tuỳ biến sâu hơn, cài ESP-IDF 5.x rồi thêm `arduino-esp32` làm component. Tuy nhiên các sketch hiện tại đã viết theo Arduino nên Arduino CLI đủ dùng.
- **PlatformIO:** có thể tạo `platformio.ini` với `platform = espressif32`, `board = esp32s3box`, `framework = arduino`, nhưng chưa cần cho nhu cầu hiện tại.

Với các bước trên, bạn có thể nạp code trực tiếp từ Cursor Terminal lên ESP32-S3 N16R8 và đảm bảo đầy đủ thư viện cần thiết cho các sketch trong thư mục `hardware/`.

