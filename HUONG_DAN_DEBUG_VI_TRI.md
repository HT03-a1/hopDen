# Hướng dẫn Debug Vị trí không cập nhật

## ✅ Backend đã hoạt động đúng!

Từ logs bạn cung cấp:
```
[TELEMETRY] 📡 Emitting user:update WebSocket event
[TELEMETRY] ✅ Đã cập nhật vị trí user U0001
```

**Backend đã:**
- ✅ Nhận dữ liệu từ ESP32
- ✅ Cập nhật users.json
- ✅ Emit WebSocket event

## 🔍 Vấn đề có thể ở Frontend

### Bước 1: Kiểm tra WebSocket URL

**Mở Browser Console (F12):**

Phải thấy log:
```
[MapPage] 🔍 Connecting to WebSocket: wss://hopdenthongminh.cloud
[MapPage] ✅ WebSocket connected: [socket_id]
```

**Nếu thấy:**
```
[MapPage] 🔍 Connecting to WebSocket: http://localhost:3000
```
→ **WebSocket URL sai!** Cần tạo file `.env`

### Bước 2: Tạo file `.env` cho Frontend

**Tạo file:** `frontend/.env`

**Nội dung:**
```env
VITE_API_URL=https://api.hopdenthongminh.cloud/api
VITE_WS_URL=wss://hopdenthongminh.cloud
```

**Lưu ý:**
- Phải có prefix `VITE_`
- WebSocket URL phải là `wss://` (WebSocket Secure), không phải `ws://`

### Bước 3: Restart Frontend

**Dừng frontend:**
- Nhấn `Ctrl + C` trong terminal

**Chạy lại:**
```bash
cd frontend
npm run dev
```

### Bước 4: Kiểm tra Browser Console

**Mở:** `https://hopdenthongminh.cloud`

**Xem Console (F12):**

**Phải thấy:**
```
[MapPage] 🔍 Connecting to WebSocket: wss://hopdenthongminh.cloud
[MapPage] ✅ WebSocket connected: [id]
```

**Khi ESP32 gửi dữ liệu, phải thấy:**
```
[MapPage] 📡 Received user:update event: {id: "U0001", lat: 21.106, lon: 105.8707, ...}
[MapPage] 🔍 Current profile ID: U0001
[MapPage] 🔍 Update data ID: U0001
[MapPage] ✅ Updating profile location: {...}
[MapPage] ✅ Profile updated, userCurrentLocation set to: [21.106, 105.8707]
```

---

## 🛠️ Nếu vẫn không thấy log

### Kiểm tra WebSocket connection:

1. **Mở DevTools (F12)**
2. **Vào tab Network → WS**
3. **Reload trang**
4. **Kiểm tra có kết nối WebSocket không**

**Nếu không có:**
- WebSocket URL sai
- Cloudflare Tunnel chưa route WebSocket
- Backend Socket.IO chưa chạy

### Kiểm tra Cloudflare Tunnel config:

**File `~/.cloudflared/config.yml`:**

Đảm bảo có route cho WebSocket:
```yaml
ingress:
  - hostname: hopdenthongminh.cloud
    service: http://localhost:5173
  # WebSocket thường dùng chung endpoint với HTTP
```

**Lưu ý:** Socket.IO thường dùng chung endpoint với HTTP API, không cần route riêng.

---

## ✅ Checklist

```
☐ File frontend/.env đã tạo
☐ VITE_WS_URL=wss://hopdenthongminh.cloud
☐ Đã restart frontend sau khi tạo .env
☐ Browser Console thấy "WebSocket connected"
☐ Browser Console thấy "Received user:update event" khi ESP32 gửi
☐ Map marker di chuyển đến vị trí mới
☐ UserSidePanel hiển thị vị trí mới
```

---

## 🚀 Test nhanh

1. **Mở Browser Console (F12)**
2. **Reload trang**
3. **Xem log WebSocket connection**
4. **Đợi ESP32 gửi dữ liệu (30 giây)**
5. **Xem log "Received user:update event"**

**Nếu thấy log → WebSocket hoạt động!**
**Nếu không thấy → Kiểm tra WebSocket URL và connection**

---

**Hãy kiểm tra Browser Console và cho tôi biết bạn thấy log gì! 🔍**

