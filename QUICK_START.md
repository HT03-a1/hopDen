# Hướng dẫn chạy nhanh

## Bước 1: Cài đặt Backend

```bash
cd backend
npm install
npm run dev
```

Backend sẽ chạy tại `http://localhost:3000`

**Lưu ý**: Khi server khởi động lần đầu, các file JSON sẽ được tạo tự động trong `backend/data/`. File `taikhoan.md` sẽ được tạo tự động chứa tất cả tài khoản đăng nhập.

## Bước 2: Cài đặt Frontend (terminal mới)

```bash
cd frontend
npm install
npm run dev
```

Frontend sẽ chạy tại `http://localhost:5173`

## Bước 3: Test với tài khoản mẫu

Xem file `backend/data/taikhoan.md` để xem danh sách đầy đủ tất cả tài khoản.

### Đăng nhập như User:
1. Mở `http://localhost:5173`
2. Chọn "Người dùng"
3. Email: `nguyenvanan@example.com`
4. Password: `password123`
5. Click "Đăng nhập"

### Đăng nhập như Trạm y tế:
1. Chọn "Trạm y tế"
2. Email: `cho-ray@example.com`
3. Password: `password123`
4. Click "Đăng nhập"

### Đăng nhập như Trạm cứu hộ:
1. Chọn "Trạm cứu hộ"
2. Email: `cuuhocq1@example.com`
3. Password: `password123`
4. Click "Đăng nhập"

**Lưu ý**: Trạm sửa xe đã được gộp vào Trạm cứu hộ.

## Test các tính năng

### Với User:

#### Quản lý vị trí
- ✅ Xem ID người dùng (hiển thị rõ trong panel) - dùng để cặp với ESP32
- ✅ Nhập thủ công vị trí nếu không có ESP32
- ✅ Vị trí tự động cập nhật từ ESP32 (nếu có)

#### SOS & Cứu hộ
- ✅ Xem bản đồ với các trạm và user khác
- ✅ Gửi SOS (nút đỏ "Gửi SOS")
  - Chọn loại: Tai nạn, Hỏng xe, Y tế, Khác
  - Chọn mức độ: Thấp, Trung bình, Cao, Khẩn cấp
  - Thêm ghi chú (tùy chọn)
- ✅ Hủy SOS đang active
- ✅ Xem thông tin trạm đang kết nối
  - Trạng thái: "Đang kết nối" → "Đã nhận" → "Đang đi"
  - Tự động cập nhật khi trạm thay đổi
- ✅ Tìm trạm y tế gần nhất
- ✅ Tìm trạm cứu hộ gần nhất
- ✅ Xem đường đi đến trạm (routing theo đường thực tế)
- ✅ Xem lịch sử SOS

### Với Trạm:

#### Quản lý SOS
- ✅ Xem SOS trong khu vực (tự động lọc theo loại trạm)
  - Trạm y tế: Chỉ thấy SOS "Tai nạn" và "Y tế"
  - Trạm cứu hộ: Chỉ thấy SOS "Hỏng xe" và "Khác"
- ✅ **Sẵn sàng nhận nhiệm vụ**: Đánh dấu để được ưu tiên
- ✅ Nhận nhiệm vụ (claim SOS)
- ✅ Từ chối nhiệm vụ (tự động chuyển sang trạm khác)
- ✅ Cập nhật trạng thái: Đã nhận → Đang đi → Hoàn thành
- ✅ Hủy nhiệm vụ (tự động chuyển sang trạm khác)

#### Thông tin & Điều hướng
- ✅ Hiển thị khoảng cách đến SOS
- ✅ Hiển thị đường đi đến vị trí SOS (routing theo đường thực tế)
- ✅ Mở Google Maps để điều hướng
- ✅ Xem thông tin chi tiết SOS và người dùng
- ✅ Xem lịch sử nhiệm vụ đã hoàn thành (30 ngày gần nhất)
- ✅ Vị trí người dùng bị ẩn sau khi hoàn thành nhiệm vụ (bảo vệ quyền riêng tư)

## Đăng ký tài khoản mới

### Đăng ký User mới:
1. Click "Đăng ký tài khoản người dùng"
2. Điền thông tin (bao gồm lat/lon)
3. Sau khi đăng ký, ID user mới sẽ được tạo tự động (ví dụ: U0006)
4. ID này sẽ hiển thị rõ trên giao diện để dùng cặp với thiết bị ESP32

### Đăng ký Trạm mới:
1. Click "Đăng ký trạm y tế / cứu hộ"
2. Điền thông tin trạm
3. Chọn loại: Trạm y tế / Trạm cứu hộ

## Tích hợp ESP32 (Tùy chọn)

### Cấu hình ESP32

1. Mở file `hardware/esp32_fake_telemetry.ino` trong Arduino IDE
2. Cài đặt thư viện:
   - WiFi
   - HTTPClient
   - ArduinoJson
3. Cấu hình WiFi và Backend URL
4. Cấu hình `USER_ID` khớp với ID user trong hệ thống
5. Upload code lên ESP32

### Test ESP32

1. ESP32 sẽ tự động kết nối WiFi và đăng nhập
2. ESP32 gửi dữ liệu GPS mỗi 30 giây
3. Vị trí user trên web sẽ tự động cập nhật
4. Map sẽ tự động reload để hiển thị vị trí mới

## Lưu ý

- Tất cả dữ liệu được lưu trong `backend/data/*.json`
- Khi đăng ký mới, dữ liệu sẽ được ghi thêm vào file JSON
- WebSocket sẽ tự động cập nhật khi có SOS mới hoặc cập nhật
- File `taikhoan.md` được tạo tự động và cập nhật khi có tài khoản mới
- Routing sử dụng OSRM để tính đường đi theo đường thực tế
- Dữ liệu ESP32 được ưu tiên hơn dữ liệu nhập thủ công

## Troubleshooting

### Backend không chạy
- Kiểm tra port 3000 có bị chiếm không
- Kiểm tra Node.js version >= 18
- Xóa `node_modules` và chạy lại `npm install`

### Frontend không chạy
- Kiểm tra port 5173 có bị chiếm không
- Kiểm tra Node.js version >= 18
- Xóa `node_modules` và chạy lại `npm install`

### ESP32 không kết nối được
- Kiểm tra WiFi SSID và password
- Kiểm tra Backend URL và port
- Kiểm tra `USER_ID` có khớp với user trong hệ thống không
- Kiểm tra backend đang chạy

### Vị trí không cập nhật
- Kiểm tra ESP32 có gửi dữ liệu không (xem Serial Monitor)
- Kiểm tra WebSocket connection
- Kiểm tra `users.json` có được cập nhật không
- Thử nhập thủ công vị trí để test
