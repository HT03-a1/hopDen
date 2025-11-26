import * as fs from 'fs';
import * as path from 'path';
import { User, Station } from '../types';

const DATA_DIR = path.join(__dirname, '../../data');

// Ensure data directory exists
if (!fs.existsSync(DATA_DIR)) {
  fs.mkdirSync(DATA_DIR, { recursive: true });
}

export function readJson<T>(filename: string): T[] {
  const filePath = path.join(DATA_DIR, filename);
  try {
    if (!fs.existsSync(filePath)) {
      return [];
    }
    const data = fs.readFileSync(filePath, 'utf-8');
    if (!data || data.trim() === '') {
      console.warn(`⚠️ Warning: ${filename} is empty - file exists but has no content`);
      return [];
    }
    const parsed = JSON.parse(data);
    if (!Array.isArray(parsed)) {
      console.error(`❌ Error: ${filename} does not contain a valid array. Content: ${data.substring(0, 100)}`);
      return [];
    }
    return parsed;
  } catch (error: any) {
    console.error(`❌ CRITICAL: Error reading ${filename}:`, error.message);
    console.error(`   File path: ${filePath}`);
    console.error(`   This may cause data loss! Please check the file manually.`);
    // Vẫn trả về [] để app không crash, nhưng log rõ ràng để debug
    return [];
  }
}

export function writeJson<T>(filename: string, data: T[]): void {
  const filePath = path.join(DATA_DIR, filename);
  try {
    // Cảnh báo nếu đang ghi mảng rỗng vào file đã có dữ liệu
    if (data.length === 0 && fs.existsSync(filePath)) {
      const existingData = fs.readFileSync(filePath, 'utf-8');
      if (existingData && existingData.trim() !== '' && existingData.trim() !== '[]') {
        console.warn(`⚠️ WARNING: Writing empty array to ${filename} which previously had data!`);
        console.warn(`   Previous content length: ${existingData.length} bytes`);
      }
    }
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf-8');
  } catch (error) {
    console.error(`Error writing ${filename}:`, error);
    throw error;
  }
}

