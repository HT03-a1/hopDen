================================================================================
    HỆ THỐNG HỘP ĐEN THÔNG MINH CHO XE MÁY / Ô TÔ
    TÀI LIỆU MÔ TẢ CHỨC NĂNG VÀ TÍNH NĂNG
================================================================================

📋 MỤC LỤC
----------
1. Tổng quan hệ thống
2. Chức năng cho Người dùng (User)
3. Chức năng cho Trạm y tế
4. Chức năng cho Trạm cứu hộ
5. Tính năng nổi bật
6. Công nghệ sử dụng
7. Tích hợp phần cứng (ESP32)

================================================================================
1. TỔNG QUAN HỆ THỐNG
================================================================================

Hệ thống Hộp đen thông minh là một nền tảng web quản lý và theo dõi thiết bị 
hộp đen được lắp đặt trên xe máy hoặc ô tô. Hệ thống cung cấp các chức năng:

✓ Theo dõi vị trí GPS real-time từ thiết bị ESP32
✓ Gửi tín hiệu SOS khi gặp sự cố (tai nạn, hỏng xe, y tế)
✓ Tự động điều phối và kết nối với trạm y tế/cứu hộ gần nhất
✓ Hiển thị bản đồ tương tác với routing theo đường thực tế
✓ Quản lý nhiệm vụ cho trạm y tế và cứu hộ
✓ Đánh giá và phản hồi sau khi được cứu hộ

Hệ thống hỗ trợ 3 loại người dùng:
- Người dùng (User): Người sử dụng thiết bị hộp đen trên xe
- Trạm y tế (Medical Station): Trạm y tế nhận và xử lý SOS y tế
- Trạm cứu hộ (Rescue Station): Trạm cứu hộ nhận và xử lý SOS hỏng xe

================================================================================
2. CHỨC NĂNG CHO NGƯỜI DÙNG (USER)
================================================================================

2.1. QUẢN LÝ VỊ TRÍ
-------------------

✓ Hiển thị ID người dùng rõ ràng (ví dụ: U0001)
  - ID này dùng để cặp với thiết bị ESP32
  - Hiển thị ngay trên giao diện để dễ nhớ

✓ Nhận vị trí tự động từ ESP32
  - Thiết bị ESP32 gửi dữ liệu GPS mỗi 30 giây
  - Vị trí tự động cập nhật trên bản đồ real-time
  - Không cần thao tác thủ công

✓ Nhập vị trí thủ công (nếu không có ESP32)
  - Có thể nhập tọa độ lat/lon thủ công
  - Dữ liệu thủ công sẽ được thay thế khi có dữ liệu ESP32

✓ Ưu tiên dữ liệu thông minh
  - Dữ liệu từ ESP32 luôn được ưu tiên
  - Dữ liệu nhập thủ công chỉ dùng khi không có ESP32
  - Tự động chuyển đổi giữa 2 nguồn dữ liệu

2.2. GỬI TÍN HIỆU SOS
---------------------

✓ Gửi SOS khi gặp sự cố
  - Nút "Gửi SOS" nổi bật trên giao diện
  - Chọn loại sự cố:
    • Tai nạn (giao cho Trạm y tế)
    • Hỏng xe (giao cho Trạm cứu hộ)
    • Y tế (giao cho Trạm y tế)
    • Khác (giao cho Trạm cứu hộ)
  - Chọn mức độ: Thấp, Trung bình, Cao, Khẩn cấp
  - Thêm ghi chú mô tả tình huống (tùy chọn)

✓ Tự động điều phối
  - Hệ thống tự động tìm trạm gần nhất phù hợp
  - Tự động gán SOS cho trạm
  - Ưu tiên trạm đã đánh dấu "Sẵn sàng"

✓ Theo dõi trạng thái SOS
  - "Đang kết nối": Đang tìm trạm
  - "Đã nhận": Trạm đã nhận nhiệm vụ
  - "Đang đi": Trạm đang trên đường đến
  - "Hoàn thành": Đã được cứu hộ
  - Tự động cập nhật real-time qua WebSocket

✓ Hủy SOS
  - Có thể hủy SOS đang active nếu không cần thiết
  - Tự động thông báo cho trạm

2.3. TÌM KIẾM VÀ ĐIỀU HƯỚNG
----------------------------

✓ Tìm trạm y tế gần nhất
  - Hiển thị danh sách trạm y tế gần vị trí
  - Sắp xếp theo khoảng cách
  - Hiển thị khoảng cách và thời gian di chuyển

✓ Tìm trạm cứu hộ gần nhất
  - Hiển thị danh sách trạm cứu hộ gần vị trí
  - Sắp xếp theo khoảng cách
  - Hiển thị khoảng cách và thời gian di chuyển

