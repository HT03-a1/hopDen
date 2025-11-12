# Hướng dẫn sử dụng 2 tài khoản cùng lúc

## Cách 1: Mở 2 tab trình duyệt (Đơn giản nhất)

1. **Mở tab đầu tiên:**
   - Mở trình duyệt và vào `http://localhost:5173`
   - Đăng nhập với tài khoản thứ nhất
   - Ví dụ: `nguyenvanan@example.com` / `password123` (User)

2. **Mở tab thứ hai:**
   - Mở tab mới (Ctrl+T) hoặc cửa sổ mới (Ctrl+N)
   - Vào `http://localhost:5173` trong tab/cửa sổ mới
   - Đăng nhập với tài khoản thứ hai
   - Ví dụ: `cho-ray@example.com` / `password123` (Trạm y tế)

3. **Kết quả:**
   - Mỗi tab sẽ có session riêng (localStorage riêng)
   - Có thể test tương tác giữa 2 tài khoản
   - WebSocket sẽ kết nối riêng cho mỗi tab

## Cách 2: Mở 2 cửa sổ trình duyệt riêng

1. Mở cửa sổ trình duyệt thứ nhất
2. Mở cửa sổ trình duyệt thứ hai (Ctrl+Shift+N cho cửa sổ ẩn danh)
3. Đăng nhập 2 tài khoản khác nhau

## Cách 3: Dùng trình duyệt khác nhau

1. Mở Chrome với tài khoản 1
2. Mở Firefox/Edge với tài khoản 2

## Test các tình huống

### Test User gửi SOS và Trạm nhận:
1. Tab 1: Đăng nhập User (ví dụ: `nguyenvanan@example.com`)
2. Tab 2: Đăng nhập Trạm y tế (ví dụ: `cho-ray@example.com`)
3. Tab 1: Gửi SOS
4. Tab 2: Sẽ thấy SOS mới xuất hiện (realtime qua WebSocket)

### Test 2 User cùng lúc:
1. Tab 1: User 1 (`nguyenvanan@example.com`)
2. Tab 2: User 2 (`tranthibinh@example.com`)
3. Cả 2 có thể gửi SOS và xem map riêng

### Test User và Trạm cứu hộ:
1. Tab 1: User (`nguyenvanan@example.com`)
2. Tab 2: Trạm cứu hộ (`cuuhocq1@example.com`)
3. User gửi SOS → Trạm cứu hộ nhận và claim

## Lưu ý

- Mỗi tab có localStorage riêng, không ảnh hưởng nhau
- WebSocket kết nối riêng cho mỗi tab
- Backend hỗ trợ nhiều kết nối đồng thời
- Có thể mở nhiều tab cùng lúc để test

## Tài khoản mẫu để test

### Users:
- `nguyenvanan@example.com` / `password123` (U0001)
- `tranthibinh@example.com` / `password123` (U0002)
- `levancuong@example.com` / `password123` (U0003)

### Trạm y tế:
- `cho-ray@example.com` / `password123` (S0001)
- `benhvien115@example.com` / `password123` (S0002)

### Trạm cứu hộ:
- `cuuhocq1@example.com` / `password123` (S0006)
- `cuuhoccaotoc@example.com` / `password123` (S0008)

