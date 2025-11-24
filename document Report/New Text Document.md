




 
 
MỤC LỤC

1. LÍ DO CHỌN ĐỀ TÀI	1
2. MỤC TIÊU NGHIÊN CỨU	1
3. KẾ HOẠCH NGHIÊN CỨU	1
4. PHƯƠNG PHÁP NGHIÊN CỨU	1
4.1.  Phương pháp nghiên cứu lí thuyết	1
4.2. Phương pháp thực nghiệm	2
4.3. Phương pháp thu thập thông tin và tổng kết kinh nghiệm	2
5. NỘI DUNG NGHIÊN CỨU	2
6. TIẾN HÀNH NGHIÊN CỨU	2
6.1. Giải pháp	2
6.2. Phân tích thiết kế	2
6.3. Cơ chế hoạt động hệ thống	5
7. THỰC NGHIỆM VÀ ĐÁNH GIÁ	5
7.1. Quy trình thực nghiệm	5
7.2. Kết quả thực nghiệm	5
7.3. Đánh giá sản phẩm	6
7.4. Điểm mới của sản phẩm	6
8. HƯỚNG PHÁT TRIỂN SẢN PHẨM	6
TÀI LIỆU THAM KHẢO	7
PHỤ LỤC	8


 
1. LÍ DO CHỌN ĐỀ TÀI 

Ở Việt Nam, mỗi năm xảy ra hàng chục nghìn vụ tai nạn giao thông, trong đó có hàng nghìn người tử vong. Đáng lo ngại hơn, nhiều vụ tai nạn thương tâm xảy ra ở những con đường vắng, vào đêm khuya, hoặc tại các khu vực hẻo lánh, nơi người gặp nạn không thể tự kêu cứu và người đi đường ít có khả năng phát hiện.

**Vấn đề cốt lõi:** Trong những tình huống khẩn cấp, **thời gian** là yếu tố quyết định đến sự sống còn. Mỗi phút trì hoãn có thể khiến tỷ lệ tử vong tăng đáng kể. Nhiều trường hợp nạn nhân có thể được cứu sống nếu được phát hiện và đưa đến bệnh viện kịp thời, nhưng do không có phương tiện cảnh báo khẩn cấp hiệu quả, họ đã mất đi cơ hội sống quý giá.

**Những khó khăn trong thực tế:**
- Trong nhiều vụ tai nạn nghiêm trọng, nạn nhân bị bất tỉnh, mất khả năng vận động, không thể sử dụng điện thoại để gọi cứu hộ.
- Việc xác định vị trí tai nạn chính xác trên đường cao tốc, đường liên tỉnh, hoặc khu vực nông thôn rất khó khăn, dẫn đến việc đội cứu hộ mất thời gian tìm kiếm.
- Các phương tiện hiện tại chưa có hệ thống phát hiện và cảnh báo tai nạn tự động, phụ thuộc hoàn toàn vào khả năng của người lái xe.
- Người lái xe còn gặp nhiều tình huống bất tiện như xe hỏng giữa đường, hết xăng, hoặc sự cố kỹ thuật, mà không biết gọi ai giúp đỡ.

**Giải pháp hiện có và hạn chế:** Các hệ thống eCall trên xe ô tô cao cấp có chi phí cao, không phù hợp với đại đa số người dùng xe máy ở Việt Nam. Các ứng dụng điện thoại yêu cầu người dùng phải chủ động kích hoạt, không thể phát hiện tai nạn tự động, và không tích hợp với hệ thống cứu hộ chuyên nghiệp.

**Ý nghĩa và mục tiêu của đề tài:** Xuất phát từ những thực trạng và hạn chế trên, nhóm nghiên cứu quyết định thiết kế một **hệ thống giám sát tai nạn giao thông và web cứu hộ thông minh** với các mục tiêu:
●	**Phát hiện tai nạn hoàn toàn tự động:** Sử dụng cảm biến gia tốc và gia tốc góc kết hợp với cảm biến tốc độ để phát hiện va chạm chính xác, loại bỏ cảnh báo nhầm.
●	**Cảnh báo khẩn cấp đa kênh:** Gửi cảnh báo SOS cùng vị trí GPS chính xác đến trạm cứu hộ và cấp cứu y tế gần nhất qua Internet, SMS, và gọi điện tự động.
●	**Cho phép người dùng chủ động:** Cung cấp nút bấm khẩn cấp để người dùng có thể gửi tín hiệu cầu cứu khi gặp sự cố.
●	**Tích hợp với hệ thống cứu hộ chuyên nghiệp:** Nền tảng web trung tâm tự động điều phối và kết nối giữa người gặp nạn với các trạm y tế, trạm cứu hộ gần nhất.
●	**Chi phí hợp lý và dễ tiếp cận:** Sản phẩm có chi phí thấp, có thể lắp đặt trên cả xe đạp, xe máy, và ô tô.

Với ý nghĩa nhân văn sâu sắc và tiềm năng ứng dụng thực tế cao, đề tài này không chỉ góp phần giảm thiểu thiệt hại về người và tài sản do tai nạn giao thông, mà còn mang lại sự an tâm cho người lái xe và gia đình họ, đồng thời cải thiện đáng kể hiệu quả của các dịch vụ cứu hộ và cấp cứu y tế tại Việt Nam.
2. MỤC TIÊU NGHIÊN CỨU