✓ Xem đường đi đến trạm
  - Routing theo đường thực tế (không phải đường thẳng)
  - Sử dụng OSRM (Open Source Routing Machine)
  - Hiển thị polyline trên bản đồ
  - Hiển thị khoảng cách và thời gian ước tính

✓ Mở Google Maps
  - Nút mở Google Maps để điều hướng chi tiết
  - Tự động điền điểm đến

2.4. XEM THÔNG TIN TRẠM
-----------------------

✓ Thông tin trạm đang kết nối
  - Tên trạm
  - Địa chỉ
  - Số điện thoại
  - Email
  - Khoảng cách
  - Trạng thái kết nối
  - Tự động cập nhật khi trạm thay đổi

✓ Xem chi tiết trạm khác
  - Click vào marker trên bản đồ
  - Xem thông tin đầy đủ
  - Xem đánh giá từ người dùng khác

2.5. LỊCH SỬ VÀ ĐÁNH GIÁ
------------------------

✓ Xem lịch sử SOS
  - Danh sách tất cả SOS đã gửi
  - Thông tin chi tiết: thời gian, loại, trạng thái
  - Trạm đã xử lý

✓ Đánh giá trạm
  - Đánh giá sau khi được cứu hộ
  - Chọn sao (1-5 sao)
  - Viết nhận xét
  - Giúp cải thiện chất lượng dịch vụ

================================================================================
3. CHỨC NĂNG CHO TRẠM Y TẾ
================================================================================

3.1. QUẢN LÝ SOS
----------------

✓ Xem SOS trong khu vực
  - Chỉ hiển thị SOS phù hợp: "Tai nạn" và "Y tế"
  - Tự động lọc theo loại trạm
  - Sắp xếp theo khoảng cách gần nhất

✓ Đánh dấu "Sẵn sàng nhận nhiệm vụ"
  - Toggle để bật/tắt trạng thái sẵn sàng
  - Trạm sẵn sàng được ưu tiên khi có SOS mới
  - Giúp điều phối hiệu quả hơn

✓ Nhận nhiệm vụ (Claim SOS)
  - Click "Nhận nhiệm vụ" để claim SOS
  - Tự động cập nhật trạng thái: "Đã nhận"
  - Người dùng được thông báo ngay lập tức

✓ Từ chối nhiệm vụ
  - Có thể từ chối nếu không thể xử lý
  - Tự động chuyển SOS sang trạm khác gần nhất
  - Trạm đã từ chối sẽ không nhận lại SOS đó

✓ Cập nhật trạng thái
  - "Đã nhận" → "Đang đi" → "Hoàn thành"
  - Mỗi bước tự động thông báo cho người dùng
  - Real-time update qua WebSocket

✓ Hủy nhiệm vụ
  - Có thể hủy nếu cần thiết
  - Tự động chuyển sang trạm khác

3.2. THÔNG TIN VÀ ĐIỀU HƯỚNG
-----------------------------

✓ Thông tin chi tiết SOS
  - Loại sự cố
  - Mức độ khẩn cấp
  - Ghi chú từ người dùng
  - Vị trí chính xác (lat/lon)
  - Thời gian tạo SOS

✓ Thông tin người dùng
  - Tên, email, số điện thoại
  - Vị trí hiện tại (real-time)
  - Lịch sử SOS trước đó

✓ Khoảng cách và thời gian
  - Hiển thị khoảng cách đến vị trí SOS
  - Thời gian di chuyển ước tính
  - Tự động cập nhật khi vị trí thay đổi

✓ Routing theo đường thực tế
  - Polyline hiển thị đường đi trên bản đồ
  - Tính toán theo đường thực tế (OSRM)
  - Không phải đường thẳng

✓ Mở Google Maps
  - Nút mở Google Maps để điều hướng
  - Tự động điền điểm đến
  - Hỗ trợ điều hướng chi tiết

3.3. LỊCH SỬ NHIỆM VỤ
---------------------

✓ Xem lịch sử đã hoàn thành
  - Danh sách nhiệm vụ đã hoàn thành (30 ngày gần nhất)
  - Thông tin chi tiết: thời gian, người dùng, loại SOS
  - Trạng thái: Hoàn thành / Đã hủy

✓ Bảo vệ quyền riêng tư
  - Vị trí người dùng bị ẩn sau khi hoàn thành
  - Chỉ hiển thị vị trí khi đang xử lý SOS
  - Tuân thủ quyền riêng tư người dùng

3.4. ĐÁNH GIÁ VÀ PHẢN HỒI
--------------------------

✓ Xem đánh giá từ người dùng
  - Xem tất cả đánh giá đã nhận
  - Số sao trung bình
  - Nhận xét chi tiết
  - Giúp cải thiện chất lượng dịch vụ

================================================================================
4. CHỨC NĂNG CHO TRẠM CỨU HỘ
================================================================================

