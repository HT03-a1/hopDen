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
    return JSON.parse(data);
  } catch (error) {
    console.error(`Error reading ${filename}:`, error);
    return [];
  }
}

export function writeJson<T>(filename: string, data: T[]): void {
  const filePath = path.join(DATA_DIR, filename);
  try {
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf-8');
  } catch (error) {
    console.error(`Error writing ${filename}:`, error);
    throw error;
  }
}

export function initializeData(): void {
  // Initialize users.json
  const usersPath = path.join(DATA_DIR, 'users.json');
  if (!fs.existsSync(usersPath)) {
    const seedUsers = [
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
    console.log('Created users.json with seed data');
  }

  // Initialize stations.json
  const stationsPath = path.join(DATA_DIR, 'stations.json');
  const existingStations = readJson<Station>('stations.json');
  
  // Always check and add missing stations
  if (!fs.existsSync(stationsPath) || existingStations.length < 30) {
    const seedStations = [
      {
        id: 'S0001',
        stationName: 'Bệnh viện Chợ Rẫy',
        type: 'medical',
        email: 'cho-ray@example.com',
        password: 'password123',
        phone: '02838554137',
        address: '201B Nguyễn Chí Thanh, Quận 5, TP.HCM',
        lat: 10.7559,
        lon: 106.6679,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa trung ương',
        ratingAvg: 4.5,
        ratingCount: 120,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0002',
        stationName: 'Bệnh viện Nhân dân 115',
        type: 'medical',
        email: 'benhvien115@example.com',
        password: 'password123',
        phone: '02838654123',
        address: '527 Sư Vạn Hạnh, Quận 10, TP.HCM',
        lat: 10.7739,
        lon: 106.6679,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa',
        ratingAvg: 4.3,
        ratingCount: 95,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0003',
        stationName: 'Bệnh viện Đại học Y Dược',
        type: 'medical',
        email: 'bvdhyd@example.com',
        password: 'password123',
        phone: '02838554101',
        address: '215 Hồng Bàng, Quận 5, TP.HCM',
        lat: 10.7559,
        lon: 106.6609,
        openHours: '7:00 - 17:00',
        description: 'Bệnh viện đại học',
        ratingAvg: 4.4,
        ratingCount: 88,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0004',
        stationName: 'Bệnh viện FV',
        type: 'medical',
        email: 'fv@example.com',
        password: 'password123',
        phone: '02854113535',
        address: '6 Nguyễn Lương Bằng, Quận 7, TP.HCM',
        lat: 10.7309,
        lon: 106.7209,
        openHours: '24/7',
        description: 'Bệnh viện quốc tế',
        ratingAvg: 4.7,
        ratingCount: 200,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0005',
        stationName: 'Trung tâm Y tế Quận 1',
        type: 'medical',
        email: 'yttq1@example.com',
        password: 'password123',
        phone: '02838291234',
        address: '34 Lý Tự Trọng, Quận 1, TP.HCM',
        lat: 10.7769,
        lon: 106.7009,
        openHours: '7:00 - 20:00',
        description: 'Trung tâm y tế quận',
        ratingAvg: 4.2,
        ratingCount: 65,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0006',
        stationName: 'Trạm cứu hộ 24/7 Quận 1',
        type: 'rescue',
        email: 'cuuhocq1@example.com',
        password: 'password123',
        phone: '02838291111',
        address: '123 Nguyễn Huệ, Quận 1, TP.HCM',
        lat: 10.7769,
        lon: 106.7029,
        openHours: '24/7',
        description: 'Dịch vụ cứu hộ xe máy, ô tô',
        ratingAvg: 4.6,
        ratingCount: 150,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0007',
        stationName: 'Trạm sửa xe nhanh Quận 3',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxeq3@example.com',
        password: 'password123',
        phone: '02839301234',
        address: '456 Võ Văn Tần, Quận 3, TP.HCM',
        lat: 10.7829,
        lon: 106.6909,
        openHours: '6:00 - 22:00',
        description: 'Sửa chữa xe máy, ô tô',
        ratingAvg: 4.5,
        ratingCount: 180,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0008',
        stationName: 'Cứu hộ đường cao tốc',
        type: 'rescue',
        email: 'cuuhoccaotoc@example.com',
        password: 'password123',
        phone: '02838292222',
        address: '789 Đại lộ Võ Văn Kiệt, Quận 5, TP.HCM',
        lat: 10.7559,
        lon: 106.6659,
        openHours: '24/7',
        description: 'Cứu hộ chuyên nghiệp',
        ratingAvg: 4.8,
        ratingCount: 220,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0009',
        stationName: 'Garage sửa xe Quận 7',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garageq7@example.com',
        password: 'password123',
        phone: '02854111234',
        address: '321 Nguyễn Thị Thập, Quận 7, TP.HCM',
        lat: 10.7309,
        lon: 106.7189,
        openHours: '7:00 - 21:00',
        description: 'Sửa chữa và bảo dưỡng',
        ratingAvg: 4.4,
        ratingCount: 140,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0010',
        stationName: 'Trạm cứu hộ Quận 10',
        type: 'rescue',
        email: 'cuuhocq10@example.com',
        password: 'password123',
        phone: '02838651234',
        address: '654 Sư Vạn Hạnh, Quận 10, TP.HCM',
        lat: 10.7739,
        lon: 106.6659,
        openHours: '24/7',
        description: 'Dịch vụ cứu hộ toàn diện',
        ratingAvg: 4.7,
        ratingCount: 190,
        createdAt: new Date().toISOString()
      },
      // Hà Nội - 10 Trạm y tế
      {
        id: 'S0011',
        stationName: 'Bệnh viện Bạch Mai',
        type: 'medical',
        email: 'bachmai@example.com',
        password: 'password123',
        phone: '02438623731',
        address: '78 Giải Phóng, Phương Mai, Đống Đa, Hà Nội',
        lat: 21.0014,
        lon: 105.8425,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa trung ương',
        ratingAvg: 4.6,
        ratingCount: 250,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0012',
        stationName: 'Bệnh viện Việt Đức',
        type: 'medical',
        email: 'vietduc@example.com',
        password: 'password123',
        phone: '02438253531',
        address: '40 Tràng Thi, Hoàn Kiếm, Hà Nội',
        lat: 21.0278,
        lon: 105.8514,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa trung ương',
        ratingAvg: 4.7,
        ratingCount: 280,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0013',
        stationName: 'Bệnh viện Nhi Trung ương',
        type: 'medical',
        email: 'nhitrunguong@example.com',
        password: 'password123',
        phone: '02462738831',
        address: '18/879 La Thành, Láng Thượng, Đống Đa, Hà Nội',
        lat: 21.0144,
        lon: 105.8022,
        openHours: '24/7',
        description: 'Bệnh viện chuyên khoa nhi',
        ratingAvg: 4.5,
        ratingCount: 200,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0014',
        stationName: 'Bệnh viện Đại học Y Hà Nội',
        type: 'medical',
        email: 'dhydhn@example.com',
        password: 'password123',
        phone: '02438523731',
        address: '1 Tôn Thất Tùng, Trung Tự, Đống Đa, Hà Nội',
        lat: 21.0014,
        lon: 105.8364,
        openHours: '7:00 - 17:00',
        description: 'Bệnh viện đại học',
        ratingAvg: 4.4,
        ratingCount: 180,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0015',
        stationName: 'Bệnh viện E',
        type: 'medical',
        email: 'benhviene@example.com',
        password: 'password123',
        phone: '02438261216',
        address: '89 Trần Cung, Nghĩa Tân, Cầu Giấy, Hà Nội',
        lat: 21.0431,
        lon: 105.8014,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa',
        ratingAvg: 4.3,
        ratingCount: 150,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0016',
        stationName: 'Bệnh viện Hữu Nghị',
        type: 'medical',
        email: 'huunghi@example.com',
        password: 'password123',
        phone: '02438523731',
        address: '1 Trần Khánh Dư, Bạch Đằng, Hai Bà Trưng, Hà Nội',
        lat: 21.0083,
        lon: 105.8611,
        openHours: '24/7',
        description: 'Bệnh viện đa khoa',
        ratingAvg: 4.5,
        ratingCount: 170,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0017',
        stationName: 'Bệnh viện Thanh Nhàn',
        type: 'medical',
        email: 'thanhnhan@example.com',
        password: 'password123',
        phone: '02438623731',
        address: '42 Thanh Nhàn, Hai Bà Trưng, Hà Nội',
        lat: 20.9986,
        lon: 105.8569,
        openHours: '7:00 - 20:00',
        description: 'Bệnh viện đa khoa',
        ratingAvg: 4.2,
        ratingCount: 120,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0018',
        stationName: 'Bệnh viện Phụ sản Hà Nội',
        type: 'medical',
        email: 'phusanhn@example.com',
        password: 'password123',
        phone: '02438253731',
        address: '929 La Thành, Ngọc Khánh, Ba Đình, Hà Nội',
        lat: 21.0306,
        lon: 105.8083,
        openHours: '24/7',
        description: 'Bệnh viện chuyên khoa phụ sản',
        ratingAvg: 4.6,
        ratingCount: 220,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0019',
        stationName: 'Bệnh viện Tim Hà Nội',
        type: 'medical',
        email: 'timhn@example.com',
        password: 'password123',
        phone: '02438253731',
        address: '92 Trần Hưng Đạo, Hoàn Kiếm, Hà Nội',
        lat: 21.0247,
        lon: 105.8542,
        openHours: '24/7',
        description: 'Bệnh viện chuyên khoa tim mạch',
        ratingAvg: 4.8,
        ratingCount: 300,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0020',
        stationName: 'Bệnh viện Mắt Trung ương',
        type: 'medical',
        email: 'mattruong@example.com',
        password: 'password123',
        phone: '02438253731',
        address: '85 Bà Triệu, Hoàn Kiếm, Hà Nội',
        lat: 21.0242,
        lon: 105.8508,
        openHours: '7:00 - 17:00',
        description: 'Bệnh viện chuyên khoa mắt',
        ratingAvg: 4.5,
        ratingCount: 190,
        createdAt: new Date().toISOString()
      },
      // Hà Nội - 10 Trạm sửa xe
      {
        id: 'S0021',
        stationName: 'Garage sửa xe Hoàn Kiếm',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garagehoankiem@example.com',
        password: 'password123',
        phone: '02438251234',
        address: '45 Hàng Bông, Hoàn Kiếm, Hà Nội',
        lat: 21.0285,
        lon: 105.8500,
        openHours: '7:00 - 21:00',
        description: 'Sửa chữa xe máy, ô tô',
        ratingAvg: 4.5,
        ratingCount: 180,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0022',
        stationName: 'Trạm sửa xe Đống Đa',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxedongda@example.com',
        password: 'password123',
        phone: '02438252345',
        address: '123 Tây Sơn, Đống Đa, Hà Nội',
        lat: 21.0100,
        lon: 105.8300,
        openHours: '6:00 - 22:00',
        description: 'Sửa chữa và bảo dưỡng xe',
        ratingAvg: 4.4,
        ratingCount: 160,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0023',
        stationName: 'Garage Cầu Giấy',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garagecaugiay@example.com',
        password: 'password123',
        phone: '02438253456',
        address: '78 Hoàng Quốc Việt, Cầu Giấy, Hà Nội',
        lat: 21.0450,
        lon: 105.8000,
        openHours: '7:00 - 20:00',
        description: 'Sửa chữa xe máy, ô tô chuyên nghiệp',
        ratingAvg: 4.6,
        ratingCount: 200,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0024',
        stationName: 'Trạm sửa xe Hai Bà Trưng',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxehaibatrung@example.com',
        password: 'password123',
        phone: '02438254567',
        address: '234 Bạch Mai, Hai Bà Trưng, Hà Nội',
        lat: 20.9950,
        lon: 105.8500,
        openHours: '6:00 - 21:00',
        description: 'Sửa chữa nhanh, uy tín',
        ratingAvg: 4.3,
        ratingCount: 140,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0025',
        stationName: 'Garage Ba Đình',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garagebadinh@example.com',
        password: 'password123',
        phone: '02438255678',
        address: '567 Đội Cấn, Ba Đình, Hà Nội',
        lat: 21.0350,
        lon: 105.8100,
        openHours: '7:00 - 19:00',
        description: 'Sửa chữa và bảo dưỡng',
        ratingAvg: 4.5,
        ratingCount: 170,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0026',
        stationName: 'Trạm sửa xe Thanh Xuân',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxethanhxuan@example.com',
        password: 'password123',
        phone: '02438256789',
        address: '89 Nguyễn Trãi, Thanh Xuân, Hà Nội',
        lat: 20.9900,
        lon: 105.8200,
        openHours: '6:00 - 22:00',
        description: 'Sửa chữa xe máy, ô tô',
        ratingAvg: 4.4,
        ratingCount: 150,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0027',
        stationName: 'Garage Long Biên',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garagelongbien@example.com',
        password: 'password123',
        phone: '02438257890',
        address: '123 Nguyễn Văn Cừ, Long Biên, Hà Nội',
        lat: 21.0400,
        lon: 105.8700,
        openHours: '7:00 - 20:00',
        description: 'Sửa chữa chuyên nghiệp',
        ratingAvg: 4.6,
        ratingCount: 190,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0028',
        stationName: 'Trạm sửa xe Tây Hồ',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxetayho@example.com',
        password: 'password123',
        phone: '02438258901',
        address: '456 Lạc Long Quân, Tây Hồ, Hà Nội',
        lat: 21.0600,
        lon: 105.8200,
        openHours: '6:00 - 21:00',
        description: 'Sửa chữa và bảo dưỡng',
        ratingAvg: 4.5,
        ratingCount: 165,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0029',
        stationName: 'Garage Hoàng Mai',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'garagehoangmai@example.com',
        password: 'password123',
        phone: '02438259012',
        address: '789 Giải Phóng, Hoàng Mai, Hà Nội',
        lat: 20.9800,
        lon: 105.8400,
        openHours: '7:00 - 19:00',
        description: 'Sửa chữa xe máy, ô tô',
        ratingAvg: 4.3,
        ratingCount: 130,
        createdAt: new Date().toISOString()
      },
      {
        id: 'S0030',
        stationName: 'Trạm sửa xe Nam Từ Liêm',
        type: 'rescue', // Đã gộp repair vào rescue
        email: 'suaxenamtuliem@example.com',
        password: 'password123',
        phone: '02438250123',
        address: '321 Phạm Hùng, Nam Từ Liêm, Hà Nội',
        lat: 21.0200,
        lon: 105.7800,
        openHours: '6:00 - 22:00',
        description: 'Sửa chữa nhanh, uy tín',
        ratingAvg: 4.7,
        ratingCount: 210,
        createdAt: new Date().toISOString()
      }
    ];
    
    // If file exists, merge with existing data (avoid duplicates)
    if (fs.existsSync(stationsPath) && existingStations.length > 0) {
      const existingIds = new Set(existingStations.map(s => s.id));
      const newStations = seedStations.filter(s => !existingIds.has(s.id));
      if (newStations.length > 0) {
        const mergedStations = [...existingStations, ...newStations];
        writeJson('stations.json', mergedStations);
        console.log(`Added ${newStations.length} new stations to stations.json. Total: ${mergedStations.length}`);
      } else {
        console.log(`All stations already exist in stations.json. Total: ${existingStations.length}`);
      }
    } else {
      writeJson('stations.json', seedStations);
      console.log(`Created stations.json with ${seedStations.length} seed stations`);
    }
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

  // Generate taikhoan.md file
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