Đề tài hướng tới việc tạo ra một **hệ thống cứu hộ tự động toàn diện** với các mục tiêu cụ thể:

●	**Thiết bị phần cứng thông minh:** Chế tạo thiết bị hộp đen có khả năng phát hiện tai nạn tự động bằng cảm biến gia tốc, gia tốc góc và GPS, có thể lắp đặt trên xe đạp, xe máy, ô tô.

●	**Hệ thống cảnh báo đa kênh:** Tự động gửi cảnh báo SOS cùng vị trí GPS chính xác đến trạm cứu hộ và cấp cứu y tế gần nhất qua Internet, SMS và gọi điện.

●	**Nền tảng web điều phối:** Xây dựng máy chủ web trung tâm xử lý dữ liệu realtime, tự động điều phối và kết nối giữa người gặp nạn với các trạm y tế, trạm cứu hộ phù hợp.

●	**Chi phí hợp lý:** Thiết kế sản phẩm có chi phí thấp, phù hợp với đại đa số người dùng Việt Nam, dễ lắp đặt và sử dụng.
3. KẾ HOẠCH NGHIÊN CỨU 
STT	NỘI DUNG	THỜI GIAN THỰC HIỆN
1	Lên ý tưởng	Ngày 6/9/2025 đến 10/9/2025
2	Tìm đọc tài liệu, nghiên cứu về các vật liệu 	Ngày 10/9/2025 đến 30/9/2025
3	Viết đề cương nghiên cứu	Ngày 1/10/2025 đến 5/10/2025
4	Vẽ sơ đồ tổng quan hệ thống, tính toán thiết kế hệ thống	Ngày 6/10/2025 đến 10/10/2025
5	Mua vật liệu, chế tạo các chi tiết	Ngày 11/10/2025 đến 20/10/2025
6	Lắp đặt	Ngày 21/10/2025 đến 22/10/2025
7	Vận hành thử nghiệm, kiểm tra lỗi, xử lí dữ liệu	Ngày 23/10/2025 đến 2/11/2025
8	Hoàn thiện sản phẩm	Ngày 2/11/2025 đến 6/11/2025
4. PHƯƠNG PHÁP NGHIÊN CỨU

**4.1. Phương pháp nghiên cứu lí thuyết:**
- Nghiên cứu cấu tạo, nguyên lí hoạt động của vi điều khiển ESP32-S3, module GPS NEO-8M, cảm biến gia tốc góc MPU9250, module SIM 4G (SIM7682), và các linh kiện liên quan.
- Nghiên cứu các công nghệ web để xây dựng nền tảng cứu hộ (backend và frontend), giao thức truyền dữ liệu (HTTP, SMS, gọi điện).
- Nghiên cứu kiến thức về Arduino và lập trình C++ cho phần cứng, các công nghệ web cơ bản cho phần mềm.

**4.2. Phương pháp thực nghiệm:**
- Thiết kế và chế tạo thiết bị hộp đen, xây dựng nền tảng web trung tâm.
- Thử nghiệm thiết bị trong nhiều tình huống khác nhau: xe đứng yên, xe di chuyển, va chạm giả lập, và các trường hợp sự cố khác.
- Kiểm chứng độ chính xác phát hiện va chạm, hiệu quả cảnh báo, và độ tin cậy của hệ thống.

**4.3. Phương pháp thu thập thông tin và tổng kết kinh nghiệm:**
- Thu thập các ý kiến nhận xét, phản hồi từ người dùng thử nghiệm.
- Phân tích kết quả thực nghiệm, đánh giá ưu điểm và hạn chế của sản phẩm.
- Chỉnh sửa, hoàn thiện sản phẩm dựa trên kết quả đánh giá và đưa ra kết luận.
5. NỘI DUNG NGHIÊN CỨU

Nội dung nghiên cứu tập trung vào việc thiết kế và phát triển một **hệ thống giám sát tai nạn giao thông và web cứu hộ thông minh** bao gồm:

**5.1. Thiết bị hộp đen thông minh:**
- Thiết bị tự động đo gia tốc, gia tốc góc và tốc độ di chuyển của phương tiện bằng cảm biến MPU9250 và GPS.
- Phát hiện va chạm/tai nạn tự động khi các thông số vượt ngưỡng cài đặt (gia tốc > 2.5g, tốc độ ≥ 10 km/h).
- Ghi âm vòng tròn liên tục để lưu trữ âm thanh trước, trong và sau tai nạn, hỗ trợ phân tích và điều tra sau này.
- Khi phát hiện va chạm, thiết bị đếm ngược 30 giây với cảnh báo buzzer, cho phép người dùng hủy nếu không cần thiết.
- Nếu không bị hủy, thiết bị tự động gửi cảnh báo SOS cùng vị trí GPS chính xác đến server, đồng thời gửi SMS và gọi điện đến số cứu hộ.