export function initializeData(): void {
  // Initialize users.json
  // CHỈ tạo seed data khi file KHÔNG tồn tại, KHÔNG tạo lại khi file rỗng (người dùng có thể đã xóa)
  const usersPath = path.join(DATA_DIR, 'users.json');
  if (!fs.existsSync(usersPath)) {
    const seedUsers: User[] = [
      {
        id: 'U0001',
        name: 'Nguyễn Văn An',
        email: 'nguyenvanan@example.com',
        password: 'password123', // In production, should be hashed
        phone: '0912345678',
        address: '123 Đường ABC, Quận 1, TP.HCM',
        lat: 10.7769,
        lon: 106.7009,
        createdAt: new Date().toISOString()
      },
      {
        id: 'U0002',
        name: 'Trần Thị Bình',
        email: 'tranthibinh@example.com',
        password: 'password123',
        phone: '0912345679',
        address: '456 Đường XYZ, Quận 3, TP.HCM',
        lat: 10.7829,
        lon: 106.6909,
        createdAt: new Date().toISOString()
      },
      {
        id: 'U0003',
        name: 'Lê Văn Cường',
        email: 'levancuong@example.com',
        password: 'password123',
        phone: '0912345680',
        address: '789 Đường DEF, Quận 5, TP.HCM',
        lat: 10.7559,
        lon: 106.6679,
        createdAt: new Date().toISOString()
      },
      {
        id: 'U0004',
        name: 'Phạm Thị Dung',
        email: 'phamthidung@example.com',
        password: 'password123',
        phone: '0912345681',
        address: '321 Đường GHI, Quận 7, TP.HCM',
        lat: 10.7309,
        lon: 106.7209,
        createdAt: new Date().toISOString()
      },
      {
        id: 'U0005',
        name: 'Hoàng Văn Em',
        email: 'hoangvanem@example.com',
        password: 'password123',
        phone: '0912345682',
        address: '654 Đường JKL, Quận 10, TP.HCM',
        lat: 10.7739,
        lon: 106.6679,
        createdAt: new Date().toISOString()
      }
    ];
    writeJson('users.json', seedUsers);
    console.log('Created users.json with seed data (file did not exist)');
  } else {
    const users = readJson<User>('users.json');
    if (users.length > 0) {
      console.log(`users.json already exists with ${users.length} user(s) - skipping seed data creation`);
    } else {
      console.log('users.json exists but is empty - keeping it empty (user may have deleted all users)');
    }
  }

  // Initialize stations.json
  const stationsPath = path.join(DATA_DIR, 'stations.json');
  const hasStationsFile = fs.existsSync(stationsPath);

  if (!hasStationsFile) {
    const seedStations: Station[] = [
      {
        id: 'S0031',
        stationName: 'Bệnh viện Đại học Quốc gia Hà Nội',
        type: 'medical',
        email: 'contact@benhviendhqghn.vn',
        password: '12345678',
        phone: '02435544833',
        address: '182 Lương Thế Vinh, Thanh Xuân Bắc, Thanh Xuân, Hà Nội',
        lat: 20.99254,
        lon: 105.79031,
        openHours: '24/7',
        description: 'Bệnh viện trực thuộc Đại học Quốc gia Hà Nội, cung cấp dịch vụ khám chữa bệnh tổng quát, cấp cứu và các gói xét nghiệm chuyên sâu cho cán bộ, sinh viên và người dân khu vực phía Tây Hà Nội.',
        ratingAvg: 0,
        ratingCount: 0,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0032',
        stationName: 'Bệnh viện Giao thông Vận tải',
        type: 'medical',
        email: 'contact@benhviengtvt.vn',
        password: '12345678',
        phone: '02437664751',
        address: '1194 Đường Láng, Chợ Dừa, Đống Đa, Hà Nội',
        lat: 21.02279,
        lon: 105.81397,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa ngành Giao thông Vận tải, chuyên cấp cứu tai nạn giao thông, phẫu thuật chấn thương và khám sức khỏe lái xe. Có đầy đủ khoa cấp cứu, hồi sức, cận lâm sàng và phòng khám ngoài giờ',
        ratingAvg: 0,
        ratingCount: 0,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0033',
        stationName: 'Cứu Hộ Xe Hơi 24/24 SửaChữa. CâuBình. VáLốp',
        type: 'rescue',
        email: 'cuuhohehoi@gmail.com',
        password: '12345678',
        phone: '0969454669',
        address: 'XRX7+66 Thanh Xuân, Hanoi, Vietnam',
        lat: 21.0244162,
        lon: 105.7762249,
        openHours: '24/7',
        description: '“Hỏng đâu cứu đó – an tâm mọi hành trình.”\n “Xe hỏng có chúng tôi – an toàn tiếp tục lăn bánh.”\n “Hỗ trợ khẩn cấp – sửa chữa tận tâm.”\n “Đồng hành trên mọi cung đường.”',
        ratingAvg: 0,
        ratingCount: 0,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0034',
        stationName: 'Sửa Xe Máy Hà Nội - Cứu Hộ Xe Máy 24h',
        type: 'rescue',
        email: 'cuuhoxemay@gmail.com',
        password: '12345678',
        phone: '0812788663',
        address: '144 P. Thái Thịnh, Thịnh Quang, Đống Đa, Hà Nội, Vietnam',
        lat: 21.0244162,
        lon: 105.7762249,
        openHours: '24/7',
        description: 'Trạm cứu hộ & sửa xe của chúng tôi luôn sẵn sàng đồng hành trên mọi cung đường, có mặt ngay khi bạn cần. Dù là sự cố nhỏ hay hỏng hóc nghiêm trọng, đội kỹ thuật luôn phản ứng nhanh, sửa chữa chuẩn xác và đảm bảo chiếc xe của bạn trở lại trạng thái an toàn. “Hỏng đâu cứu đó – an tâm mọi hành trình” là cam kết mà chúng tôi mang đến cho mỗi khách hàng.',
        ratingAvg: 0,
        ratingCount: 0,
        createdAt: new Date().toISOString()
      }
    ];

    writeJson('stations.json', seedStations);
    console.log(`Created stations.json with ${seedStations.length} seed stations`);
  } else {
    console.log('stations.json already exists - skip seeding stations');
  }

  // Initialize other JSON files
  const files = ['devices.json', 'sos.json', 'ratings.json', 'telemetry.json'];
  files.forEach(file => {
    const filePath = path.join(DATA_DIR, file);
    if (!fs.existsSync(filePath)) {
      writeJson(file, []);
      console.log(`Created ${file}`);
    }
  });

  // Generate taikhoan.md file (only once during initialization)
  // Removed duplicate call from index.ts
  generateTaiKhoanFile();
}

