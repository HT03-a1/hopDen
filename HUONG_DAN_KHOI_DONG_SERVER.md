# Hướng dẫn Khởi động Server

## 📋 Tổng quan

Để website hoạt động, bạn cần khởi động 3 thành phần:

1. **Backend** (Node.js) - Port 3000
2. **Frontend** (Vite) - Port 5173
3. **Cloudflare Tunnel** - Kết nối local với domain

---

## 🚀 Các bước khởi động

### Bước 1: Khởi động Backend

**Mở Terminal 1 (Command Prompt hoặc PowerShell):**

```bash
cd C:\Users\admin\Downloads\hopDen\backend
npm run dev
```

**Kết quả mong đợi:**
```
[nodemon] starting `ts-node src/index.ts`
Server is running on http://localhost:3000
```

**✅ Dấu hiệu thành công:**
- Thấy dòng "Server is running on http://localhost:3000"
- Không có lỗi đỏ

**⚠️ Lưu ý:**
- Giữ terminal này mở (không đóng)
- Nếu có lỗi, xem terminal để biết chi tiết

---

### Bước 2: Khởi động Frontend

**Mở Terminal 2 mới (Command Prompt hoặc PowerShell):**

```bash
cd C:\Users\admin\Downloads\hopDen\frontend
npm run dev
```

**Kết quả mong đợi:**
```
  VITE v5.x.x  ready in xxx ms

  ➜  Local:   http://localhost:5173/
  ➜  Network: http://0.0.0.0:5173/
```

**✅ Dấu hiệu thành công:**
- Thấy "ready in xxx ms"
- Hiển thị URL local và network

**⚠️ Lưu ý:**
- Giữ terminal này mở (không đóng)
- Frontend sẽ tự động reload khi sửa code

---

### Bước 3: Khởi động Cloudflare Tunnel

**Kiểm tra Tunnel đang chạy (Service):**

```cmd
sc query cloudflared
```

**Nếu đã cài như service:**
- Service sẽ tự động chạy khi khởi động máy
- Không cần chạy thủ công

**Nếu chưa cài service hoặc muốn chạy thủ công:**

**Mở Terminal 3 mới:**

```bash
cloudflared tunnel run hopden-tunnel
```

**Kết quả mong đợi:**
```
+--------------------------------------------------------------------------------------------+
|  Your quick Tunnel has been created! Visit it at (it may take some time to be reachable): |
|  https://hopdenthongminh.cloud                                                              |
+--------------------------------------------------------------------------------------------+
```

**✅ Dấu hiệu thành công:**
- Thấy thông báo tunnel đã tạo
- Hiển thị URL domain

**⚠️ Lưu ý:**
- Giữ terminal này mở (không đóng)
- Nếu đóng, website sẽ không truy cập được từ domain

---

## ✅ Kiểm tra tất cả đã chạy

### 1. Kiểm tra Backend (Port 3000)

**Mở browser:**
```
http://localhost:3000/api/health
```

**Hoặc:**
```
https://api.hopdenthongminh.cloud/api/health
```

**Kết quả mong đợi:**
```json
{"status":"ok","message":"Server is running"}
```

### 2. Kiểm tra Frontend (Port 5173)

**Mở browser:**
```
http://localhost:5173
```

**Hoặc:**
```
https://hopdenthongminh.cloud
```

**Kết quả mong đợi:**
- Hiển thị trang login hoặc trang map (nếu đã đăng nhập)

### 3. Kiểm tra Cloudflare Tunnel

**Mở browser:**
```
https://hopdenthongminh.cloud
```

**Kết quả mong đợi:**
- Website load được
- Không có lỗi "Connection refused" hoặc "502 Bad Gateway"

---

## 📋 Checklist khởi động

```
☐ Terminal 1: Backend đang chạy (npm run dev)
☐ Terminal 2: Frontend đang chạy (npm run dev)
☐ Terminal 3: Cloudflare Tunnel đang chạy (hoặc service)
☐ Test http://localhost:3000/api/health → OK
☐ Test http://localhost:5173 → OK
☐ Test https://hopdenthongminh.cloud → OK
☐ Đăng nhập thành công
```

---

## 🔄 Khởi động nhanh (Script)

### Tạo file batch (Windows)

**Tạo file:** `start-server.bat`