**5.2. Nền tảng web trung tâm:**
- Web server nhận và xử lý dữ liệu từ thiết bị hộp đen theo thời gian thực.
- Tự động định vị vị trí gặp nạn và tìm kiếm trạm cứu hộ, trạm cấp cứu y tế gần nhất.
- Gửi cảnh báo SOS đến trạm phù hợp, điều phối quá trình cứu hộ.
- Cung cấp giao diện web để người dùng, trạm y tế và trạm cứu hộ theo dõi và quản lý.

**5.3. Lưu trữ và phân tích dữ liệu:**
- Thiết bị ghi nhật ký liên tục các thông số cảm biến vào thẻ nhớ SD để phục vụ phân tích sau này.
- Ghi âm vòng tròn liên tục, tự động ghi đè file cũ khi đầy để luôn lưu trữ âm thanh gần nhất.
- Lưu trữ lịch sử SOS, dữ liệu telemetry và thông tin cứu hộ trên server.
6. TIẾN HÀNH NGHIÊN CỨU 
6.1. Giải pháp
- Thiết kế mô hình với quy trình hoạt động tự động là vòng tuần hoàn kín: 
 
- Tạo trang web, quản trị web.
6.2. Phân tích thiết kế 
6.2.1. Vỏ: Nhóm thiết kế một vỏ có thể lắp được vừa các linh kiện và nhỏ gọn nhất.
Hình hộp chữ nhật: Dài -14,0 cm; Rộng - 2,0 cm; Cao -7,0 cm. 
Chất liệu vỏ: Nhựa PLA.        
          Gia công: In 3D.      
 	  
 
6.2.2. Sơ đồ khối hệ thống (Tự vẽ và xin góp ý của chuyên gia)
 
6.2.3. Sơ đồ thuật toán (Tự vẽ và xin góp ý của chuyên gia)
 
Hình 6.2.3.1 - Sơ đồ thuật toán hộp đen
 
Hình 6.2.3.2 - Sơ đồ thuật toán ESP Gửi giữ liệu
 
Hình 6.2.3.3 - Sơ đồ thuật toán tìm trạm gần nhất
 


Hình 6.2.3.4 - Sơ đồ thuật toán điều phối SOS tự động
 
Hình 6.2.3.4 - Sơ đồ thuật toán xử lý SOS của trạm
6.2.4. Linh kiện điện – điện tử (Nhờ chuyên gia tư vấn và tự mua)
+ Vi điều khiển ESP32-S3. 
+ Cảm biến MPU9250 phát hiện va chạm. 
+ GPS NEO-8M xác định vị vị trí. 
+ SIM7682 truyền dữ liệu qua mạng 4G. 
+ Màn hình OLED-0.96 in hiển thị trạng thái. 
+ Thẻ nhớ SD-64 GB ghi hành trình, trạng thái và bản ghi âm (1 phút/lần ghi) – lưu được 1 tháng.
+ Đèn, còi buzzer cảnh báo, phím bấm.
+ Cảm biến nhiệt độ, độ ẩm – DHT 11 (cảnh bào nhiệt trong xe quá nóng).
+ Pin Lipo-10000mah (xe đạp-1 tuần; xe máy + ô tô kết hợp nguồn điện ngoài).
6.2.5. Thiết kế mạch điều khiển trung tâm: Làm thủ công trên phần mềm onshape.
**6.2.6. Lập trình:**
- **Lập trình phần cứng:** Lập trình C++ cho ESP32-S3 bằng phần mềm Arduino IDE, sử dụng FreeRTOS để xử lý đa nhiệm, kết nối và điều khiển các cảm biến (MPU9250, GPS, DHT11, DS1307, SIM 4G, OLED, SD Card).
- **Lập trình phần mềm:** Xây dựng nền tảng web trung tâm (backend và frontend) để nhận, xử lý dữ liệu và điều phối cứu hộ.

**6.2.7. Cài đặt các thông số:**

a. **Thông số chuyển động (từ cảm biến MPU9250):**
- Gia tốc 3 trục: Ax, Ay, Az; Gia tốc tổng: √(Ax² + Ay² + Az²)
- Gia tốc góc (con quay hồi chuyển): Gx, Gy, Gz
- Ngưỡng phát hiện va chạm: 
  • Gia tốc tuyến tính: 2.5g (mặc định, có thể điều chỉnh 1.5g - 5.0g)
  • Gia tốc góc: 300 deg/s² (mặc định)
  • Tốc độ tối thiểu: 10 km/h (mặc định)
  • Va chạm được phát hiện khi: Tốc độ ≥ 10 km/h VÀ (Gia tốc > 2.5g HOẶC Gia tốc góc > 300 deg/s²)
- Tần suất lấy mẫu: 100–200 ms

b. **Thông số vị trí (GPS):**
- Vĩ độ (Latitude), Kinh độ (Longitude), Độ cao (Altitude)
- Tốc độ di chuyển (Speed km/h)
- Hướng di chuyển (Course/direction)

c. **Thời gian thực (RTC – DS1307):**
- Giờ – phút – giây; Ngày – tháng – năm
- Pin backup để duy trì thời gian khi mất điện

