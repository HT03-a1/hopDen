# Hệ thống Hộp đen thông minh cho xe máy / ô tô

Hệ thống web quản lý và theo dõi thiết bị hộp đen thông minh với bản đồ trung tâm, hỗ trợ người dùng, trạm y tế và trạm cứu hộ.

## Công nghệ sử dụng

### Frontend
- React 18 + TypeScript
- Vite
- Tailwind CSS
- React Router
- Leaflet (bản đồ)
- Axios (HTTP client)
- Zustand (state management)
- Socket.IO Client (WebSocket)

### Backend
- Node.js + Express + TypeScript
- Socket.IO (WebSocket)
- File JSON làm database

## Cấu trúc dự án

```
hopDen/
├── frontend/          # React frontend
│   ├── src/
│   │   ├── components/    # React components
│   │   ├── pages/        # Pages (Login, Register, Map)
│   │   ├── store/        # Zustand store
│   │   ├── api/          # API client
│   │   └── ...
│   └── package.json
├── backend/           # Express backend
│   ├── src/
│   │   ├── routes/       # API routes
│   │   ├── services/     # Business logic
│   │   ├── types/        # TypeScript types
│   │   └── index.ts      # Entry point
│   ├── data/             # JSON database files
│   └── package.json
└── README.md
```

## Cài đặt và chạy

### Yêu cầu
- Node.js >= 18
- npm hoặc yarn

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

Khi server khởi động lần đầu, các file JSON sẽ được tạo tự động trong thư mục `backend/data/`:
- `users.json` - 5 user mẫu
- `stations.json` - 5 trạm y tế + 5 trạm cứu hộ
- `devices.json` - Thiết bị
- `sos.json` - Lịch sử SOS
- `ratings.json` - Đánh giá
- `telemetry.json` - Dữ liệu telemetry

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

### Người dùng (User)
- Email: `nguyenvanan@example.com`
- Password: `password123`
- Role: `user`

Hoặc các user khác: `tranthibinh@example.com`, `levancuong@example.com`, v.v.

### Trạm y tế (Medical Station)
- Email: `cho-ray@example.com`
- Password: `password123`
- Role: `medical_station`

### Trạm cứu hộ (Rescue Station)
- Email: `cuuhocq1@example.com`
- Password: `password123`
- Role: `rescue_station`

## Tính năng chính

### Cho Người dùng (User)
- Xem bản đồ với vị trí của mình và các trạm
- Gửi SOS khi gặp sự cố
- Tìm trạm y tế / cứu hộ gần nhất
- Xem đường đi đến trạm
- Xem lịch sử SOS
- Đánh giá trạm sau khi được cứu hộ
- **Hiển thị ID người dùng rõ ràng** để cặp với thiết bị hộp đen phần cứng

### Cho Trạm y tế / Cứu hộ
- Xem bản đồ với các SOS trong khu vực
- Nhận nhiệm vụ (claim SOS)
- Cập nhật trạng thái nhiệm vụ (Đã nhận → Đang đi → Hoàn thành)
- Hiển thị đường đi đến vị trí SOS
- Xem thông tin chi tiết SOS

## API Endpoints

### Authentication
- `POST /api/auth/login` - Đăng nhập
- `POST /api/auth/register-user` - Đăng ký user
- `POST /api/auth/register-station` - Đăng ký trạm

### Map
- `GET /api/map/entities` - Lấy tất cả entities trên map
- `GET /api/map/routes?fromLat=&fromLon=&toLat=&toLon=` - Lấy route (polyline)

### Stations
- `GET /api/stations/nearest?lat=&lon=&type=medical|rescue` - Tìm trạm gần nhất
- `GET /api/stations?type=medical|rescue|repair` - Lấy danh sách trạm

### SOS
- `POST /api/sos` - Tạo SOS mới
- `GET /api/sos?status=&userId=&stationId=` - Lấy danh sách SOS
- `GET /api/sos/:id` - Lấy chi tiết SOS
- `PATCH /api/sos/:id/claim` - Nhận nhiệm vụ (claim)
- `PATCH /api/sos/:id/status` - Cập nhật trạng thái

### Ratings
- `POST /api/ratings` - Tạo đánh giá
- `GET /api/ratings?stationId=` - Lấy đánh giá theo trạm

### Telemetry
- `POST /api/telemetry` - Gửi dữ liệu telemetry
- `GET /api/telemetry?deviceId=&userId=` - Lấy dữ liệu telemetry

## WebSocket Events

### Client nhận:
- `sos:new` - SOS mới được tạo
- `sos:update` - SOS được cập nhật

## Lưu ý quan trọng

1. **ID người dùng**: Mỗi user có một ID duy nhất (ví dụ: U0001, U0002, ...) được hiển thị rõ ràng trên giao diện. ID này sẽ được dùng để cặp với thiết bị hộp đen phần cứng bên ngoài.

2. **Database JSON**: Tất cả dữ liệu được lưu trong file JSON trong thư mục `backend/data/`. Khi đăng ký mới, dữ liệu sẽ được ghi thêm vào các file này.

3. **Mật khẩu**: Hiện tại mật khẩu được lưu plain text để demo. Trong production, nên hash bằng bcrypt.

4. **Token**: Hiện tại dùng mock token. Trong production, nên dùng JWT thật.

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

- [ ] Hash mật khẩu với bcrypt
- [ ] JWT authentication thật
- [ ] Tích hợp API bản đồ thật (Google Maps, Mapbox)
- [ ] Tích hợp API routing thật (OSRM, Google Directions)
- [ ] Push notifications
- [ ] Real-time location tracking
- [ ] Dashboard analytics
- [ ] Export dữ liệu

## License

MIT

