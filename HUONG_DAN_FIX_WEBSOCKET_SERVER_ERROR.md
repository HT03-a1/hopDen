# Hướng dẫn Sửa lỗi WebSocket "server error"

## ❌ Lỗi hiện tại

```
[MapPage] ❌ WebSocket connection error: Error: server error
at XMLHttpRequest.onreadystatechange (traffic.js:1:1178)
```

**Nguyên nhân:** WebSocket đang kết nối sai domain hoặc Cloudflare Tunnel chưa route đúng Socket.IO path.

## ✅ Giải pháp

### Bước 1: Kiểm tra file `.env`

**File `frontend/.env` phải có:**

```env
VITE_API_URL=https://api.hopdenthongminh.cloud/api
VITE_WS_URL=wss://api.hopdenthongminh.cloud
```

**Lưu ý quan trọng:**
- WebSocket URL phải trỏ đến **`api.hopdenthongminh.cloud`** (backend domain), KHÔNG phải `hopdenthongminh.cloud` (frontend domain)
- Dùng `wss://` (WebSocket Secure) cho HTTPS
- Nếu không có `VITE_WS_URL`, code sẽ tự động tạo từ `VITE_API_URL`

### Bước 2: Restart Frontend

```bash
# Dừng frontend (Ctrl+C)
cd frontend
npm run dev
```

### Bước 3: Kiểm tra Browser Console

**Mở:** `https://hopdenthongminh.cloud`

**Mở DevTools (F12) → Console**

**Phải thấy:**
```
[MapPage] 🔍 Connecting to WebSocket: wss://api.hopdenthongminh.cloud
[MapPage] ✅ WebSocket connected: [socket_id]
```

**KHÔNG còn lỗi "server error".**

### Bước 4: Kiểm tra Cloudflare Tunnel

**File `~/.cloudflared/config.yml` phải có:**

```yaml
ingress:
  # Backend API (bao gồm Socket.IO)
  - hostname: api.hopdenthongminh.cloud
    service: http://localhost:3000
  
  # Frontend
  - hostname: hopdenthongminh.cloud
    service: http://localhost:5173
  
  # Catch-all
  - service: http_status:404
```

**Lưu ý:** Socket.IO tự động thêm path `/socket.io/`, không cần route riêng.

### Bước 5: Restart Cloudflare Tunnel

```bash
# Dừng tunnel (Ctrl+C)
cloudflared tunnel run
```

---

## 🔍 Debug

### Kiểm tra Backend có chạy không:

```bash
curl http://localhost:3000/api/health
```

**Phải trả về:**
```json
{"status":"ok","message":"Server is running"}
```

### Kiểm tra Socket.IO endpoint:

Mở browser: `https://api.hopdenthongminh.cloud/socket.io/?EIO=4&transport=polling`

**Phải thấy response từ Socket.IO (không phải 404).**

---

## ✅ Kết quả mong đợi

1. ✅ WebSocket kết nối thành công đến `wss://api.hopdenthongminh.cloud`
2. ✅ Console không còn lỗi "server error"
3. ✅ Vị trí từ ESP32 cập nhật real-time trên map
4. ✅ WebSocket events (`user:update`, `sos:new`, etc.) hoạt động bình thường