d. **Thông số cảnh báo – trạng thái:**
- Trạng thái va chạm: Bình thường / Đang đếm ngược / Đã gửi SOS
- Mức độ nghiêm trọng: Tự động phân loại
- Trạng thái gửi cảnh báo: Đã gửi / Chưa gửi

e. **Cài đặt lưu trữ trên thẻ SD:**
- Chu kỳ ghi log: 30 giây
- Tên file log: LOG_YYYYMMDD.csv (ví dụ: LOG_20250115.csv)
- Dung lượng: Tùy SD Card (8GB-32GB, khuyến nghị 8GB cho 1 tháng lưu trữ)

f. **Cài đặt ghi âm:**
- Microphone: INMP441 (I2S digital microphone)
- Tần số mẫu: 16 kHz, 16-bit, mono
- Định dạng file: WAV
- Thời gian mỗi file: 2 phút
- Số lượng file: 50 file (vòng tròn, tự động ghi đè file cũ)
- Tổng thời gian lưu trữ: 100 phút (1 giờ 40 phút)
- Tên file: AUDIO_YYYYMMDD_HHMMSS.wav (theo thời gian kết thúc đoạn ghi)

**6.3. Cơ chế hoạt động hệ thống:**

**Khởi động và vận hành thường xuyên:**
- Khi được cấp nguồn, thiết bị khởi động và kiểm tra tất cả các cảm biến (MPU9250, GPS, DHT11, DS1307, SIM 4G, OLED, SD Card).
- Thiết bị tự động đo và ghi nhật ký các giá trị cảm biến (gia tốc, tốc độ, vị trí GPS, nhiệt độ, độ ẩm) vào thẻ SD mỗi 30 giây.
- Thiết bị gửi dữ liệu telemetry (vị trí GPS, tốc độ, trạng thái) lên web trung tâm mỗi 30 giây để cập nhật vị trí realtime.
- Ghi âm vòng tròn liên tục để lưu trữ âm thanh trước, trong và sau tai nạn.

**Phần bị động (Tự động phát hiện tai nạn):**

*Bước 1 - Đo và phân tích liên tục:*
- Task Sensors đọc cảm biến MPU9250 mỗi 100-200ms để đo gia tốc 3 trục (Ax, Ay, Az) và gia tốc góc (Gx, Gy, Gz).
- Tính gia tốc tuyến tính: √(Ax² + Ay² + Az²) và gia tốc góc từ sự thay đổi của gyroscope.
- Đồng thời đọc tốc độ từ GPS để xác định phương tiện có đang di chuyển hay không.

*Bước 2 - Phát hiện va chạm:*
- Thiết bị phát hiện va chạm khi **cả 2 điều kiện** đúng:
  • Tốc độ ≥ 10 km/h (phương tiện đang di chuyển)
  • Gia tốc tuyến tính > 2.5g HOẶC gia tốc góc > 300 deg/s² (có va chạm mạnh)
- Nếu chỉ có gia tốc vượt ngưỡng nhưng tốc độ < 10 km/h → Thiết bị bỏ qua (tránh cảnh báo nhầm khi xe đứng yên).

*Bước 3 - Cảnh báo và đếm ngược:*
- Khi phát hiện va chạm, thiết bị kích hoạt cảnh báo:
  • Chuyển sang trạng thái "Đang đếm ngược"
  • Bật buzzer bíp liên tục (tần suất bíp tăng dần khi gần hết thời gian)
  • Hiển thị "ACCIDENT!" và đếm ngược "00:30" → "00:00" trên màn hình OLED
  • Cho phép người dùng nhấn giữ nút Button 1 trong 5 giây để hủy SOS

*Bước 4 - Gửi SOS tự động:*
- Nếu sau 30 giây không bị hủy, thiết bị tự động:
  • Gửi SOS lên web trung tâm qua HTTP POST với thông tin: vị trí GPS, tốc độ, gia tốc, thời gian, loại SOS (tai nạn)
  • Gửi SMS đến số điện thoại cứu hộ với nội dung: "SOS Tai nạn! Vị trí: lat,lon. Thời gian: ..."
  • Gọi điện tự động đến số cứu hộ để cảnh báo khẩn cấp
  • Lưu log vào thẻ SD với thông tin chi tiết

*Bước 5 - Điều phối cứu hộ:*
- Web trung tâm nhận SOS, tự động:
  • Phân loại loại SOS (tai nạn → tìm trạm y tế)
  • Tìm kiếm trạm cứu hộ/trạm cấp cứu y tế gần nhất dựa trên vị trí GPS
  • Ưu tiên trạm có trạng thái "sẵn sàng" (ready = true)
  • Gửi cảnh báo SOS đến trạm gần nhất và phù hợp nhất
- Nếu trạm gần nhất từ chối hoặc không phản hồi trong thời gian nhất định, web trung tâm tự động tìm và liên hệ với trạm tiếp theo.

*Bước 6 - Đồng bộ và hoàn thành:*
- Thiết bị đồng bộ trạng thái SOS với server mỗi 5 giây để cập nhật tình trạng.
- Khi trạm hoàn thành nhiệm vụ hoặc SOS bị hủy, server cập nhật trạng thái, thiết bị tự động hủy SOS và trở về trạng thái bình thường.

