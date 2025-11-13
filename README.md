# Hệ thống Hộp đen thông minh cho xe máy / ô tô

Hệ thống web quản lý và theo dõi thiết bị hộp đen thông minh với bản đồ trung tâm, hỗ trợ người dùng, trạm y tế và trạm cứu hộ. Tích hợp với ESP32 để nhận dữ liệu GPS real-time.

## Công nghệ sử dụng

### Frontend
- React 18 + TypeScript
- Vite
- Tailwind CSS
- React Router
- Leaflet (bản đồ)
- React-Leaflet (React wrapper cho Leaflet)
- Axios (HTTP client)
- Zustand (state management)
- Socket.IO Client (WebSocket)

### Backend
- Node.js + Express + TypeScript
- Socket.IO (WebSocket)
- File JSON làm database
- OSRM (Open Source Routing Machine) - Routing theo đường thực tế

### Hardware
- ESP32 - Gửi dữ liệu GPS và telemetry
- Arduino IDE - Lập trình ESP32

## Cấu trúc dự án

```
hopDen/
├── frontend/          # React frontend
│   ├── src/
│   │   ├── components/    # React components
│   │   │   ├── BlinkingUserMarker.tsx
│   │   │   ├── SOSModal.tsx
│   │   │   ├── StationSidePanel.tsx
│   │   │   ├── UserSidePanel.tsx
│   │   │   └── ...
│   │   ├── pages/        # Pages (Login, Register, Map)
│   │   ├── store/        # Zustand store
│   │   ├── api/          # API client
│   │   └── ...
│   ├── logo/             # Logo files
│   │   ├── logocoloa.png
│   │   └── CLBSTEM.jpg
│   ├── public/           # Static files
│   │   └── logo/         # Logo files (served)
│   └── package.json
├── backend/           # Express backend
│   ├── src/
│   │   ├── routes/       # API routes
│   │   │   ├── auth.ts
│   │   │   ├── map.ts
│   │   │   ├── sos.ts
│   │   │   ├── stations.ts
│   │   │   ├── telemetry.ts
│   │   │   └── ratings.ts
│   │   ├── services/     # Business logic
│   │   │   └── dataService.ts
│   │   ├── types/        # TypeScript types
│   │   └── index.ts      # Entry point
│   ├── data/             # JSON database files
│   │   ├── users.json
│   │   ├── stations.json
│   │   ├── sos.json
│   │   ├── devices.json
│   │   ├── telemetry.json
│   │   ├── ratings.json
│   │   └── taikhoan.md   # Tài khoản đăng nhập (auto-generated)
│   └── package.json
├── hardware/         # ESP32 code
│   └── esp32_fake_telemetry.ino
└── README.md
```

## Cài đặt và chạy

### Yêu cầu
- Node.js >= 18
- npm hoặc yarn
- Arduino IDE (nếu muốn test ESP32)

### Backend

1. Di chuyển vào thư mục backend:
```bash
cd backend
```

2. Cài đặt dependencies:
```bash
npm install
```

3. Chạy server (development mode với nodemon):
```bash
npm run dev
```

Server sẽ chạy tại `http://localhost:3000`

**Lưu ý**: Khi server khởi động lần đầu, các file JSON sẽ được tạo tự động trong thư mục `backend/data/`:
- `users.json` - Danh sách người dùng
- `stations.json` - Danh sách trạm (y tế + cứu hộ)
- `devices.json` - Thiết bị hộp đen
- `sos.json` - Lịch sử SOS
- `ratings.json` - Đánh giá trạm
- `telemetry.json` - Dữ liệu telemetry từ ESP32
- `taikhoan.md` - File tự động tạo chứa tất cả tài khoản đăng nhập

### Frontend

1. Mở terminal mới, di chuyển vào thư mục frontend:
```bash
cd frontend
```

2. Cài đặt dependencies:
```bash
npm install
```

3. Chạy development server:
```bash
npm run dev
```

Frontend sẽ chạy tại `http://localhost:5173`

## Tài khoản mẫu để test

Xem file `backend/data/taikhoan.md` để xem danh sách đầy đủ tất cả tài khoản.

### Người dùng (User)
- Email: `nguyenvanan@example.com`
- Password: `password123`
- Role: `user`

Hoặc các user khác trong file `taikhoan.md`

### Trạm y tế (Medical Station)
- Email: `cho-ray@example.com`
- Password: `password123`
- Role: `medical_station`

### Trạm cứu hộ (Rescue Station)
- Email: `cuuhocq1@example.com`
- Password: `password123`
- Role: `rescue_station`

