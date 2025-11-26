# Hướng dẫn Fix Cloudflare Tunnel

## Vấn đề:
Lỗi: `Cannot determine default origin certificate path`

## Giải pháp nhanh:

### Cách 1: Dùng Quick Tunnel (Không cần config)

**Chạy lệnh này trong terminal mới:**

```bash
# Cho Backend API
cloudflared tunnel --url http://localhost:3000

# Hoặc cho Frontend
cloudflared tunnel --url http://localhost:5173
```

**Lưu ý:** Quick tunnel sẽ tạo URL ngẫu nhiên, không phải domain cố định.

---

### Cách 2: Tạo Config File (Cho Named Tunnel)

**Bước 1: Tạo folder config**
```powershell
New-Item -ItemType Directory -Force -Path "$env:USERPROFILE\.cloudflared"
```

**Bước 2: Tạo file config.yml**

Tạo file: `C:\Users\admin\.cloudflared\config.yml`

**Nội dung:**
```yaml
tunnel: <TUNNEL_ID>
credentials-file: C:\Users\admin\.cloudflared\<TUNNEL_ID>.json

ingress:
  - hostname: api.hopdenthongminh.cloud
    service: http://localhost:3000
  - hostname: hopdenthongminh.cloud
    service: http://localhost:5173
  - service: http_status:404
```

**Bước 3: Lấy Tunnel ID**

Chạy lệnh:
```bash
cloudflared tunnel list
```

Hoặc vào Cloudflare Dashboard → Zero Trust → Networks → Tunnels

**Bước 4: Download credentials file**

Từ Cloudflare Dashboard, download file `.json` và đặt vào:
`C:\Users\admin\.cloudflared\<TUNNEL_ID>.json`

**Bước 5: Chạy tunnel**
```bash
cloudflared tunnel run <TUNNEL_NAME>
```

---

### Cách 3: Kiểm tra Service đang chạy

Nếu tunnel đã cài như service:

```cmd
# Kiểm tra
sc query cloudflared

# Restart service
net stop cloudflared
net start cloudflared
```

---

## Khuyến nghị:

**Nếu cần nhanh:** Dùng Quick Tunnel tạm thời
**Nếu cần ổn định:** Tạo config file đúng cách