**Phần chủ động (SOS cứu hộ thủ công):**

*Kích hoạt:*
- Khi người sử dụng gặp sự cố (xe hỏng giữa đường, hết xăng, trục trặc kỹ thuật, hoặc các tình huống khác), nhấn giữ nút Button 2 (SOS cứu hộ) trong 5 giây để kích hoạt.

*Đếm ngược và gửi SOS:*
- Thiết bị kích hoạt SOS cứu hộ thủ công:
  • Chuyển sang trạng thái "RESCUE - Đang đếm ngược"
  • Bật buzzer bíp liên tục tương tự SOS tai nạn
  • Hiển thị "RESCUE!" và đếm ngược 30 giây trên OLED
  • Người dùng có thể nhấn giữ Button 2 trong 5 giây để hủy
- Sau 30 giây (nếu không bị hủy), thiết bị tự động gửi SOS lên web trung tâm với loại "breakdown" (hỏng xe).

*Điều phối:*
- Web trung tâm nhận SOS loại "breakdown", tự động tìm và liên hệ với trạm cứu hộ gần nhất (không phải trạm y tế).
- Trạm cứu hộ nhận SOS, có thể nhận nhiệm vụ và đến hỗ trợ kịp thời.
7. THỰC NGHIỆM VÀ ĐÁNH GIÁ
**7.1. Quy trình thực nghiệm:**
- Thử nghiệm thiết bị trên xe đạp, xe máy và ô tô trong môi trường tự nhiên với các tình huống: có va chạm, không có va chạm, xe đứng yên.
- Kiểm tra khả năng phát hiện va chạm tự động, cảnh báo, và gửi SOS.
- Kiểm tra khả năng hủy SOS và SOS cứu hộ thủ công.

**7.2. Kết quả thực nghiệm:** 
	Trường hợp 1:
Khi xe không hoạt động, bị đổ, không có người xung quanh:
	Thiết bị sẽ không gửi tín hiệu SOS.
	Trường hợp 2
Khi bị tai nạn nhưng người dùng không cần thiết gọi cứu hộ, người sử dụng tắt:
	Thiết bị sẽ không gửi tín hiệu SOS.
	Trường hợp 3:
Khi xe hoạt động, xe bị va chạm:
	Thiết bị sẽ cảnh báo trong 30s (30 tiếng bip). 
	Nếu nhấn nút hủy gửi tín hiệu, thiết bị sẽ dừng lại. 
	Nếu không nhấn nút hủy gửi tín hiệu, thiết bị sẽ tự động gửi cảnh báo SOS tới trung tâm điện thoại và trung tâm.
7.3. Đánh giá sản phẩm 
•	Thiết bị đã thực hiện được yêu cầu được đặt ra: 
	Tự động đo gia tốc. 
	Tự động xử lý theo quy trình. 
•	Độ tin cậy: Các lần thực nghiệm đều thu được kết quả tương tự nhau.
7.4. Điểm mới của sản phẩm
	Sản phẩm tích hợp chức năng 3 trong 1: Đo, cảnh báo, tự động liên kết cứu hộ.
	Liên kết với web.
	Thiết bị gồm 2 chế độ phục vụ cho nhu cầu sử dụng của từng cá nhân:         
•	Chế độ tự động. 
•	Chế độ tự điều khiển trên phím bấm. 
	Kích thước sản phẩm nhỏ gọn.
	Giao diện hiển thị đơn giản, đầy đủ thông tin, dễ hiểu.
8. HƯỚNG PHÁT TRIỂN SẢN PHẨM

- **Tích hợp AI:** Ứng dụng AI để phân tích hành vi lái xe và cảnh báo khi mệt mỏi, buồn ngủ.
- **Tính năng bổ sung:** Thêm bản đồ thời tiết, tình trạng giao thông, chia sẻ hành trình cho người thân, và báo cáo các điểm nguy hiểm trên đường.
- **Mô hình Smart City:** Hướng tới mô hình thành phố thông minh kết nối các phương tiện có hộp đen, xây dựng bản đồ khu vực tai nạn dựa trên dữ liệu thu thập.
- **Dữ liệu phân tích:** Cung cấp dữ liệu cho cơ quan quản lý giao thông để phân tích điểm đen tai nạn và tạo mạng lưới cứu hộ cộng đồng.
- **Ứng dụng di động:** Phát triển ứng dụng di động (iOS, Android) để người dùng quản lý thiết bị, xem lịch sử hành trình, và nhận thông báo realtime.
- **Cải thiện pin và tiết kiệm năng lượng:** Tối ưu hóa thuật toán để giảm tiêu thụ năng lượng, kéo dài thời gian hoạt động của pin, và hỗ trợ pin mặt trời cho xe đạp.
- **Nâng cao độ chính xác:** Cải thiện thuật toán phát hiện va chạm với machine learning, kết hợp thêm cảm biến (radar, camera) để giảm false positive và tăng độ chính xác.
- **Mở rộng hỗ trợ:** Mở rộng hỗ trợ cho nhiều loại phương tiện khác như xe tải, xe buýt, xe cứu thương, và phương tiện công cộng.
- **Bảo mật và quyền riêng tư:** Tăng cường mã hóa dữ liệu, xác thực người dùng, và quản lý quyền truy cập để đảm bảo an toàn thông tin cá nhân.







