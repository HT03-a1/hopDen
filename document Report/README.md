# 📁 CẤU TRÚC THƯ MỤC BÁO CÁO

## 📋 TỔNG QUAN

Thư mục này chứa báo cáo đề tài KHKT: **"THIẾT BỊ GIÁM SÁT TAI NẠN GIAO THÔNG VÀ WEB CỨU HỘ THÔNG MINH"**

---

## 📂 CẤU TRÚC FILE

### 📄 File chính

1. **`New Text Document.md`** - Báo cáo chính
   - Nội dung tổng quan về đề tài
   - Mục lục, lí do chọn đề tài, mục tiêu nghiên cứu
   - Phương pháp, nội dung, tiến hành nghiên cứu
   - Thực nghiệm và đánh giá
   - Hướng phát triển sản phẩm
   - Tài liệu tham khảo
   - Phụ lục (tham chiếu đến các file phụ lục chi tiết)

---

### 📚 File phụ lục (bổ sung chi tiết)

Các file phụ lục được tách riêng để **tránh làm loạn file chính** và dễ quản lý:

2. **`PHU_LUC_01_HINH_ANH_VA_SO_DO.md`** - Hình ảnh và sơ đồ
   - Mô tả hình ảnh sản phẩm cần chèn
   - Mô tả sơ đồ khối hệ thống
   - Mô tả sơ đồ thuật toán chi tiết
   - Hướng dẫn chèn hình ảnh vào Markdown

3. **`PHU_LUC_02_CO_CHE_HOAT_DONG_CHI_TIET.md`** - Cơ chế hoạt động chi tiết
   - Quy trình khởi động hệ thống
   - Quy trình phát hiện va chạm/tai nạn tự động
   - Quy trình đếm ngược và cảnh báo
   - Quy trình gửi SOS tự động
   - Quy trình đồng bộ 2 chiều với server
   - Quy trình SOS cứu hộ thủ công
   - Quy trình gửi dữ liệu telemetry
   - Quy trình ghi nhật ký liên tục
   - Quy trình hiển thị trên OLED
   - Quy trình cảnh báo nhiệt độ/độ ẩm
   - Quy trình xử lý lỗi và phục hồi

4. **`PHU_LUC_03_KET_QUA_THUC_NGHIEM_CHI_TIET.md`** - Kết quả thực nghiệm chi tiết
   - Quy trình thực nghiệm
   - Các trường hợp thử nghiệm chi tiết
   - Kết quả định lượng (bảng thống kê)
   - Đánh giá sản phẩm (ưu điểm, hạn chế, điểm mới)
   - Kết luận

5. **`PHU_LUC_04_DONG_BO_THONG_TIN_PHAN_CUNG.md`** - Đồng bộ thông tin phần cứng
   - So sánh linh kiện giữa báo cáo và code thực tế
   - So sánh chân GPIO
   - So sánh thông số kỹ thuật
   - So sánh chức năng
   - Danh sách cần cập nhật báo cáo
   - Khuyến nghị

6. **`Báo cáo - Đề tài KHKT - THIẾT BỊ GIÁM SÁT TAI NẠN GIAO THÔNG VÀ WEB CỨU HỘ THÔNG MINH-5.docx`** - File Word gốc
   - File báo cáo Word ban đầu
   - Có thể sử dụng để export PDF

---

## 🖼️ THƯ MỤC HÌNH ẢNH (Cần tạo)

**Tạo thư mục:** `document Report/images/`