**Lưu ý**: Trạm sửa xe đã được gộp vào Trạm cứu hộ.

## Tính năng chính

### Cho Người dùng (User)

#### Quản lý vị trí
- **Vị trí từ ESP32**: Hệ thống tự động nhận và cập nhật vị trí từ thiết bị ESP32
- **Nhập thủ công**: Nếu không có dữ liệu ESP32, có thể nhập tọa độ thủ công
- **Ưu tiên dữ liệu**: Dữ liệu từ ESP32 luôn được ưu tiên, dữ liệu nhập thủ công sẽ tồn tại cho đến khi có dữ liệu ESP32 mới

#### SOS & Cứu hộ
- Gửi SOS khi gặp sự cố (Tai nạn, Hỏng xe, Y tế, Khác)
- Hủy SOS đang active
- Xem thông tin trạm đang kết nối (tự động cập nhật khi trạm thay đổi)
- Trạng thái kết nối: "Đang kết nối" → "Đã nhận" → "Đang đi"
- Tìm trạm y tế / cứu hộ gần nhất
- Xem đường đi đến trạm (routing theo đường thực tế)
- Xem lịch sử SOS
- Đánh giá trạm sau khi được cứu hộ

#### Thông tin cá nhân
- Hiển thị ID người dùng rõ ràng (ví dụ: U0001) để cặp với thiết bị ESP32
- Xem và cập nhật thông tin cá nhân
- Xem vị trí hiện tại

### Cho Trạm y tế / Cứu hộ

#### Quản lý SOS
- Xem SOS trong khu vực (tự động lọc theo loại trạm):
  - **Trạm y tế**: Chỉ nhận SOS "Tai nạn" và "Y tế"
  - **Trạm cứu hộ**: Chỉ nhận SOS "Hỏng xe" và "Khác"
- **Sẵn sàng nhận nhiệm vụ**: Đánh dấu sẵn sàng để được ưu tiên khi có SOS
- Nhận nhiệm vụ (claim SOS)
- Từ chối nhiệm vụ (tự động chuyển sang trạm khác)
- Cập nhật trạng thái: Đã nhận → Đang đi → Hoàn thành
- Hủy nhiệm vụ (tự động chuyển sang trạm khác)

#### Điều phối tự động
- Tự động tìm trạm gần nhất khi có SOS mới
- Ưu tiên trạm đã đánh dấu "Sẵn sàng"
- Tự động chuyển SOS sang trạm khác khi trạm từ chối
- Loại trừ trạm đã từ chối trong cùng chu kỳ SOS

#### Thông tin & Điều hướng
- Hiển thị khoảng cách đến SOS
- Hiển thị đường đi đến vị trí SOS (routing theo đường thực tế)
- Mở Google Maps để điều hướng
- Xem thông tin chi tiết SOS và người dùng
- Xem lịch sử nhiệm vụ đã hoàn thành (30 ngày gần nhất)
- **Bảo vệ quyền riêng tư**: Ẩn vị trí người dùng sau khi hoàn thành nhiệm vụ

## API Endpoints

### Authentication
- `POST /api/auth/login` - Đăng nhập
- `POST /api/auth/register-user` - Đăng ký user
- `POST /api/auth/register-station` - Đăng ký trạm
- `PATCH /api/auth/users/:id/location` - Cập nhật vị trí user (nhập thủ công)

### Map
- `GET /api/map/entities` - Lấy tất cả entities trên map (users, stations, SOS, devices)
- `GET /api/map/routes?fromLat=&fromLon=&toLat=&toLon=` - Lấy route (polyline) sử dụng OSRM

### Stations
- `GET /api/stations/nearest?lat=&lon=&type=medical|rescue` - Tìm trạm gần nhất
- `GET /api/stations?type=medical|rescue` - Lấy danh sách trạm
- `GET /api/stations/:id` - Lấy thông tin chi tiết trạm

### SOS
- `POST /api/sos` - Tạo SOS mới (tự động gán cho trạm gần nhất)
- `GET /api/sos?status=&userId=&stationId=` - Lấy danh sách SOS
- `GET /api/sos/:id` - Lấy chi tiết SOS
- `PATCH /api/sos/:id/claim` - Nhận nhiệm vụ (claim)
- `PATCH /api/sos/:id/status` - Cập nhật trạng thái (pending, accepted, on_route, done, cancelled)
- `PATCH /api/sos/:id/reject` - Từ chối nhiệm vụ (tự động chuyển sang trạm khác)
- `PATCH /api/sos/:id/ready` - Đánh dấu sẵn sàng nhận nhiệm vụ

### Ratings
- `POST /api/ratings` - Tạo đánh giá
- `GET /api/ratings?stationId=` - Lấy đánh giá theo trạm