TÀI LIỆU THAM KHẢO

1. TÀI LIỆU VỀ ESP32-S3 VÀ LẬP TRÌNH
[1] Espressif Systems. (2023). "ESP32-S3 Technical Reference Manual". 
    Espressif Systems. Truy cập từ: https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_cn.pdf
[2] Espressif Systems. (2023). "ESP32-S3 Datasheet". 
    Espressif Systems. Truy cập từ: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

2. TÀI LIỆU VỀ CÁC MODULE PHẦN CỨNG
[3] InvenSense. (2015). "MPU-9250 Product Specification - 9-Axis MotionTracking Device". InvenSense Inc. Truy cập từ: https://invensense.tdk.com/wp-content/uploads/2015/02/PS-MPU-9250A-01-v1.1.pdf
[4] Maxim Integrated. (2014). "DS1307 64 x 8 Serial Real-Time Clock Datasheet". Maxim Integrated Products. Truy cập từ: https://datasheets.maximintegrated.com/en/ds/DS1307.pdf

3. TÀI LIỆU VỀ THƯ VIỆN VÀ LẬP TRÌNH
[5] FreeRTOS. (2024). "FreeRTOS Real-Time Operating System Documentation". Amazon Web Services. Truy cập từ: https://www.freertos.org/Documentation/RTOS_book.html
[6] Volodymyr Shymanskyy. (2024). "TinyGSM Library - GSM/GPRS Library for Arduino". GitHub. Truy cập từ: https://github.com/vshymanskyy/TinyGSM

4. TÀI LIỆU VỀ HỆ THỐNG BLACK BOX VÀ AN TOÀN XE
[7] National Highway Traffic Safety Administration. (2023). "Event Data Recorders (EDR) in Vehicles - Technical Specifications and Standards". NHTSA. Truy cập từ: https://www.nhtsa.gov/equipment/event-data-recorders
[8] European Commission. (2022). "Vehicle Black Box Systems: Safety and Privacy. Considerations in the European Union". European Transport Research Review, 14(2), 1-15.

5. TÀI LIỆU VỀ GPS VÀ ĐỊNH VỊ
[9] Kumar, R., & Patel, S. (2023). "GPS-Based Vehicle Tracking and Monitoring System Using ESP32". International Journal of Engineering Research, 12(4), 234-241.
[10] u-blox. (2023). "NEO-8M u-blox 8 / u-blox M8 Receiver Description". 
      u-blox AG. Truy cập từ: 
      https://content.u-blox.com/sites/default/files/products/documents/NEO-8M_DataSheet_%28UBX-13003321%29.pdf

6. TÀI LIỆU VỀ SD CARD VÀ LƯU TRỮ
[11] SD Card Association. (2023). "SD Card Physical Layer Specification". 
      SD Association. Truy cập từ: https://www.sdcard.org/
[12] Taylor, B., & Moore, K. (2022). "SD Card Data Storage Optimization for 
      Embedded Systems". Journal of Embedded Computing, 15(3), 123-135.

7. TÀI LIỆU VỀ WIFI VÀ WEBSERVER
[13] Espressif Systems. (2024). "WiFi Library for ESP32-S3". 
      Truy cập từ: 
      https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html
[14] Jackson, P., & Harris, L. (2023). "Web Server Implementation on 
      Resource-Constrained IoT Devices". IEEE Internet of Things Journal, 
      10(8), 6789-6798.

8. TÀI LIỆU HƯỚNG DẪN VÀ TUTORIAL
[15] Random Nerd Tutorials. (2024). "ESP32-S3 Projects and Tutorials". 
      Truy cập từ: https://randomnerdtutorials.com/esp32/
[16] Hackster.io. (2024). "ESP32-S3 IoT Projects". 
      Truy cập từ: https://www.hackster.io/esp32/projects











PHỤ LỤC
PHỤ LỤC 1 - Địa chỉ web
https://hopdenthongminh.cloud/map

PHỤ LỤC 2 - Hình ảnh sản phẩm
 	 
 	 
**Xem chi tiết:** `PHU_LUC_01_HINH_ANH_VA_SO_DO.md`

PHỤ LỤC 3 - Cơ chế hoạt động chi tiết
**Xem chi tiết:** `PHU_LUC_02_CO_CHE_HOAT_DONG_CHI_TIET.md`

PHỤ LỤC 4 - Kết quả thực nghiệm chi tiết
**Xem chi tiết:** `PHU_LUC_03_KET_QUA_THUC_NGHIEM_CHI_TIET.md`

PHỤ LỤC 5 - Đồng bộ thông tin phần cứng
**Xem chi tiết:** `PHU_LUC_04_DONG_BO_THONG_TIN_PHAN_CUNG.md`

PHỤ LỤC 6 - Danh sách linh kiện điện – điện tử
TT	Tên mã	Thông số kỹ thuật	Cách thức hoạt động
1	ESP32-S3
 
 	-CPU 2 lõi 240 MHz, RAM 512 KB, Flash 4–8 MB
