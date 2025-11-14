# Hướng dẫn Sửa lỗi Vị trí không cập nhật trên Map

## ✅ Đã sửa

1. **Thêm logging chi tiết** - Để debug dễ hơn
2. **Cải thiện key cho marker** - Force re-render khi vị trí thay đổi
3. **Tối ưu logic cập nhật** - Chỉ cập nhật khi vị trí thực sự thay đổi

## 🔍 Kiểm tra

### Bước 1: Mở Browser Console

**Mở:** `https://hopdenthongminh.cloud`

**Mở DevTools (F12) → Console**

### Bước 2: Kiểm tra WebSocket

**Phải thấy:**
```
[MapPage] 🔍 Connecting to WebSocket: wss://hopdenthongminh.cloud
[MapPage] ✅ WebSocket connected: [socket_id]
```

**Nếu không thấy:**
- WebSocket URL sai → Cần tạo file `.env`

### Bước 3: Đợi ESP32 gửi dữ liệu

**Sau 30 giây (khi ESP32 gửi), phải thấy:**
```
[MapPage] 📡 Received user:update event: {id: "U0001", lat: 21.1528, lon: 105.9521, ...}
[MapPage] 🔍 Current profile ID: U0001
[MapPage] 🔍 Update data ID: U0001
[MapPage] ✅ Updating profile location: {old: {...}, new: {...}}
[MapPage] ✅ Profile updated, userCurrentLocation set to: [21.1528, 105.9521]
[MapPage] ✅ Map data reloaded
```

### Bước 4: Kiểm tra Map

**Marker trên map phải:**
- Di chuyển đến vị trí mới
- Cập nhật ngay lập tức

**UserSidePanel phải:**
- Hiển thị vị trí mới trong "Thông tin cá nhân"

---

## 🛠️ Nếu vẫn không cập nhật

### Kiểm tra 1: WebSocket có kết nối không?

**Xem Console:**
- Có log "WebSocket connected" không?
- Có log "Received user:update event" không?

**Nếu không:**
- Tạo file `frontend/.env` với `VITE_WS_URL=wss://hopdenthongminh.cloud`
- Restart frontend

### Kiểm tra 2: Profile ID có khớp không?

**Xem Console:**
- `Current profile ID` và `Update data ID` có giống nhau không?

**Nếu khác:**
- Đảm bảo đăng nhập đúng user (U0001)

### Kiểm tra 3: Marker có re-render không?

**Xem Console:**
- Có log "Updating userCurrentLocation from profile" không?

**Nếu không:**
- Kiểm tra `profile.lat` và `profile.lon` có giá trị không

### Kiểm tra 4: Map có reload không?

**Xem Console:**
- Có log "Map data reloaded" không?

**Nếu không:**
- Kiểm tra `loadMapData()` có được gọi không

---

## 📋 Checklist

```
☐ File frontend/.env đã tạo với VITE_WS_URL=wss://hopdenthongminh.cloud
☐ Frontend đã restart sau khi tạo .env
☐ Browser Console thấy "WebSocket connected"
☐ Browser Console thấy "Received user:update event" khi ESP32 gửi
☐ Browser Console thấy "Profile updated"
☐ Browser Console thấy "Map data reloaded"
☐ Marker trên map di chuyển đến vị trí mới
☐ UserSidePanel hiển thị vị trí mới
```

---

## 🚀 Test

1. **Mở Browser Console (F12)**
2. **Reload trang**
3. **Xem log WebSocket connection**
4. **Đợi ESP32 gửi dữ liệu (30 giây)**
5. **Xem log "Received user:update event"**
6. **Kiểm tra marker trên map có di chuyển không**

---

**Hãy kiểm tra Browser Console và cho tôi biết bạn thấy log gì! 🔍**

