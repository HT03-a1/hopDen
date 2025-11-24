# HƯỚNG DẪN PHÁT TRIỂN APP VỚI MIT APP INVENTOR

## Tổng quan

MIT App Inventor là công cụ kéo-thả để phát triển ứng dụng Android/iOS. Bạn có thể tích hợp app với backend hiện tại để:

- Xem vị trí thiết bị trên bản đồ
- Tạo SOS từ app
- Xem danh sách SOS
- Cập nhật vị trí từ app
- Nhận thông báo real-time qua WebSocket

## Backend API Endpoints

### Base URL
```
http://hopdenthongminh.cloud:3000
hoặc
https://hopdenthongminh.cloud:3000
```

### 1. Authentication

#### Đăng nhập
```
POST /api/auth/login
Body: {
  "email": "user@example.com",
  "password": "password123"
}
Response: {
  "token": "jwt_token_here",
  "user": { "id": "U0001", "name": "...", ... }
}
```

#### Đăng ký User
```
POST /api/auth/register-user
Body: {
  "email": "user@example.com",
  "password": "password123",
  "name": "Tên người dùng",
  "phone": "0123456789"
}
```

### 2. Map & Entities

#### Lấy tất cả entities trên map
```
GET /api/map/entities?userId=U0001
Response: {
  "sosList": [...],
  "doneSOSList": [...],
  "historySOSList": [...],
  "userList": [...],
  "stationList": [...],
  "deviceList": [...]
}
```

### 3. SOS

#### Tạo SOS mới
```
POST /api/sos
Headers: { "Authorization": "Bearer {token}" }
Body: {
  "userId": "U0001",
  "type": "accident" | "breakdown" | "medical" | "other",
  "severity": "critical" | "high" | "medium" | "low",
  "location": {
    "lat": 21.0285,
    "lon": 105.8542
  },
  "note": "Ghi chú..."
}
Response: {
  "id": "SOS123...",
  "status": "pending",
  ...
}
```

#### Lấy danh sách SOS
```
GET /api/sos?userId=U0001&status=pending
Response: [ { "id": "...", "status": "...", ... }, ... ]
```

#### Lấy chi tiết SOS
```
GET /api/sos/:id
Response: { "id": "...", "status": "...", ... }
```

#### Cập nhật trạng thái SOS
```
PATCH /api/sos/:id/status
Body: {
  "status": "pending" | "accepted" | "on_route" | "done" | "cancelled"
}
```

### 4. Telemetry (Cập nhật vị trí)

#### Gửi vị trí từ app
```
POST /api/telemetry
Body: {
  "userId": "U0001",
  "deviceId": "MOBILE_APP",
  "lat": 21.0285,
  "lon": 105.8542,
  "speed": 0.0,
  "source": "mobile",
  "timestamp": "2025-01-15T10:30:00.000Z"
}
```

### 5. Stations

#### Tìm trạm gần nhất
```
GET /api/stations/nearest?lat=21.0285&lon=105.8542&type=medical
Response: { "id": "...", "name": "...", "distance": 1234, ... }
```

## Hướng dẫn tích hợp MIT App Inventor

### Bước 1: Tạo Project mới

1. Truy cập: https://appinventor.mit.edu/
2. Tạo project mới
3. Đặt tên: "HopDenThongMinh"

### Bước 2: Thêm Components cần thiết

#### Components cần thêm:
- **Web**: Để gửi HTTP requests
- **LocationSensor**: Để lấy GPS từ điện thoại
- **Map**: Để hiển thị bản đồ (nếu cần)
- **Notifier**: Để hiển thị thông báo
- **WebViewer**: Để hiển thị web (nếu muốn dùng web thay vì native)

### Bước 3: Cấu hình Web Component

#### Base URL
```
http://hopdenthongminh.cloud:3000
```

### Bước 4: Blocks Code mẫu

#### 4.1. Đăng nhập

```
Khi ButtonLogin.Click:
  - Gọi Web1.PostText
    - URL: "http://hopdenthongminh.cloud:3000/api/auth/login"
    - RequestHeaders: [["Content-Type", "application/json"]]
    - TextToPost: {
        "email": TextBoxEmail.Text,
        "password": TextBoxPassword.Text
      }
  
Khi Web1.GotText:
  - Nếu response code = 200:
    - Parse JSON response
    - Lưu token vào TinyDB
    - Chuyển sang màn hình chính
  - Nếu không:
    - Hiển thị lỗi bằng Notifier
```

#### 4.2. Lấy vị trí GPS và gửi lên server

