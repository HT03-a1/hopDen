import { Router, Request, Response } from 'express';
import { readJson, writeJson } from '../services/dataService';
import { Telemetry, Device, User, SOS } from '../types';
import { Server } from 'socket.io';
import { findNextStationForSOS } from './sos';

const router = Router();
const HARDWARE_PRIORITY_WINDOW_MS = 60 * 1000; // 60 giây ưu tiên dữ liệu phần cứng

// Hàm tính khoảng cách giữa 2 tọa độ (Haversine formula)
function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
  const R = 6371; // Earth radius in km
  const dLat = (lat2 - lat1) * Math.PI / 180;
  const dLon = (lon2 - lon1) * Math.PI / 180;
  const a = 
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
    Math.sin(dLon / 2) * Math.sin(dLon / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

// Hàm tìm trạm gần nhất
function findNearestStation(sosLat: number, sosLon: number, sosType: string): any | null {
  try {
    const stations = readJson<any>('stations.json');
    const requiredType = sosType === 'accident' || sosType === 'medical' ? 'medical' : 'rescue';
    
    // Tối ưu: Tính khoảng cách trong một lần duyệt
    let nearestStation: any | null = null;
    let minDistance = Infinity;
    
    for (const station of stations) {
      // Kiểm tra loại trạm và tọa độ hợp lệ
      if (station.type !== requiredType || !station.lat || !station.lon || isNaN(station.lat) || isNaN(station.lon)) {
        continue;
      }
      
      // Tính khoảng cách và so sánh ngay
      const distance = calculateDistance(sosLat, sosLon, station.lat, station.lon);
      if (distance < minDistance) {
        minDistance = distance;
        nearestStation = station;
      }
    }
    
    return nearestStation;
  } catch (error: any) {
    console.error('[TELEMETRY] Error finding nearest station:', error.message);
    return null;
  }
}

// Hàm tự động tạo SOS từ telemetry data
function createAutoSOS(
  userId: string,
  deviceId: string,
  lat: number,
  lon: number,
  eventType: string,
  severity: string,
  io?: Server
): SOS | null {
  try {
    // Kiểm tra xem đã có SOS active chưa (tránh tạo trùng)
    const sosList = readJson<SOS>('sos.json');
    const existingActiveSOS = sosList.find(sos => 
      sos.userId === userId &&
      sos.status !== 'done' &&
      sos.status !== 'cancelled' &&
      sos.type === 'accident' // Chỉ kiểm tra SOS tai nạn
    );
    
    if (existingActiveSOS) {
      console.log(`[TELEMETRY] ⚠️ User ${userId} đã có SOS active (${existingActiveSOS.id}), không tạo mới`);
      return existingActiveSOS;
    }
    
    // Map severity từ "HIGH" sang "high" (lowercase)
    const normalizedSeverity = severity.toLowerCase() as 'low' | 'medium' | 'high' | 'critical';
    // Map event_type "CRASH" sang type "accident"
    const sosType: 'accident' | 'breakdown' | 'medical' | 'other' = 'accident';
    
    const newSOS: SOS = {
      id: `SOS${Date.now()}`,
      userId,
      deviceId,
      type: sosType,
      severity: normalizedSeverity === 'high' ? 'high' : normalizedSeverity === 'critical' ? 'critical' : 'high',
      location: {
        lat,
        lon
      },
      status: 'pending',
      note: `Tự động tạo từ phần cứng: ${eventType} - ${severity}`,
      rejectedStationIds: [],
      readyStationIds: [],
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString()
    };
    
    sosList.push(newSOS);
    writeJson('sos.json', sosList);
    console.log(`[TELEMETRY] 🆘 Đã tự động tạo SOS: ${newSOS.id} cho user ${userId}`);
    
    // Tự động tìm và gán trạm - ƯU TIÊN các trạm đã bấm "Sẵn sàng hỗ trợ" (nếu có)
    try {
      // Dùng findNextStationForSOS để đảm bảo ưu tiên ready stations (nếu có)
      // Khi SOS mới tạo từ phần cứng, thường chưa có trạm nào bấm "Sẵn sàng", 
      // nhưng vẫn dùng hàm này để nhất quán và hỗ trợ trường hợp có trạm đã bấm sẵn sàng trước đó
      const nearestStation = findNextStationForSOS(newSOS);
      
      if (nearestStation) {
        const sosIndex = sosList.findIndex(s => s.id === newSOS.id);
        if (sosIndex !== -1) {
          sosList[sosIndex].assignedStationId = nearestStation.id;
          sosList[sosIndex].status = 'pending';
          
          // QUAN TRỌNG: Set assignment deadline để trạm có đếm ngược và nút "Nhận nhiệm vụ"
          const now = Date.now();
          const ASSIGNMENT_TIMEOUT_MS = 2 * 60 * 1000; // 2 phút
          sosList[sosIndex].assignedAt = new Date(now).toISOString();
          sosList[sosIndex].assignmentExpiresAt = new Date(now + ASSIGNMENT_TIMEOUT_MS).toISOString();
          
          sosList[sosIndex].updatedAt = new Date().toISOString();
          writeJson('sos.json', sosList);
          
          // Cập nhật newSOS để broadcast
          newSOS.assignedStationId = nearestStation.id;
          newSOS.status = 'pending';
          newSOS.assignedAt = sosList[sosIndex].assignedAt;
          newSOS.assignmentExpiresAt = sosList[sosIndex].assignmentExpiresAt;
          newSOS.updatedAt = sosList[sosIndex].updatedAt;
          
          const isReadyStation = newSOS.readyStationIds?.includes(nearestStation.id);
          console.log(`[TELEMETRY] 🏥 SOS ${newSOS.id} đã được gán cho trạm ${nearestStation.id} (${nearestStation.stationName}) với deadline ${sosList[sosIndex].assignmentExpiresAt}`);
          console.log(`[TELEMETRY] ${isReadyStation ? '✅ Trạm này đã bấm "Sẵn sàng" - được ưu tiên!' : 'ℹ️ Trạm này chưa bấm "Sẵn sàng"'}`);
          console.log(`[TELEMETRY] ℹ️ Khi trạm này không nhận, hệ thống sẽ ưu tiên các trạm đã bấm "Sẵn sàng hỗ trợ"`);
        }
      } else {
        console.log(`[TELEMETRY] ⚠️ Không tìm thấy trạm phù hợp cho SOS ${newSOS.id}`);
      }
    } catch (autoAssignError: any) {
      console.error('[TELEMETRY] Error auto-assigning station:', autoAssignError.message);
    }
    
    // Broadcast SOS mới qua WebSocket
    if (io) {
      try {
        io.emit('sos:new', newSOS);
        io.emit('sos:update', newSOS);
        console.log(`[TELEMETRY] 📡 Đã broadcast SOS ${newSOS.id} qua WebSocket`);
      } catch (socketError: any) {
        console.error('[TELEMETRY] Error broadcasting SOS:', socketError.message);
      }
    }
    
    return newSOS;
  } catch (error: any) {
    console.error('[TELEMETRY] ❌ Lỗi khi tạo SOS tự động:', error);
    return null;
  }
}

// Create telemetry data
router.post('/', (req: Request, res: Response) => {
  const clientIp = req.ip || req.socket.remoteAddress || 'unknown';
  const timestamp = new Date().toISOString();

  // LOG CHI TIẾT DỮ LIỆU TỪ PHẦN CỨNG
  console.log('\n========================================');
  console.log('[TELEMETRY] 📡 NHẬN DỮ LIỆU TỪ PHẦN CỨNG');
  console.log('========================================');
  console.log(`⏰ Thời gian: ${timestamp}`);
  console.log(`🌐 IP nguồn: ${clientIp}`);
  console.log(`📦 Full Request Body:`, JSON.stringify(req.body, null, 2));
  console.log(`📋 Headers:`, JSON.stringify(req.headers, null, 2));
  console.log('========================================\n');

  // Hỗ trợ cả 2 format: format mới từ phần cứng và format cũ
  let userId: string | undefined;
  let deviceId: string | undefined;
  let lat: number | undefined;
  let lon: number | undefined;
  let speed: number | undefined;
  let source: string = 'hardware';
  let eventType: string | undefined;
  let severity: string | undefined;

  // Format mới từ phần cứng: device_id, location: { latitude, longitude }
  if (req.body.device_id) {
    userId = req.body.device_id; // device_id chính là userId
    deviceId = `DHW_${req.body.device_id}`; // Tạo deviceId từ userId
    
    // Đọc location từ nested object
    if (req.body.location) {
      lat = req.body.location.latitude;
      lon = req.body.location.longitude;
    }
    
    // Đọc các trường khác
    eventType = req.body.event_type;
    severity = req.body.severity;
    
    console.log(`[TELEMETRY] 📥 Format từ phần cứng:`);
    console.log(`  device_id (userId): ${userId}`);
    console.log(`  deviceId: ${deviceId}`);
    console.log(`  location.latitude: ${lat}`);
    console.log(`  location.longitude: ${lon}`);
    console.log(`  event_type: ${eventType || 'N/A'}`);
    console.log(`  severity: ${severity || 'N/A'}`);
    console.log(`  gmail: ${req.body.gmail || 'N/A'}`);
    console.log(`  timestamp: ${req.body.timestamp || 'N/A'}`);
  } 
  // Format cũ: deviceId, userId, lat, lon trực tiếp
  else {
    userId = req.body.userId;
    deviceId = req.body.deviceId;
    lat = req.body.lat;
    lon = req.body.lon;
    speed = req.body.speed;
    source = req.body.source || 'hardware';
    
    console.log(`[TELEMETRY] 📥 Format cũ (mobile/web):`);
    console.log(`  deviceId: ${deviceId}`);
    console.log(`  userId: ${userId}`);
    console.log(`  lat: ${lat}`);
    console.log(`  lon: ${lon}`);
    console.log(`  speed: ${speed || 'N/A'}`);
  }

  if (!userId || lat === undefined || lon === undefined) {
    console.error('[TELEMETRY] ❌ Thiếu thông tin bắt buộc:', { 
      userId, 
      lat, 
      lon,
      body: req.body 
    });
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc: cần device_id (hoặc userId) và location (hoặc lat/lon)' });
  }

  try {
    // Parse lat/lon thành number (ESP32 có thể gửi string)
    const latNum = parseFloat(String(lat));
    const lonNum = parseFloat(String(lon));

    // Validate coordinates
    if (isNaN(latNum) || isNaN(lonNum)) {
      console.error('[TELEMETRY] ❌ Tọa độ không hợp lệ:', { lat, lon, latNum, lonNum });
      return res.status(400).json({ error: 'Tọa độ không hợp lệ (phải là số)' });
    }

    if (latNum < -90 || latNum > 90 || lonNum < -180 || lonNum > 180) {
      console.error('[TELEMETRY] ❌ Tọa độ nằm ngoài phạm vi:', { latNum, lonNum });
      return res.status(400).json({ error: 'Tọa độ nằm ngoài phạm vi hợp lệ' });
    }

    const telemetryList = readJson<Telemetry>('telemetry.json');
    const users = readJson<User>('users.json');
    const now = new Date();
    const nowIso = now.toISOString();
    const normalizedSource: 'hardware' | 'mobile' = source === 'mobile' ? 'mobile' : 'hardware';
    let updatedUser: User | null = null;

    // Đảm bảo deviceId có giá trị
    if (!deviceId) {
      deviceId = `DHW_${userId}`;
    }

    // Log dữ liệu đã parse
    console.log('[TELEMETRY] ✅ Dữ liệu đã parse:', { 
      deviceId, 
      userId, 
      lat: latNum, 
      lon: lonNum, 
      speed: speed || 'N/A',
      source: normalizedSource,
      eventType: eventType || 'N/A',
      severity: severity || 'N/A'
    });

    // XÁC THỰC TÀI KHOẢN NẾU LÀ DỮ LIỆU TỪ PHẦN CỨNG
    if (req.body.device_id && req.body.gmail && req.body.pass) {
      console.log('[TELEMETRY] 🔐 Bắt đầu xác thực tài khoản từ phần cứng...');
      
      const user = users.find(u => u && u.id === userId);
      
      if (!user) {
        console.error(`[TELEMETRY] ❌ Xác thực thất bại: Không tìm thấy user với ID ${userId}`);
        return res.status(401).json({ 
          error: 'Xác thực thất bại: Không tìm thấy tài khoản với ID này',
          code: 'USER_NOT_FOUND'
        });
      }
      
      // Kiểm tra email (gmail)
      const emailMatch = user.email.toLowerCase() === req.body.gmail.toLowerCase();
      if (!emailMatch) {
        console.error(`[TELEMETRY] ❌ Xác thực thất bại: Email không khớp`);
        console.error(`  Email từ phần cứng: ${req.body.gmail}`);
        console.error(`  Email trong hệ thống: ${user.email}`);
        return res.status(401).json({ 
          error: 'Xác thực thất bại: Email không khớp',
          code: 'EMAIL_MISMATCH'
        });
      }
      
      // Kiểm tra password (pass) - so sánh case-sensitive
      const passwordMatch = user.password === req.body.pass;
      if (!passwordMatch) {
        console.error(`[TELEMETRY] ❌ Xác thực thất bại: Mật khẩu không khớp`);
        console.error(`  Mật khẩu từ phần cứng: ${req.body.pass}`);
        console.error(`  Mật khẩu trong hệ thống: ${user.password}`);
        return res.status(401).json({ 
          error: 'Xác thực thất bại: Mật khẩu không khớp',
          code: 'PASSWORD_MISMATCH'
        });
      }
      
      console.log(`[TELEMETRY] ✅ Xác thực thành công cho user ${userId} (${user.name})`);
      console.log(`  Email: ${user.email}`);
      console.log(`  Device ID: ${deviceId}`);
    }

    if (Array.isArray(users)) {
      const userIndex = users.findIndex(user => user && user.id === userId);
      
      if (userIndex !== -1) {
        const user = users[userIndex];
        const oldLat = user.lat;
        const oldLon = user.lon;

        if (normalizedSource === 'hardware') {
          users[userIndex].lat = latNum;
          users[userIndex].lon = lonNum;
          users[userIndex].lastLocationSource = 'hardware';
          users[userIndex].lastLocationUpdatedAt = nowIso;
          users[userIndex].lastHardwareLocationAt = nowIso;
          users[userIndex].isOnline = true; // Đánh dấu online khi nhận dữ liệu từ phần cứng
          // Ghi ngay để đảm bảo isOnline được lưu
          writeJson('users.json', users);
        } else {
          users[userIndex].lastMobileLocationAt = nowIso;

          const lastHardwareIso = user.lastHardwareLocationAt;
          let hardwareIsFresh = false;
          if (lastHardwareIso) {
            const lastHardwareTime = Date.parse(lastHardwareIso);
            if (!Number.isNaN(lastHardwareTime)) {
              hardwareIsFresh = (now.getTime() - lastHardwareTime) < HARDWARE_PRIORITY_WINDOW_MS;
            }
          }

          if (!hardwareIsFresh) {
            users[userIndex].lat = latNum;
            users[userIndex].lon = lonNum;
            users[userIndex].lastLocationSource = 'mobile';
            users[userIndex].lastLocationUpdatedAt = nowIso;
          }
        }

        updatedUser = users[userIndex];
        writeJson('users.json', users);
        // Log cập nhật vị trí user
        console.log(`[TELEMETRY] ✅ Đã cập nhật vị trí user ${userId}:`, {
          old: { lat: oldLat, lon: oldLon },
          new: { lat: updatedUser.lat, lon: updatedUser.lon },
          source: updatedUser.lastLocationSource
        });
      } else {
        console.warn(`[TELEMETRY] ⚠️ Không tìm thấy user ${userId} để cập nhật vị trí`);
        console.warn(`[TELEMETRY] Danh sách users hiện có:`, users.map(u => u?.id).filter(Boolean));
      }
    }
    
    const newTelemetry: Telemetry = {
      id: `T${Date.now()}`,
      deviceId: deviceId || `DHW_${userId}`,
      userId,
      lat: latNum,
      lon: lonNum,
      speed: speed || undefined,
      source: normalizedSource,
      timestamp: nowIso
    };
    
    // Tự động tạo SOS nếu phát hiện CRASH với severity HIGH hoặc CRITICAL
    if (eventType === 'CRASH' && severity && (severity.toUpperCase() === 'HIGH' || severity.toUpperCase() === 'CRITICAL')) {
      console.log(`[TELEMETRY] ⚠️ Phát hiện sự kiện CRASH từ phần cứng!`);
      console.log(`[TELEMETRY] 🆘 Tự động tạo SOS...`);
      
      const io = req.app.get('io');
      const autoSOS = createAutoSOS(
        userId,
        deviceId || `DHW_${userId}`,
        latNum,
        lonNum,
        eventType,
        severity,
        io
      );
      
      if (autoSOS) {
        console.log(`[TELEMETRY] ✅ Đã tự động tạo SOS ${autoSOS.id} thành công!`);
      } else {
        console.error(`[TELEMETRY] ❌ Không thể tạo SOS tự động`);
      }
    }
    
    // Tự động tắt SOS nếu phát hiện NORMAL (tình huống đã ổn định)
    if (eventType === 'NORMAL') {
      console.log(`[TELEMETRY] ✅ Phát hiện sự kiện NORMAL từ phần cứng!`);
      console.log(`[TELEMETRY] 🔄 Tự động tắt SOS active (nếu có)...`);
      
      try {
        const sosList = readJson<SOS>('sos.json');
        const activeSOS = sosList.find(sos => 
          sos.userId === userId &&
          sos.status !== 'done' &&
          sos.status !== 'cancelled' &&
          sos.type === 'accident' // Chỉ tắt SOS tai nạn
        );
        
        if (activeSOS) {
          const sosIndex = sosList.findIndex(s => s.id === activeSOS.id);
          if (sosIndex !== -1) {
            sosList[sosIndex].status = 'cancelled';
            sosList[sosIndex].updatedAt = new Date().toISOString();
            sosList[sosIndex].note = (sosList[sosIndex].note || '') + ` | Tự động tắt bởi phần cứng: NORMAL event`;
            writeJson('sos.json', sosList);
            
            console.log(`[TELEMETRY] ✅ Đã tự động tắt SOS ${activeSOS.id} (status = cancelled)`);
            
            // Broadcast update qua WebSocket
            const io = req.app.get('io');
            if (io) {
              io.emit('sos:update', sosList[sosIndex]);
              console.log(`[TELEMETRY] 📡 Đã broadcast SOS cancelled qua WebSocket`);
            }
          }
        } else {
          console.log(`[TELEMETRY] ℹ️ Không có SOS active để tắt`);
        }
      } catch (error: any) {
        console.error(`[TELEMETRY] ❌ Lỗi khi tắt SOS tự động:`, error.message);
      }
    }

    telemetryList.push(newTelemetry);
    writeJson('telemetry.json', telemetryList);
    console.log(`[TELEMETRY] 💾 Đã lưu telemetry: ${newTelemetry.id}`);

    // Update device location
    const devices = readJson<Device>('devices.json');
    const deviceIndex = devices.findIndex(d => d.id === deviceId);
    
    if (deviceIndex !== -1) {
      devices[deviceIndex].lat = latNum;
      devices[deviceIndex].lon = lonNum;
      devices[deviceIndex].lastUpdate = new Date().toISOString();
    } else {
      // Create new device if not exists
      devices.push({
        id: deviceId,
        userId,
        deviceName: `Thiết bị ${deviceId}`,
        vehicleType: 'unknown',
        lat: latNum,
        lon: lonNum,
        status: 'active',
        lastUpdate: new Date().toISOString()
      });
    }

    writeJson('devices.json', devices);
    
    if (deviceIndex !== -1) {
      console.log(`[TELEMETRY] 🔄 Đã cập nhật device ${deviceId}`);
    } else {
      console.log(`[TELEMETRY] ➕ Đã tạo device mới: ${deviceId}`);
    }
    
    // Cập nhật trạng thái online cho tất cả users (đánh dấu offline nếu quá 2 phút không có dữ liệu)
    // Lưu ý: User hiện tại đã được set isOnline = true ở trên, nên không cần cập nhật lại
    try {
      const allUsers = readJson<User>('users.json');
      const OFFLINE_THRESHOLD_MS = 2 * 60 * 1000; // 2 phút
      const nowTime = now.getTime();
      
      allUsers.forEach((user, index) => {
        // Bỏ qua user hiện tại (đã được set isOnline = true ở trên)
        if (user.id === userId && normalizedSource === 'hardware') {
          return; // Giữ nguyên isOnline = true đã set
        }
        
        if (user.lastHardwareLocationAt) {
          const lastHardwareTime = Date.parse(user.lastHardwareLocationAt);
          if (!Number.isNaN(lastHardwareTime)) {
            const timeDiff = nowTime - lastHardwareTime;
            allUsers[index].isOnline = timeDiff < OFFLINE_THRESHOLD_MS;
          } else {
            allUsers[index].isOnline = false;
          }
        } else {
          // Không có lastHardwareLocationAt → Offline (chưa có phần cứng)
          allUsers[index].isOnline = false;
        }
      });
      
      writeJson('users.json', allUsers);
    } catch (error: any) {
      console.error('[TELEMETRY] Lỗi khi cập nhật trạng thái online:', error.message);
    }

    if (updatedUser) {
      const io = req.app.get('io');
      if (io) {
        const updateData = {
          id: updatedUser.id,
          lat: updatedUser.lat,
          lon: updatedUser.lon,
          lastLocationSource: updatedUser.lastLocationSource,
          lastLocationUpdatedAt: updatedUser.lastLocationUpdatedAt,
          lastHardwareLocationAt: updatedUser.lastHardwareLocationAt,
          isOnline: updatedUser.isOnline
        };
        console.log(`[TELEMETRY] 📡 Emitting user:update WebSocket event:`, updateData);
        io.emit('user:update', updateData);
      } else {
        console.warn('[TELEMETRY] ⚠️ Socket.IO instance không có sẵn, không thể emit user:update');
      }
    } else {
      console.warn('[TELEMETRY] ⚠️ Không có updatedUser để emit WebSocket event');
    }

    console.log(`[TELEMETRY] ✅ Hoàn thành xử lý telemetry ${newTelemetry.id}\n`);
    res.status(201).json(newTelemetry);
  } catch (error) {
    console.error('[TELEMETRY] ❌ Lỗi xử lý telemetry:', error);
    if (error instanceof Error) {
      console.error('[TELEMETRY] ❌ Stack trace:', error.stack);
    }
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get telemetry by device
router.get('/', (req: Request, res: Response) => {
  const { deviceId, userId } = req.query;

  try {
    let telemetryList = readJson<Telemetry>('telemetry.json');

    if (deviceId) {
      telemetryList = telemetryList.filter(t => t.deviceId === deviceId);
    }

    if (userId) {
      telemetryList = telemetryList.filter(t => t.userId === userId);
    }

    res.json(telemetryList);
  } catch (error) {
    console.error('Get telemetry error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