Tương tự như Trạm y tế, nhưng:
- Chỉ nhận SOS: "Hỏng xe" và "Khác"
- Tự động lọc theo loại trạm
- Các chức năng khác giống Trạm y tế

Lưu ý: Trạm sửa xe đã được gộp vào Trạm cứu hộ.

================================================================================
5. TÍNH NĂNG NỔI BẬT
================================================================================

5.1. ĐIỀU PHỐI TỰ ĐỘNG THÔNG MINH
----------------------------------

✓ Tự động tìm trạm gần nhất
  - Khi có SOS mới, hệ thống tự động tìm trạm gần nhất
  - Lọc theo loại trạm phù hợp
  - Tính toán khoảng cách chính xác

✓ Ưu tiên trạm sẵn sàng
  - Trạm đã đánh dấu "Sẵn sàng" được ưu tiên
  - Giúp điều phối nhanh chóng và hiệu quả

✓ Tự động chuyển đổi trạm
  - Khi trạm từ chối, tự động chuyển sang trạm khác
  - Loại trừ trạm đã từ chối trong cùng chu kỳ SOS
  - Đảm bảo SOS luôn được xử lý

✓ Phân loại SOS thông minh
  - Trạm y tế chỉ nhận: Tai nạn, Y tế
  - Trạm cứu hộ chỉ nhận: Hỏng xe, Khác
  - Tự động gán đúng trạm phù hợp

5.2. REAL-TIME UPDATES
----------------------

✓ WebSocket real-time
  - Tất cả thay đổi được cập nhật ngay lập tức
  - Không cần refresh trang
  - Đồng bộ giữa nhiều người dùng

✓ Cập nhật vị trí real-time
  - Vị trí từ ESP32 cập nhật mỗi 30 giây
  - Tự động hiển thị trên bản đồ
  - Marker nhấp nháy để dễ nhận biết

✓ Thông báo trạng thái
  - Trạng thái SOS thay đổi ngay lập tức
  - Người dùng và trạm đều nhận thông báo
  - Không bỏ lỡ thông tin quan trọng

5.3. ROUTING THEO ĐƯỜNG THỰC TẾ
---------------------------------

✓ Sử dụng OSRM (Open Source Routing Machine)
  - Tính toán đường đi theo đường thực tế
  - Không phải đường thẳng
  - Chính xác và thực tế

✓ Hiển thị polyline trên bản đồ
  - Đường đi được vẽ rõ ràng trên bản đồ
  - Dễ theo dõi và điều hướng

✓ Khoảng cách và thời gian ước tính
  - Hiển thị khoảng cách thực tế
  - Thời gian di chuyển ước tính
  - Giúp lập kế hoạch tốt hơn

5.4. BẢO VỆ QUYỀN RIÊNG TƯ
----------------------------

✓ Ẩn vị trí sau khi hoàn thành
  - Vị trí người dùng chỉ hiển thị khi đang xử lý SOS
  - Tự động ẩn sau khi hoàn thành nhiệm vụ
  - Bảo vệ quyền riêng tư người dùng

✓ Kiểm soát thông tin
  - Chỉ trạm đang xử lý mới thấy vị trí chi tiết
  - Thông tin cá nhân được bảo vệ
  - Tuân thủ quy định bảo mật

5.5. GIAO DIỆN THÂN THIỆN
-------------------------

✓ Responsive design
  - Hoạt động tốt trên desktop, tablet, mobile
  - Tự động điều chỉnh layout
  - Trải nghiệm người dùng tốt

✓ Bản đồ tương tác
  - Leaflet map với các marker rõ ràng
  - Zoom, pan dễ dàng
  - Click để xem thông tin chi tiết

✓ Side panel thông tin
  - Hiển thị thông tin chi tiết
  - Dễ dàng đóng/mở
  - Tối ưu cho mobile

✓ Màu sắc phân biệt
  - User: Xanh lá
  - Station: Xanh dương
  - SOS: Đỏ
  - Dễ nhận biết và phân biệt

5.6. TÍCH HỢP PHẦN CỨNG
------------------------

✓ Kết nối ESP32
  - Nhận dữ liệu GPS tự động
  - Không cần thao tác thủ công
  - Real-time tracking

✓ Ưu tiên dữ liệu thông minh
  - Dữ liệu ESP32 luôn được ưu tiên
  - Tự động chuyển đổi giữa ESP32 và nhập thủ công
  - Đảm bảo độ chính xác

✓ Đăng nhập tự động
  - ESP32 tự động đăng nhập vào hệ thống
  - Gửi dữ liệu định kỳ
  - Không cần can thiệp

================================================================================
6. CÔNG NGHỆ SỬ DỤNG
================================================================================

6.1. FRONTEND
-------------

✓ React 18 + TypeScript
  - Framework hiện đại và mạnh mẽ
  - Type-safe code
  - Dễ bảo trì và mở rộng