```
Khi ButtonUpdateLocation.Click:
  - Nếu LocationSensor1.HasLongitudeLatitude:
    - Gọi Web1.PostText
      - URL: "http://hopdenthongminh.cloud:3000/api/telemetry"
      - RequestHeaders: [["Content-Type", "application/json"]]
      - TextToPost: {
          "userId": TinyDB1.GetValue("userId"),
          "deviceId": "MOBILE_APP",
          "lat": LocationSensor1.Latitude,
          "lon": LocationSensor1.Longitude,
          "speed": 0.0,
          "source": "mobile",
          "timestamp": (lấy thời gian hiện tại)
        }
  
Khi Web1.GotText:
  - Nếu response code = 201:
    - Hiển thị "Đã cập nhật vị trí thành công"
  - Nếu không:
    - Hiển thị lỗi
```

#### 4.3. Tạo SOS

```
Khi ButtonCreateSOS.Click:
  - Nếu LocationSensor1.HasLongitudeLatitude:
    - Gọi Web1.PostText
      - URL: "http://hopdenthongminh.cloud:3000/api/sos"
      - RequestHeaders: [
          ["Content-Type", "application/json"],
          ["Authorization", "Bearer " + TinyDB1.GetValue("token")]
        ]
      - TextToPost: {
          "userId": TinyDB1.GetValue("userId"),
          "type": ListPickerType.Selection,
          "severity": ListPickerSeverity.Selection,
          "location": {
            "lat": LocationSensor1.Latitude,
            "lon": LocationSensor1.Longitude
          },
          "note": TextBoxNote.Text
        }
  
Khi Web1.GotText:
  - Nếu response code = 201:
    - Parse JSON để lấy SOS ID
    - Hiển thị "SOS đã được tạo thành công"
    - Lưu SOS ID vào TinyDB
  - Nếu không:
    - Hiển thị lỗi
```

#### 4.4. Lấy danh sách SOS

```
Khi ButtonRefreshSOS.Click:
  - Gọi Web1.Get
    - URL: "http://hopdenthongminh.cloud:3000/api/sos?userId=" + TinyDB1.GetValue("userId")
    - RequestHeaders: [
        ["Authorization", "Bearer " + TinyDB1.GetValue("token")]
      ]
  
Khi Web1.GotText:
  - Parse JSON response (mảng)
  - Hiển thị vào ListView
```

#### 4.5. Lấy entities trên map

```
Khi ButtonLoadMap.Click:
  - Gọi Web1.Get
    - URL: "http://hopdenthongminh.cloud:3000/api/map/entities?userId=" + TinyDB1.GetValue("userId")
  
Khi Web1.GotText:
  - Parse JSON response
  - Hiển thị:
    - SOS markers trên map
    - User markers
    - Station markers
    - Device markers
```

### Bước 5: Cấu hình LocationSensor

- **Enabled**: true
- **TimeInterval**: 30000 (30 giây)
- **DistanceInterval**: 10 (10 mét)

### Bước 6: Xử lý JSON Response

MIT App Inventor có component **Web** với khả năng parse JSON:

```
Khi Web1.GotText:
  - Dùng block "get value from JSON" để parse
  - Ví dụ: get value "id" from JSON response
```

### Bước 7: Lưu trữ dữ liệu

Sử dụng **TinyDB** để lưu:
- Token (sau khi đăng nhập)
- UserId
- SOS ID
- Các dữ liệu khác

### Bước 8: Real-time Updates (WebSocket)

MIT App Inventor không hỗ trợ WebSocket trực tiếp, nhưng có thể:
- Polling định kỳ (gọi GET mỗi 5-10 giây)
- Hoặc dùng WebViewer để hiển thị web version (có WebSocket)

## Ví dụ Blocks Code chi tiết

### Đăng nhập

```
Khi ButtonLogin.Click:
  - Đặt Web1.Url = "http://hopdenthongminh.cloud:3000/api/auth/login"
  - Đặt Web1.RequestHeaders = [["Content-Type", "application/json"]]
  - Gọi Web1.PostText với TextToPost = {
      "email": TextBoxEmail.Text,
      "password": TextBoxPassword.Text
    }

Khi Web1.GotText:
  - Nếu Web1.ResponseCode = 200:
    - Đặt jsonResponse = Web1.ResponseText
    - Đặt token = get value "token" from jsonResponse
    - Đặt user = get value "user" from jsonResponse
    - Đặt userId = get value "id" from user
    - Gọi TinyDB1.StoreValue với tag "token" và value token
    - Gọi TinyDB1.StoreValue với tag "userId" và value userId
    - Gọi Screen1.SwitchTo với ScreenName = "MainScreen"
  - Nếu không:
    - Gọi Notifier1.ShowAlert với message = "Đăng nhập thất bại"
```

### Gửi vị trí GPS