-Wi-Fi 2.4 GHz, Bluetooth 5.0 LE
-Giao tiếp: USB-OTG, UART (COM), SPI, I2C, ADC 12 bit
-Điện áp 3.0–3.6V, hỗ trợ tiết kiệm điện	-Sử dụng với module i2c 
-Hiển thị thông tin giá trị các cảm biến gửi về lên màn hình
2	MPU9250
 
	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: I2C hoặc SPI
Cảm biến: 3 trục gia tốc + 3 trục con quay hồi chuyển + 3 trục từ kế
Độ phân giải: 16-bit ADC
Phạm vi đo:
Gia tốc: ±2g/±4g/±8g/±16g
Gyro: ±250/500/1000/2000 °/s
Từ kế: ±4800 μT
Nhiệt độ làm việc: -40°C đến 85°C	Vi điều khiển đọc dữ liệu qua I2C hoặc SPI.
MPU-9250 đo gia tốc, tốc độ góc và từ trường theo 3 trục.
Dữ liệu dùng để xác định vận tốc, góc nghiêng, hướng và định hướng thiết bị.
3	OLED 0.96inch
 	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: I2C (SDA, SCL) hoặc SPI
Kích thước phổ biến: 0.96", 1.3"
Độ phân giải: 128 × 64 hoặc 128 × 32 pixels
Hiển thị: Trắng, vàng hoặc đa màu tùy module
	Vi điều khiển gửi dữ liệu hình ảnh hoặc ký tự qua I2C/SPI.
OLED tự phát sáng từng pixel, không cần đèn nền.
Hiển thị thông tin từ cảm biến, trạng thái thiết bị hoặc dữ liệu điều khiển.
Tiêu thụ điện thấp, hiển thị sắc nét ngay cả ở điều kiện thiếu sáng.
4	Còi buzzer
 	Điện áp hoạt động: 3.3V – 5V
Dòng tiêu thụ: ~30mA – 100mA
Loại: Active Buzzer hoặc Passive Buzzer
Active Buzzer: Phát ra âm thanh ngay khi có điện áp cấp.
Passive Buzzer: Phát ra âm thanh khi có tín hiệu PWM hoặc tần số điều khiển.
Tần số âm thanh (nếu có): 2kHz – 5kHz (tuỳ loại)	Active Buzzer: Khi vi điều khiển cấp điện, còi phát ra âm thanh.
Passive Buzzer: Phát âm thanh khi có tín hiệu điều khiển từ vi điều khiển, thường là tín hiệu PWM.
Dùng trong các ứng dụng cảnh báo, thông báo hoặc phản hồi âm thanh.
Kết hợp với delay hoặc PWM để điều khiển tần số và thời gian phát âm thanh.
5	Mạch tăng áp
 
	Điện áp đầu vào: DC 3V – 40V
Điện áp đầu ra: DC 5V – 35V (tùy model)
Dòng ra: 3A (tối đa, tùy vào model)
Hiệu suất: 80% – 95%
Kích thước: 45mm x 20mm x 14mm (tùy module)	Mạch tăng áp chuyển đổi điện áp đầu vào thấp lên mức điện áp cao hơn.
Được sử dụng để cung cấp nguồn cho các thiết bị cần điện áp cao hơn so với nguồn cấp.
Vi điều khiển điều khiển mạch qua PWM để điều chỉnh điện áp đầu ra.
Cung cấp dòng điện ổn định cho các thiết bị như cảm biến, động cơ nhỏ, hoặc mạch khác cần điện áp cao hơn.
6	RTC module 
 	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: I2C (SCL, SDA)
Độ chính xác cao nhờ thạch anh bù nhiệt (TCXO)
Có pin dự phòng để giữ thời gian khi mất nguồn
Nhiệt độ làm việc: -40°C đến 85°C	Module tự đếm và lưu thời gian (giờ, phút, giây, ngày, tháng, năm).
Vi điều khiển giao tiếp qua I2C để đọc hoặc cài đặt thời gian.
Khi mất điện, pin backup tiếp tục duy trì bộ đếm, giúp hệ thống luôn có thời gian chính xác.
7	INMP441
 	Điện áp hoạt động: 3.3V
Giao tiếp: I2S (SCK, WS, SD)
Tần số mẫu: 8 kHz – 48 kHz
Độ nhạy: -26 dB ±3 dB
Dải tần số: 100 Hz – 10 kHz
Độ ồn: 63 dBA
Dòng tiêu thụ: 1.2 mA	Mic thu âm và chuyển tín hiệu analog thành tín hiệu số qua giao tiếp I2S.
Vi điều khiển sử dụng giao tiếp I2S để nhận dữ liệu âm thanh.
Dữ liệu âm thanh có thể được sử dụng để xử lý, phân tích hoặc ghi âm.
Phù hợp với các ứng dụng giao tiếp giọng nói, nhận diện âm thanh hoặc thu âm.
8	GPS NEO8M
 	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: UART (TX/RX), I2C (tùy module)
