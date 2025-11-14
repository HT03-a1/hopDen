# Hướng dẫn Sửa lỗi WebSocket Timeout

## ❌ Lỗi hiện tại

```
[MapPage] ❌ WebSocket connection error: Error: timeout
```

**Nguyên nhân:** Cloudflare Tunnel có thể không hỗ trợ WebSocket tốt, hoặc Socket.IO cần cấu hình thêm.

## ✅ Giải pháp

### Giải pháp 1: Dùng Polling thay vì WebSocket (Khuyến nghị)

**File `frontend/src/pages/MapPage.tsx`:**

Thay đổi Socket.IO config để ưu tiên polling:

```typescript
const newSocket = io(wsUrl, {
  transports: ['polling', 'websocket'], // Ưu tiên polling trước
  reconnection: true,
  reconnectionDelay: 1000,
  reconnectionAttempts: 5,
  timeout: 20000, // Tăng timeout lên 20 giây
});
```

### Giải pháp 2: Kiểm tra Cloudflare Tunnel config

**File `~/.cloudflared/config.yml`:**

Đảm bảo có route cho WebSocket (thường dùng chung với HTTP):

```yaml
ingress:
  # Backend API (bao gồm WebSocket)
  - hostname: api.hopdenthongminh.cloud
    service: http://localhost:3000
  
  # Frontend
  - hostname: hopdenthongminh.cloud
    service: http://localhost:5173
  
  # Catch-all
  - service: http_status:404
```

**Lưu ý:** Socket.IO thường dùng chung endpoint với HTTP, không cần route riêng.

### Giải pháp 3: Cấu hình Backend Socket.IO

**File `backend/src/index.ts`:**

Đảm bảo Socket.IO được cấu hình đúng:

```typescript
const io = new Server(httpServer, {
  cors: {
    origin: allowedOrigins,
    methods: ["GET", "POST"],
    credentials: true
  },
  transports: ['polling', 'websocket'], // Hỗ trợ cả polling và websocket
  allowEIO3: true, // Tương thích với client cũ
});
```

---

## 🛠️ Sửa ngay

Tôi sẽ cập nhật code để dùng polling làm phương thức chính (ổn định hơn qua Cloudflare Tunnel).

