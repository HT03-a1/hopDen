# Hướng dẫn Sửa lỗi Vị trí không cập nhật từ ESP32

## 🔍 Nguyên nhân có thể

1. **ESP32 không gửi được dữ liệu lên server** (Status Code -2)
2. **WebSocket không kết nối được** (Frontend không nhận event)
3. **Backend không emit WebSocket event**
4. **Frontend không listen đúng event**

---

## ✅ Kiểm tra từng bước

### Bước 1: Kiểm tra ESP32 có gửi được dữ liệu không

**Xem Serial Monitor ESP32:**
- Status Code phải là **200** hoặc **201** (không phải -2)
- Nếu là -2 → ESP32 không kết nối được server

**Giải pháp:**
- Kiểm tra SIM 4G đã kết nối mạng chưa
- Kiểm tra URL và port đúng chưa
- Kiểm tra backend đang chạy

### Bước 2: Kiểm tra Backend có nhận được dữ liệu

**Xem terminal Backend:**
```bash
cd backend
npm run dev
```

**Khi ESP32 gửi dữ liệu, phải thấy:**
```
[TELEMETRY] Nhận dữ liệu: { deviceId: 'DHW001', userId: 'U0001', ... }
[TELEMETRY] ✅ Đã cập nhật vị trí user U0001: ...
[TELEMETRY] 📡 Emitting user:update WebSocket event
```

**Nếu không thấy:**
- ESP32 chưa gửi được
- Hoặc request bị lỗi

### Bước 3: Kiểm tra WebSocket kết nối

**Mở Browser DevTools (F12):**
1. Vào tab **Console**
2. Tìm log về WebSocket connection
3. Vào tab **Network** → **WS**
4. Kiểm tra có kết nối WebSocket không

**Nếu không có WebSocket:**
- Kiểm tra WebSocket URL trong frontend
- Kiểm tra Cloudflare Tunnel có hỗ trợ WebSocket không

### Bước 4: Kiểm tra Frontend có nhận event không

**Thêm log vào MapPage.tsx:**

Tìm dòng:
```typescript
newSocket.on('user:update', async (data) => {
```

Thêm log:
```typescript
newSocket.on('user:update', async (data) => {
  console.log('[WebSocket] Received user:update:', data);
  // ... code hiện tại
});
```

**Reload trang và xem Console:**
- Nếu thấy log → WebSocket hoạt động
- Nếu không thấy → WebSocket không kết nối hoặc không nhận event

---

## 🛠️ Sửa lỗi

### Lỗi 1: ESP32 Status Code -2

**Nguyên nhân:** Không kết nối được server

**Giải pháp:**
1. Kiểm tra SIM 4G đã kết nối mạng
2. Kiểm tra URL: `api.hopdenthongminh.cloud`
3. Kiểm tra port: `80`
4. Kiểm tra backend đang chạy

### Lỗi 2: WebSocket không kết nối

**Nguyên nhân:** WebSocket URL sai hoặc Cloudflare Tunnel chưa route

**Giải pháp:**

**1. Kiểm tra file `.env` frontend:**
```env
VITE_WS_URL=wss://hopdenthongminh.cloud
```

**2. Kiểm tra WebSocket URL trong code:**
```typescript
const wsUrl = import.meta.env.VITE_WS_URL || 'http://localhost:3000';
```

**3. Restart frontend sau khi sửa `.env`**

### Lỗi 3: Backend không emit event

**Kiểm tra backend logs:**
- Phải thấy: `[TELEMETRY] 📡 Emitting user:update WebSocket event`
- Nếu không thấy → Backend không emit

**Giải pháp:**
- Kiểm tra Socket.IO instance có sẵn không
- Kiểm tra `req.app.get('io')` không null

### Lỗi 4: Frontend không cập nhật map

**Nguyên nhân:** `loadMapData()` không reload đúng

**Giải pháp:**
- Kiểm tra `loadMapData()` có được gọi không
- Kiểm tra `entities` state có cập nhật không
- Kiểm tra marker có re-render không

---

## 🔧 Debug chi tiết

### Thêm logging vào Backend

**File `backend/src/routes/telemetry.ts`:**

Thêm log sau dòng 144:
```typescript
io.emit('user:update', updateData);
console.log('[TELEMETRY] 🔍 Debug - Emitted data:', JSON.stringify(updateData));
console.log('[TELEMETRY] 🔍 Debug - Connected clients:', (io as any).sockets?.sockets?.size || 'unknown');
```

### Thêm logging vào Frontend

**File `frontend/src/pages/MapPage.tsx`:**

Thêm log trong `user:update` handler:
```typescript
newSocket.on('user:update', async (data) => {
  console.log('[MapPage] 🔍 Received user:update:', data);
  console.log('[MapPage] 🔍 Current profile ID:', currentProfile?.id);
  console.log('[MapPage] 🔍 Data ID:', data.id);
  
  // ... code hiện tại
});
```

### Kiểm tra WebSocket connection

**Thêm vào MapPage.tsx:**
```typescript
useEffect(() => {
  const wsUrl = import.meta.env.VITE_WS_URL || 'http://localhost:3000';
  console.log('[MapPage] 🔍 Connecting to WebSocket:', wsUrl);
  const newSocket = io(wsUrl);

  newSocket.on('connect', () => {
    console.log('[MapPage] ✅ WebSocket connected:', newSocket.id);
  });

  newSocket.on('disconnect', () => {
    console.log('[MapPage] ❌ WebSocket disconnected');
  });

  newSocket.on('connect_error', (error) => {
    console.error('[MapPage] ❌ WebSocket connection error:', error);
  });

  // ... các event handlers khác
}, []);
```

---

## ✅ Checklist Debug

```
☐ ESP32 gửi được dữ liệu (Status Code 200/201)
☐ Backend nhận được dữ liệu (xem terminal logs)
☐ Backend cập nhật users.json
☐ Backend emit WebSocket event (xem terminal logs)
☐ Frontend WebSocket kết nối được (xem Browser Console)
☐ Frontend nhận được user:update event (xem Browser Console)
☐ Frontend cập nhật profile trong store
☐ Frontend reload map data
☐ Marker trên map cập nhật vị trí mới
```

---

## 🚀 Test nhanh

### Test 1: ESP32 → Backend

**Xem Serial Monitor ESP32:**
- Status Code: 200/201 ✅
- Response: Có dữ liệu ✅

### Test 2: Backend → WebSocket

**Xem terminal Backend:**
- Thấy log emit event ✅
- Không có lỗi ✅

### Test 3: WebSocket → Frontend

**Xem Browser Console:**
- WebSocket connected ✅
- Nhận được user:update event ✅
- Profile được cập nhật ✅

### Test 4: Frontend → Map

**Xem map:**
- Marker di chuyển đến vị trí mới ✅
- Vị trí trong UserSidePanel cập nhật ✅

---

## 💡 Lưu ý

1. **WebSocket URL phải đúng:**
   - Development: `ws://localhost:3000`
   - Production: `wss://hopdenthongminh.cloud`

2. **Cloudflare Tunnel phải hỗ trợ WebSocket:**
   - Kiểm tra config.yml có route WebSocket không
   - WebSocket thường dùng chung endpoint với HTTP

3. **Backend phải chạy:**
   - Nếu backend dừng, WebSocket sẽ disconnect
   - Frontend sẽ không nhận được event

4. **ESP32 phải gửi đúng format:**
   - `userId`: Phải khớp với user trong `users.json`
   - `lat`, `lon`: Phải là số hợp lệ

---

**Hãy kiểm tra từng bước và cho tôi biết kết quả! 🔍**