### Telemetry (ESP32)
- `POST /api/telemetry` - Gửi dữ liệu telemetry từ ESP32
  - Body: `{ deviceId, userId, lat, lon, speed?, source: 'hardware'|'mobile' }`
  - Tự động cập nhật vị trí user trong `users.json`
  - Ưu tiên dữ liệu `hardware` nếu fresh (trong 60 giây)
- `GET /api/telemetry?deviceId=&userId=` - Lấy dữ liệu telemetry

## WebSocket Events

### Client nhận:
- `sos:new` - SOS mới được tạo
- `sos:update` - SOS được cập nhật
- `sos:reassigned` - SOS được chuyển sang trạm khác
- `user:update` - Vị trí user được cập nhật (từ ESP32 hoặc nhập thủ công)

### Client gửi:
- Kết nối tự động khi vào trang Map

## Tích hợp ESP32

### Cấu hình ESP32

1. Mở file `hardware/esp32_fake_telemetry.ino` trong Arduino IDE
2. Cấu hình WiFi:
```cpp
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
```

3. Cấu hình Backend URL:
```cpp
const char* BACKEND_BASE_URL = "http://192.168.1.18:3000"; // IP máy chạy backend
```

4. Cấu hình User ID:
```cpp
const char* USER_ID   = "U0001";  // ID user trong hệ thống
const char* DEVICE_ID = "DHW001"; // ID thiết bị
```

5. Cấu hình thông tin đăng nhập:
```cpp
const char* USER_EMAIL    = "user@example.com";
const char* USER_PASSWORD = "password123";
```

6. Upload code lên ESP32

### Cách hoạt động

1. ESP32 kết nối WiFi và đăng nhập vào backend
2. ESP32 gửi dữ liệu GPS mỗi 30 giây đến `/api/telemetry`
3. Backend tự động cập nhật vị trí user trong `users.json`
4. Backend emit WebSocket event `user:update` để frontend cập nhật real-time
5. Frontend tự động reload map để hiển thị vị trí mới

### Ưu tiên dữ liệu vị trí

- **Hardware (ESP32)**: Luôn được ưu tiên nếu dữ liệu fresh (trong 60 giây)
- **Mobile (nhập thủ công)**: Chỉ được dùng nếu không có dữ liệu hardware hoặc hardware cũ

## Routing (Chỉ đường)

Hệ thống sử dụng **OSRM (Open Source Routing Machine)** để tính toán đường đi theo đường thực tế, không phải đường thẳng.

- Tự động tính route khi trạm nhận nhiệm vụ
- Hiển thị polyline trên bản đồ
- Có thể mở Google Maps để điều hướng chi tiết

## Lưu ý quan trọng

1. **ID người dùng**: Mỗi user có một ID duy nhất (ví dụ: U0001, U0002, ...) được hiển thị rõ ràng trên giao diện. ID này phải khớp với `USER_ID` trong code ESP32.

2. **Database JSON**: Tất cả dữ liệu được lưu trong file JSON trong thư mục `backend/data/`. Khi đăng ký mới, dữ liệu sẽ được ghi thêm vào các file này.

3. **Mật khẩu**: Hiện tại mật khẩu được lưu plain text để demo. Trong production, nên hash bằng bcrypt.

4. **Token**: Hiện tại dùng mock token. Trong production, nên dùng JWT thật.

5. **Nodemon**: Backend sử dụng nodemon để tự động restart khi code thay đổi. File `nodemon.json` đã được cấu hình để ignore thư mục `data/` để tránh restart không cần thiết.

6. **Logo**: Logo cổ loa và CLBSTEM được đặt trong `frontend/public/logo/` và hiển thị trên thanh bar.

## Build cho production

### Backend
```bash
cd backend
npm run build
npm start
```

### Frontend
```bash
cd frontend
npm run build
npm run preview
```

## Phát triển tiếp

- [x] Tích hợp ESP32 để nhận dữ liệu GPS
- [x] Routing theo đường thực tế (OSRM)
- [x] Phân loại SOS theo loại trạm
- [x] Tự động điều phối SOS
- [x] Trạm sẵn sàng nhận nhiệm vụ
- [x] Bảo vệ quyền riêng tư (ẩn vị trí sau khi hoàn thành)
- [x] Lịch sử nhiệm vụ cho trạm
- [ ] Hash mật khẩu với bcrypt
- [ ] JWT authentication thật
- [ ] Push notifications
- [ ] Dashboard analytics
- [ ] Export dữ liệu
- [ ] Mobile app (React Native)

## License

MIT