**Chứa các hình ảnh:**
- `hinh_1.1_san_pham_hoan_thien.jpg` - Sản phẩm hoàn thiện
- `hinh_1.2_cau_tao_ben_trong.jpg` - Cấu tạo bên trong
- `hinh_1.3_lap_dat_tren_xe.jpg` - Lắp đặt trên xe
- `so_do_khoi_tong_quan.png` hoặc `.svg` - Sơ đồ khối tổng quan
- `so_do_ket_noi_linh_kien.png` hoặc `.svg` - Sơ đồ kết nối linh kiện
- `so_do_thuat_toan_hop_den.png` hoặc `.svg` - Sơ đồ thuật toán hộp đen
- `so_do_thuat_toan_esp_gui_du_lieu.png` hoặc `.svg` - Sơ đồ thuật toán ESP gửi dữ liệu
- `so_do_thuat_toan_tim_tram_gan_nhat.png` hoặc `.svg` - Sơ đồ thuật toán tìm trạm gần nhất
- `so_do_thuat_toan_dieu_phoi_sos_tu_dong.png` hoặc `.svg` - Sơ đồ thuật toán điều phối SOS tự động
- `so_do_thuat_toan_xu_ly_sos_tram.png` hoặc `.svg` - Sơ đồ thuật toán xử lý SOS của trạm

---

## 📖 CÁCH SỬ DỤNG

### 1. Đọc báo cáo

- Bắt đầu với file chính: **`New Text Document.md`**
- Khi cần chi tiết, tham chiếu đến các file phụ lục tương ứng

### 2. Bổ sung nội dung

- **Hình ảnh:** Thêm vào thư mục `images/` và cập nhật `PHU_LUC_01_HINH_ANH_VA_SO_DO.md`
- **Cơ chế hoạt động:** Cập nhật `PHU_LUC_02_CO_CHE_HOAT_DONG_CHI_TIET.md`
- **Kết quả thực nghiệm:** Cập nhật `PHU_LUC_03_KET_QUA_THUC_NGHIEM_CHI_TIET.md`
- **Đồng bộ thông tin:** Cập nhật `PHU_LUC_04_DONG_BO_THONG_TIN_PHAN_CUNG.md`

### 3. Xuất PDF

- Có thể sử dụng file Word gốc để export PDF
- Hoặc sử dụng tool như Pandoc để convert Markdown sang PDF:
  ```bash
  pandoc "New Text Document.md" -o "Bao_Cao.pdf"
  ```

### 4. Chèn hình ảnh vào Markdown

Sử dụng cú pháp:
```markdown
![Mô tả hình ảnh](images/ten_file_hinh.jpg)
```

Ví dụ:
```markdown
![Sản phẩm hoàn thiện](images/hinh_1.1_san_pham_hoan_thien.jpg)
```

---

## 🔄 CẬP NHẬT

### Lần cập nhật gần nhất: 2025-01-XX

**Thay đổi:**
- ✅ Tách các phụ lục ra file riêng
- ✅ Tạo file đồng bộ thông tin phần cứng
- ✅ Thêm mô tả chi tiết về cơ chế hoạt động
- ✅ Thêm kết quả thực nghiệm chi tiết
- ✅ Cập nhật tham chiếu trong file chính

---

## 📝 LƯU Ý

1. **Giữ cấu trúc:** Không di chuyển hoặc đổi tên file một cách tùy ý
2. **Đồng bộ:** Cập nhật file `PHU_LUC_04_DONG_BO_THONG_TIN_PHAN_CUNG.md` nếu có thay đổi code
3. **Hình ảnh:** Đảm bảo tên file hình ảnh khớp với tên trong `PHU_LUC_01_HINH_ANH_VA_SO_DO.md`
4. **Version control:** Sử dụng Git để theo dõi thay đổi

---

## 🆘 HỖ TRỢ

Nếu cần hỗ trợ:
- Kiểm tra file `PHU_LUC_04_DONG_BO_THONG_TIN_PHAN_CUNG.md` để so sánh với code thực tế
- Xem file `hardware/SO_DO_KET_NOI_ESP32_S3.md` để kiểm tra sơ đồ kết nối
- Xem file `CHUC_NANG_HOP_DEN.md` ở thư mục gốc để xem chức năng chi tiết

---

**Chúc bạn hoàn thành báo cáo thành công! 🚀**