Tần số nhận: 1 Hz – 10 Hz
Độ nhạy: -161 dBm (tracking), -148 dBm (cold start)
Thời gian khởi động: Cold start < 30s, Hot start < 1s
Hỗ trợ định vị toàn cầu: GPS + GLONASS + Galileo	Module nhận tín hiệu vệ tinh GPS/GLONASS để xác định tọa độ (lat, long), độ cao, tốc độ và thời gian UTC.
Gửi dữ liệu ra vi điều khiển qua UART hoặc I2C dưới dạng chuỗi NMEA.
Vi điều khiển có thể sử dụng dữ liệu để ghi log hành trình, định vị.
9	DHT11
 	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: Digital (1 dây)
Phạm vi đo:
Nhiệt độ: 0 – 50°C, độ chính xác ±2°C
Độ ẩm: 20 – 90% RH, độ chính xác ±5% RH
Tần số lấy mẫu: ~1 Hz
Dòng tiêu thụ: ~2.5 mA khi đo	Vi điều khiển gửi tín hiệu yêu cầu → DHT11 trả về dữ liệu nhiệt độ và độ ẩm dạng số.
Dữ liệu có thể dùng để hiển thị, ghi log hoặc điều khiển thiết bị theo nhiệt độ/độ ẩm.
Cảm biến chỉ có ngõ ra digital, không có analog.

10	SIM7682
 	Điện áp hoạt động: 3.4 – 4.4V
Dòng điện: Idle ~20 mA, Tối đa ~2A khi gọi
Giao tiếp: UART (TX/RX)
Tần số: GSM 850/900/1800/1900 MHz
Kích thước: 15.8 × 17.8 × 2.4 mm
Nhiệt độ làm việc: -40°C đến 85°C
	Module kết nối mạng GSM/GPRS để gọi điện, gửi/nhận SMS hoặc dữ liệu Internet.
Dữ liệu từ cảm biến có thể được gửi trực tiếp tới điện thoại hoặc server khi phát hiện sự kiện.
Hoạt động cần nguồn.
11	Pin Lipo-10000mah
 	Điện áp danh định: 3.7V
Dung lượng: Dòng chữ khá mờ, nhưng thường loại pin này dao động từ 2000mAh – 5000mAh (tùy mã).
Hình dạng: Dạng túi (pouch), bọc lớp nhôm và viền vàng.
Dây kết nối:
Dây đỏ: cực dương (+)
Dây đen: cực âm (−)	Pin cấp nguồn cho hệ thống
Sạc nguồn cho pin
12	Mạch sạc TP4056 
 	Điện áp đầu vào: 5V DC (Type-C USB)
Dòng sạc: 1A hoặc 2A tùy module
Điện áp pin Li-ion: 3.7V – 4.2V
Bảo vệ: quá dòng, quá áp, ngắn mạch
Giao tiếp/Chỉ báo: LED báo sạc/đầy
	Nguồn USB cung cấp điện áp 5V → TP4056 điều chỉnh dòng sạc và điện áp phù hợp với pin Li-ion.
Khi pin đầy → mạch tự ngắt dòng sạc, LED báo trạng thái.
Có thể sạc an toàn pin đơn 3.7V và bảo vệ pin khỏi quá tải.
13	Công tắc 
 	Điện áp hoạt động: 3.3V – 5V
Dòng chịu được: tối đa ~100–200 mA
Loại: 2 vị trí (ON/OFF hoặc trái/phải)
Kích thước: tùy module (~10–20 mm)	Gạt sang trái hoặc phải → đóng/mở mạch tương ứng với vị trí.
Vi điều khiển đọc trạng thái digital từ chân công tắc để bật/tắt thiết bị hoặc thay đổi chế độ vận hành.
Thường dùng pull-up/pull-down resistor để đảm bảo tín hiệu ổn định.
14
	Nút bấm
 	Điện áp hoạt động: 3.3V – 5V
Dòng chịu được: ~50mA – 100mA (tuỳ loại)
Loại: Normally Open (NO) hoặc Normally Closed (NC)
Kích thước: ~10mm – 20mm (tuỳ loại)	Khi nhấn nút, mạch đóng (NO) hoặc mở (NC) tùy thuộc vào loại công tắc.
Vi điều khiển đọc trạng thái (ON/OFF) từ nút bấm để kích hoạt hoặc tắt thiết bị, thay đổi chế độ.
Thường sử dụng resistor pull-up hoặc pull-down để đảm bảo tín hiệu ổn định khi nút không được nhấn.
15	Micro sd card adapter
 	Điện áp hoạt động: 3.3V – 5V
Giao tiếp: SPI (MOSI, MISO, SCK, CS)
Hỗ trợ thẻ SD/SDHC (tối đa 32 GB)
Kích thước thẻ: chuẩn micro SD hoặc SD tùy module
Cách thức hoạt động
Vi điều khiển giao tiếp với module qua SPI để đọc/ghi dữ liệu.	Vi điều khiển giao tiếp với module qua SPI để đọc/ghi dữ liệu.
Module chuyển đổi tín hiệu SPI thành dạng mà thẻ SD hiểu được.
Dữ liệu từ thẻ SD có thể lưu trữ log cảm biến, hình ảnh hoặc các file dữ liệu khác.
Cấp nguồn cho module và thẻ SD từ board, đảm bảo điện áp ổn định.