**Nội dung:**
```batch
@echo off
echo Starting Backend...
start "Backend" cmd /k "cd /d C:\Users\admin\Downloads\hopDen\backend && npm run dev"

timeout /t 3 /nobreak >nul

echo Starting Frontend...
start "Frontend" cmd /k "cd /d C:\Users\admin\Downloads\hopDen\frontend && npm run dev"

timeout /t 3 /nobreak >nul

echo Starting Cloudflare Tunnel...
start "Cloudflare Tunnel" cmd /k "cloudflared tunnel run hopden-tunnel"

echo All servers started!
pause
```

**Cách dùng:**
1. Double-click file `start-server.bat`
2. Sẽ mở 3 cửa sổ terminal tự động

### Tạo file PowerShell

**Tạo file:** `start-server.ps1`

**Nội dung:**
```powershell
# Start Backend
Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd C:\Users\admin\Downloads\hopDen\backend; npm run dev"

Start-Sleep -Seconds 3

# Start Frontend
Start-Process powershell -ArgumentList "-NoExit", "-Command", "cd C:\Users\admin\Downloads\hopDen\frontend; npm run dev"

Start-Sleep -Seconds 3

# Start Cloudflare Tunnel
Start-Process powershell -ArgumentList "-NoExit", "-Command", "cloudflared tunnel run hopden-tunnel"

Write-Host "All servers started!"
```

**Cách dùng:**
```powershell
.\start-server.ps1
```

---

## 🛑 Dừng Server

### Dừng từng phần:

**Backend:**
- Vào Terminal 1
- Nhấn `Ctrl + C`

**Frontend:**
- Vào Terminal 2
- Nhấn `Ctrl + C`

**Cloudflare Tunnel:**
- Vào Terminal 3
- Nhấn `Ctrl + C`

**Hoặc nếu chạy service:**
```cmd
net stop cloudflared
```

---

## 🔧 Cài Cloudflare Tunnel như Service (Tự động chạy)

### Windows:

```cmd
cloudflared service install
cloudflared service start
```

**Kiểm tra:**
```cmd
sc query cloudflared
```

**Lợi ích:**
- Tự động chạy khi khởi động máy
- Không cần mở terminal thủ công
- Chạy nền (background)

**Dừng service:**
```cmd
net stop cloudflared
```

**Bắt đầu lại:**
```cmd
net start cloudflared
```

---

## 📝 Thứ tự khởi động (Khuyến nghị)

1. **Backend trước** (vì frontend cần API)
2. **Frontend sau** (có thể chạy song song)
3. **Cloudflare Tunnel cuối** (hoặc chạy service)

---

## ⚠️ Lưu ý quan trọng

1. **Phải chạy cả 3 thành phần** để website hoạt động đầy đủ
2. **Giữ các terminal mở** (không đóng)
3. **Nếu đóng terminal**, service sẽ dừng
4. **Cloudflare Tunnel** có thể cài như service để tự động chạy
5. **Kiểm tra port** nếu bị lỗi "port already in use"

---

## 🐛 Troubleshooting

### Lỗi "Port 3000 already in use"

**Giải pháp:**
```cmd
# Tìm process
netstat -ano | findstr :3000

# Kill process
taskkill /PID <PID> /F
```

### Lỗi "Port 5173 already in use"

**Giải pháp:**
```cmd
# Tìm process
netstat -ano | findstr :5173

# Kill process
taskkill /PID <PID> /F
```

### Lỗi "Cannot find module"

**Giải pháp:**
```bash
cd backend
npm install

cd ../frontend
npm install
```

### Cloudflare Tunnel không kết nối

**Giải pháp:**
1. Kiểm tra config: `cloudflared tunnel ingress validate`
2. Kiểm tra tunnel đang chạy: `cloudflared tunnel list`
3. Restart tunnel: `cloudflared tunnel run hopden-tunnel`

---

## 🎯 Quick Start (Tóm tắt)

**3 lệnh cần chạy:**

```bash
# Terminal 1 - Backend
cd backend && npm run dev

# Terminal 2 - Frontend  
cd frontend && npm run dev

# Terminal 3 - Cloudflare Tunnel (nếu chưa cài service)
cloudflared tunnel run hopden-tunnel
```

**Hoặc dùng script tự động** (xem phần trên)

---

**Chúc bạn khởi động thành công! 🚀**

