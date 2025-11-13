import { Router, Request, Response } from 'express';
import { readJson, writeJson } from '../services/dataService';
import { Telemetry, Device, User } from '../types';

const router = Router();
const HARDWARE_PRIORITY_WINDOW_MS = 60 * 1000; // 60 giây ưu tiên dữ liệu phần cứng

// Create telemetry data
router.post('/', (req: Request, res: Response) => {
  const { deviceId, userId, lat, lon, speed, source } = req.body;

  if (!deviceId || !userId || lat === undefined || lon === undefined) {
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc' });
  }

  try {
    // Parse lat/lon thành number (ESP32 có thể gửi string)
    const latNum = parseFloat(lat);
    const lonNum = parseFloat(lon);

    // Validate coordinates
    if (isNaN(latNum) || isNaN(lonNum)) {
      return res.status(400).json({ error: 'Tọa độ không hợp lệ (phải là số)' });
    }

    if (latNum < -90 || latNum > 90 || lonNum < -180 || lonNum > 180) {
      return res.status(400).json({ error: 'Tọa độ nằm ngoài phạm vi hợp lệ' });
    }

    const telemetryList = readJson<Telemetry>('telemetry.json');
    const users = readJson<User>('users.json');
    const now = new Date();
    const nowIso = now.toISOString();
    const normalizedSource: 'hardware' | 'mobile' = source === 'mobile' ? 'mobile' : 'hardware';
    let updatedUser: User | null = null;

    // Reduced logging - only log important info
    if (process.env.NODE_ENV !== 'production') {
      console.log('[TELEMETRY] Nhận dữ liệu:', { deviceId, userId, lat: latNum, lon: lonNum, source: normalizedSource });
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
        // Only log in development
        if (process.env.NODE_ENV !== 'production') {
          console.log(`[TELEMETRY] ✅ Đã cập nhật vị trí user ${userId}:`, {
            old: { lat: oldLat, lon: oldLon },
            new: { lat: updatedUser.lat, lon: updatedUser.lon },
            source: updatedUser.lastLocationSource
          });
        }
      } else {
        console.warn(`[TELEMETRY] ⚠️ Không tìm thấy user ${userId} để cập nhật vị trí`);
        console.warn(`[TELEMETRY] Danh sách users hiện có:`, users.map(u => u?.id).filter(Boolean));
      }
    }
    
    const newTelemetry: Telemetry = {
      id: `T${Date.now()}`,
      deviceId,
      userId,
      lat: latNum,
      lon: lonNum,
      speed,
      source: normalizedSource,
      timestamp: nowIso
    };

    telemetryList.push(newTelemetry);
    writeJson('telemetry.json', telemetryList);

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

    if (updatedUser) {
      const io = req.app.get('io');
      if (io) {
        const updateData = {
          id: updatedUser.id,
          lat: updatedUser.lat,
          lon: updatedUser.lon,
          lastLocationSource: updatedUser.lastLocationSource,
          lastLocationUpdatedAt: updatedUser.lastLocationUpdatedAt
        };
        // Only log in development
        if (process.env.NODE_ENV !== 'production') {
          console.log(`[TELEMETRY] 📡 Emitting user:update WebSocket event`);
        }
        io.emit('user:update', updateData);
      } else {
        console.warn('[TELEMETRY] ⚠️ Socket.IO instance không có sẵn, không thể emit user:update');
      }
    } else {
      console.warn('[TELEMETRY] ⚠️ Không có updatedUser để emit WebSocket event');
    }

    res.status(201).json(newTelemetry);
  } catch (error) {
    console.error('Create telemetry error:', error);
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

