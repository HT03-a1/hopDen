# Hướng dẫn Đổi Cloudflare từ HTTPS về HTTP

## ⚠️ Lưu ý

**Không khuyến nghị** đổi về HTTP vì:
- ❌ Không bảo mật
- ❌ Dữ liệu không được mã hóa
- ❌ Trình duyệt có thể cảnh báo

**Nhưng nếu cần** (ví dụ: ESP32 không hỗ trợ HTTPS tốt), có thể làm như sau:

---

## Cách 1: Tắt SSL/TLS (Không khuyến nghị)

### Bước 1: Vào Cloudflare Dashboard

1. Đăng nhập: https://dash.cloudflare.com/
2. Chọn domain: `hopdenthongminh.cloud`

### Bước 2: Vào SSL/TLS Settings

1. Vào **SSL/TLS** → **Overview**
2. Chọn **Off (not secure)**
3. Lưu thay đổi

**⚠️ Cảnh báo:** Điều này sẽ tắt SSL hoàn toàn và website sẽ không an toàn!

---

## Cách 2: Dùng HTTP cho ESP32, HTTPS cho Web (Khuyến nghị) ⭐

### Giải pháp tốt hơn:

**Tạo subdomain riêng cho ESP32 dùng HTTP:**

1. **Vào Cloudflare Dashboard**
2. **Vào DNS** → **Records**
3. **Thêm record mới:**
   - **Type**: A hoặc CNAME
   - **Name**: `api-http` (hoặc tên khác)
   - **Target**: Trỏ về cùng IP với `api.hopdenthongminh.cloud`
   - **Proxy status**: **DNS only** (quan trọng - tắt proxy để dùng HTTP trực tiếp)

4. **Cấu hình Cloudflare Tunnel:**
   - Thêm route mới cho `api-http.hopdenthongminh.cloud`
   - Service: `http://localhost:3000`

5. **Cập nhật ESP32 code:**
   ```cpp
   const char server[] = "api-http.hopdenthongminh.cloud";
   const int serverPort = 80;  // HTTP
   ```

**Lợi ích:**
- ✅ Website vẫn dùng HTTPS (bảo mật)
- ✅ ESP32 dùng HTTP (dễ kết nối)
- ✅ Tách biệt rõ ràng

---

## Cách 3: Dùng Port 80 với Cloudflare (Đơn giản nhất)

### Cấu hình Cloudflare Tunnel:

**File config.yml:**
```yaml
tunnel: YOUR_TUNNEL_ID
credentials-file: PATH_TO_CREDENTIALS

ingress:
  # Backend API - HTTP
  - hostname: api-http.hopdenthongminh.cloud
    service: http://localhost:3000
  
  # Backend API - HTTPS (cho web)
  - hostname: api.hopdenthongminh.cloud
    service: http://localhost:3000
  
  # Frontend - HTTPS
  - hostname: hopdenthongminh.cloud
    service: http://localhost:5173
  
  # Catch-all
  - service: http_status:404
```

### Cập nhật ESP32:

```cpp
const char server[] = "api-http.hopdenthongminh.cloud";
const int serverPort = 80;  // HTTP
```

---

## Cách 4: Dùng IP trực tiếp (Bypass Cloudflare)

### Nếu bạn có Public IP:

1. **Tìm Public IP của máy tính:**
   ```bash
   curl ifconfig.me
   ```

2. **Cấu hình Port Forwarding trên router:**
   - Port 3000 → IP máy tính

3. **Cập nhật ESP32:**
   ```cpp
   const char server[] = "YOUR_PUBLIC_IP";  // Ví dụ: "123.45.67.89"
   const int serverPort = 3000;
   ```

**⚠️ Lưu ý:**
- Cần mở port trên router
- Cần cấu hình firewall
- IP có thể thay đổi (cần Dynamic DNS)

---

## Khuyến nghị: Giữ HTTPS, sửa ESP32 code

### Thay vì đổi về HTTP, nên:

1. **Kiểm tra TinyGSM có hỗ trợ HTTPS không**
2. **Dùng port 80** - Cloudflare sẽ tự động redirect sang HTTPS
3. **Hoặc cấu hình SSL trong TinyGSM**

### Test với port 80 trước:

**Cập nhật ESP32:**
```cpp
const char server[] = "api.hopdenthongminh.cloud";
const int serverPort = 80;  // Thử port 80, Cloudflare tự redirect
```

Cloudflare thường tự động redirect HTTP → HTTPS, nên có thể dùng port 80.

---

## So sánh các phương án

| Phương án | Bảo mật | Độ khó | Khuyến nghị |
|-----------|---------|--------|-------------|
| **Tắt SSL hoàn toàn** | ❌ Không | Dễ | ❌ Không |
| **Subdomain HTTP riêng** | ⚠️ Một phần | Trung bình | ✅ Có |
| **Port 80 (auto redirect)** | ✅ Có | Dễ | ✅✅ Có |
| **IP trực tiếp** | ❌ Không | Khó | ❌ Không |

---

## Khuyến nghị cuối cùng

**Dùng port 80** - Cloudflare sẽ tự động xử lý:

```cpp
const char server[] = "api.hopdenthongminh.cloud";
const int serverPort = 80;  // HTTP, Cloudflare tự redirect sang HTTPS
```

**Lợi ích:**
- ✅ ESP32 dễ kết nối (HTTP)
- ✅ Cloudflare tự động bảo mật (redirect HTTPS)
- ✅ Website vẫn an toàn
- ✅ Không cần cấu hình phức tạp

---

**Bạn muốn thử phương án nào? Tôi khuyến nghị dùng port 80 trước! 🚀**