export function generateTaiKhoanFile(): void {
  try {
    const users = readJson<User>('users.json');
    const stations = readJson<Station>('stations.json');
    
    const taikhoanPath = path.join(DATA_DIR, 'taikhoan.md');
    let content = '# Danh sách tài khoản đăng nhập\n\n';
    content += `*Cập nhật lần cuối: ${new Date().toLocaleString('vi-VN')}*\n\n`;
    
    // Users section
    content += '## 👤 Người dùng (Users)\n\n';
    content += '| ID | Tên | Email | Mật khẩu | Số điện thoại |\n';
    content += '|----|-----|-------|----------|---------------|\n';
    users.forEach(user => {
      content += `| ${user.id} | ${user.name} | ${user.email} | ${user.password} | ${user.phone} |\n`;
    });
    
    content += '\n---\n\n';
    
    // Medical stations section
    const medicalStations = stations.filter(s => s.type === 'medical');
    content += '## 🏥 Trạm y tế (Medical Stations)\n\n';
    content += '| ID | Tên trạm | Email | Mật khẩu | Số điện thoại | Địa chỉ |\n';
    content += '|----|----------|-------|----------|---------------|----------|\n';
    medicalStations.forEach(station => {
      content += `| ${station.id} | ${station.stationName} | ${station.email} | ${station.password} | ${station.phone} | ${station.address} |\n`;
    });
    
    content += '\n---\n\n';
    
    // Rescue stations section
    const rescueStations = stations.filter(s => s.type === 'rescue');
    content += '## 🚑 Trạm cứu hộ (Rescue Stations)\n\n';
    content += '| ID | Tên trạm | Email | Mật khẩu | Số điện thoại | Địa chỉ |\n';
    content += '|----|----------|-------|----------|---------------|----------|\n';
    rescueStations.forEach(station => {
      content += `| ${station.id} | ${station.stationName} | ${station.email} | ${station.password} | ${station.phone} | ${station.address} |\n`;
    });
    
    content += '\n---\n\n';
    
    // Repair stations đã được gộp vào rescue stations, không cần section riêng
    
    content += '\n---\n\n';
    content += '## 📝 Hướng dẫn đăng nhập\n\n';
    content += '1. **Người dùng**: Chọn role "Người dùng" khi đăng nhập\n';
    content += '2. **Trạm y tế**: Chọn role "Trạm y tế" khi đăng nhập\n';
    content += '3. **Trạm cứu hộ**: Chọn role "Trạm cứu hộ" khi đăng nhập (bao gồm cả trạm sửa xe, đã được gộp vào trạm cứu hộ)\n';
    
    fs.writeFileSync(taikhoanPath, content, 'utf-8');
    console.log('Created taikhoan.md with all account information');
  } catch (error) {
    console.error('Error generating taikhoan.md:', error);
  }
}