✓ Vite
  - Build tool nhanh chóng
  - Hot module replacement
  - Development experience tốt

✓ Tailwind CSS
  - Utility-first CSS framework
  - Responsive design dễ dàng
  - Customizable và linh hoạt

✓ Leaflet + React-Leaflet
  - Bản đồ tương tác
  - Open source và miễn phí
  - Nhiều plugin hỗ trợ

✓ Zustand
  - State management đơn giản
  - Lightweight và hiệu quả
  - Dễ sử dụng

✓ Socket.IO Client
  - WebSocket real-time communication
  - Tự động reconnect
  - Event-based architecture

✓ Axios
  - HTTP client mạnh mẽ
  - Interceptors và error handling
  - Promise-based API

6.2. BACKEND
------------

✓ Node.js + Express + TypeScript
  - Runtime JavaScript phổ biến
  - Framework web nhanh và linh hoạt
  - Type-safe backend code

✓ Socket.IO
  - WebSocket server
  - Real-time bidirectional communication
  - Room và namespace support

✓ File JSON Database
  - Đơn giản, không cần setup database
  - Dễ backup và restore
  - Phù hợp cho prototype và demo

✓ OSRM Integration
  - Routing theo đường thực tế
  - Open source và miễn phí
  - Chính xác và nhanh chóng

6.3. HARDWARE
-------------

✓ ESP32
  - Microcontroller mạnh mẽ
  - WiFi và Bluetooth tích hợp
  - Dễ lập trình với Arduino IDE

✓ GPS Module
  - Nhận tín hiệu GPS
  - Độ chính xác cao
  - Real-time tracking

✓ Các module khác (tùy chọn)
  - DS1307 RTC (đồng hồ thời gian thực)
  - MPU9250 (cảm biến gia tốc)
  - DHT11 (cảm biến nhiệt độ)
  - SIM 4G (kết nối mạng)
  - SD Card (lưu trữ dữ liệu)
  - OLED (hiển thị thông tin)

================================================================================
7. TÍCH HỢP PHẦN CỨNG (ESP32)
================================================================================

7.1. CẤU HÌNH ESP32
--------------------

✓ Kết nối WiFi
  - Cấu hình SSID và password
  - Tự động kết nối khi khởi động
  - Tự động reconnect nếu mất kết nối

✓ Kết nối Backend
  - Cấu hình URL backend
  - Đăng nhập tự động với email/password
  - Lưu token để xác thực

✓ Cấu hình User ID
  - ID người dùng phải khớp với hệ thống
  - Hiển thị rõ trên giao diện web
  - Dùng để cặp thiết bị với user

✓ Gửi dữ liệu định kỳ
  - Gửi GPS mỗi 30 giây
  - Bao gồm: lat, lon, speed (nếu có)
  - Tự động cập nhật vị trí user

7.2. CÁCH HOẠT ĐỘNG
--------------------

1. ESP32 khởi động và kết nối WiFi
2. ESP32 đăng nhập vào backend với email/password
3. ESP32 nhận token xác thực
4. ESP32 gửi dữ liệu GPS mỗi 30 giây đến /api/telemetry
5. Backend tự động cập nhật vị trí user trong users.json
6. Backend emit WebSocket event 'user:update'
7. Frontend tự động cập nhật bản đồ real-time

7.3. ƯU TIÊN DỮ LIỆU
---------------------

✓ Dữ liệu Hardware (ESP32)
  - Luôn được ưu tiên nếu fresh (trong 60 giây)
  - Tự động thay thế dữ liệu nhập thủ công
  - Đảm bảo độ chính xác

✓ Dữ liệu Mobile (nhập thủ công)
  - Chỉ dùng khi không có dữ liệu ESP32
  - Hoặc khi dữ liệu ESP32 quá cũ (>60 giây)
  - Tự động bị thay thế khi có ESP32 mới

================================================================================
8. KẾT LUẬN
================================================================================

Hệ thống Hộp đen thông minh là một giải pháp toàn diện cho việc theo dõi và 
cứu hộ xe máy/ô tô. Với các tính năng:

✓ Real-time GPS tracking
✓ Tự động điều phối SOS
✓ Routing theo đường thực tế
✓ Bảo vệ quyền riêng tư
✓ Giao diện thân thiện
✓ Tích hợp phần cứng dễ dàng

Hệ thống giúp:
- Người dùng được cứu hộ nhanh chóng khi gặp sự cố
- Trạm y tế/cứu hộ quản lý nhiệm vụ hiệu quả
- Điều phối tự động, giảm thời gian phản ứng
- Theo dõi và quản lý dễ dàng

================================================================================
                    CẢM ƠN BẠN ĐÃ QUAN TÂM ĐẾN HỆ THỐNG!
================================================================================

