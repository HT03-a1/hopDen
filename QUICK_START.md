# Hướng dẫn chạy nhanh

## Bước 1: Cài đặt Backend

```bash
cd backend
npm install
npm run dev
```

Backend sẽ chạy tại `http://localhost:3000`

## Bước 2: Cài đặt Frontend (terminal mới)

```bash
cd frontend
npm install
npm run dev
```

Frontend sẽ chạy tại `http://localhost:5173`

## Bước 3: Test với tài khoản mẫu

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

## Test các tính năng

### Với User:
- ✅ Xem bản đồ với các trạm và user khác
- ✅ Gửi SOS (nút đỏ "Gửi SOS")
- ✅ Tìm trạm y tế gần nhất
- ✅ Tìm trạm cứu hộ gần nhất
- ✅ Xem lịch sử SOS
- ✅ Xem ID người dùng (hiển thị rõ trong panel)

### Với Trạm:
- ✅ Xem SOS trong khu vực
- ✅ Nhận nhiệm vụ (claim SOS)
- ✅ Cập nhật trạng thái: Đã nhận → Đang đi → Hoàn thành
- ✅ Hiển thị đường đi đến vị trí SOS

## Đăng ký tài khoản mới

### Đăng ký User mới:
1. Click "Đăng ký tài khoản người dùng"
2. Điền thông tin (bao gồm lat/lon)
3. Sau khi đăng ký, ID user mới sẽ được tạo tự động (ví dụ: U0006)
4. ID này sẽ hiển thị rõ trên giao diện để dùng cặp với thiết bị hộp đen

### Đăng ký Trạm mới:
1. Click "Đăng ký trạm y tế / cứu hộ"
2. Điền thông tin trạm
3. Chọn loại: Trạm y tế / Trạm cứu hộ / Trạm sửa xe

## Lưu ý

- Tất cả dữ liệu được lưu trong `backend/data/*.json`
- Khi đăng ký mới, dữ liệu sẽ được ghi thêm vào file JSON
- WebSocket sẽ tự động cập nhật khi có SOS mới hoặc cập nhật