```
Khi LocationSensor1.LocationChanged:
  - Nếu LocationSensor1.HasLongitudeLatitude:
    - Đặt Web1.Url = "http://hopdenthongminh.cloud:3000/api/telemetry"
    - Đặt Web1.RequestHeaders = [["Content-Type", "application/json"]]
    - Đặt jsonBody = {
        "userId": TinyDB1.GetValue("userId"),
        "deviceId": "MOBILE_APP",
        "lat": LocationSensor1.Latitude,
        "lon": LocationSensor1.Longitude,
        "speed": 0.0,
        "source": "mobile",
        "timestamp": Clock1.Now
      }
    - Gọi Web1.PostText với TextToPost = jsonBody

Khi Web1.GotText:
  - Nếu Web1.ResponseCode = 201:
    - Gọi Notifier1.ShowToast với message = "Đã cập nhật vị trí"
  - Nếu không:
    - Gọi Notifier1.ShowAlert với message = "Lỗi: " + Web1.ResponseText
```

### Tạo SOS

```
Khi ButtonCreateSOS.Click:
  - Nếu LocationSensor1.HasLongitudeLatitude:
    - Đặt Web1.Url = "http://hopdenthongminh.cloud:3000/api/sos"
    - Đặt token = TinyDB1.GetValue("token")
    - Đặt Web1.RequestHeaders = [
        ["Content-Type", "application/json"],
        ["Authorization", "Bearer " + token]
      ]
    - Đặt jsonBody = {
        "userId": TinyDB1.GetValue("userId"),
        "type": ListPickerType.Selection,
        "severity": ListPickerSeverity.Selection,
        "location": {
          "lat": LocationSensor1.Latitude,
          "lon": LocationSensor1.Longitude
        },
        "note": TextBoxNote.Text
      }
    - Gọi Web1.PostText với TextToPost = jsonBody
  - Nếu không:
    - Gọi Notifier1.ShowAlert với message = "Chưa có GPS"

Khi Web1.GotText:
  - Nếu Web1.ResponseCode = 201:
    - Đặt jsonResponse = Web1.ResponseText
    - Đặt sosId = get value "id" from jsonResponse
    - Gọi TinyDB1.StoreValue với tag "lastSOSId" và value sosId
    - Gọi Notifier1.ShowAlert với message = "SOS đã được tạo: " + sosId
  - Nếu không:
    - Gọi Notifier1.ShowAlert với message = "Lỗi: " + Web1.ResponseText
```

## Lưu ý quan trọng

### 1. CORS
Backend đã cấu hình CORS để cho phép requests từ mobile apps. MIT App Inventor sẽ không gặp vấn đề CORS.

### 2. Authentication
- Lưu token sau khi đăng nhập
- Gửi token trong header `Authorization: Bearer {token}` cho các API cần auth

### 3. GPS Permissions
- App cần quyền Location
- Kiểm tra `LocationSensor1.HasLongitudeLatitude` trước khi gửi

### 4. Error Handling
- Luôn kiểm tra `Web1.ResponseCode`
- Hiển thị lỗi cho người dùng bằng Notifier

### 5. JSON Parsing
- Dùng block "get value from JSON" để parse response
- Kiểm tra null trước khi sử dụng

## Tính năng nên có trong app

### Cho User (Người dùng thường)
1. **Đăng nhập/Đăng ký**
2. **Hiển thị bản đồ** với vị trí thiết bị
3. **Tạo SOS** (tai nạn, hỏng xe, y tế, khác)
4. **Xem danh sách SOS** của mình
5. **Cập nhật vị trí** tự động (mỗi 30 giây)
6. **Xem trạng thái SOS** (pending, accepted, on_route, done)

### Cho Station (Trạm y tế/Cứu hộ)
1. **Đăng nhập** (với role station)
2. **Xem danh sách SOS** trong khu vực
3. **Nhận nhiệm vụ** (claim SOS)
4. **Cập nhật trạng thái** (on_route, done)
5. **Xem đường đi** đến vị trí SOS
6. **Mở Google Maps** để điều hướng

## Testing

### Test API với Postman/curl trước
```bash
# Test đăng nhập
curl -X POST http://hopdenthongminh.cloud:3000/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"user@example.com","password":"password123"}'

# Test gửi telemetry
curl -X POST http://hopdenthongminh.cloud:3000/api/telemetry \
  -H "Content-Type: application/json" \
  -d '{
    "userId":"U0001",
    "deviceId":"MOBILE_APP",
    "lat":21.0285,
    "lon":105.8542,
    "speed":0.0,
    "source":"mobile"
  }'
```

## Tài liệu tham khảo

- MIT App Inventor Documentation: https://appinventor.mit.edu/explore/ai2/support
- Web Component: https://appinventor.mit.edu/explore/ai2/support/blocks/web
- LocationSensor: https://appinventor.mit.edu/explore/ai2/support/components/sensors#LocationSensor

## Hỗ trợ

Nếu gặp vấn đề:
1. Kiểm tra backend logs
2. Kiểm tra response code và message
3. Test API với Postman trước
4. Kiểm tra CORS settings trên backend

