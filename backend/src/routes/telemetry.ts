import { Router, Request, Response } from 'express';
import { readJson, writeJson } from '../services/dataService';
import { Telemetry, Device } from '../types';

const router = Router();

// Create telemetry data
router.post('/', (req: Request, res: Response) => {
  const { deviceId, userId, lat, lon, speed } = req.body;

  if (!deviceId || !userId || lat === undefined || lon === undefined) {
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc' });
  }

  try {
    const telemetryList = readJson<Telemetry>('telemetry.json');
    
    const newTelemetry: Telemetry = {
      id: `T${Date.now()}`,
      deviceId,
      userId,
      lat,
      lon,
      speed,
      timestamp: new Date().toISOString()
    };

    telemetryList.push(newTelemetry);
    writeJson('telemetry.json', telemetryList);

    // Update device location
    const devices = readJson<Device>('devices.json');
    const deviceIndex = devices.findIndex(d => d.id === deviceId);
    
    if (deviceIndex !== -1) {
      devices[deviceIndex].lat = lat;
      devices[deviceIndex].lon = lon;
      devices[deviceIndex].lastUpdate = new Date().toISOString();
    } else {
      // Create new device if not exists
      devices.push({
        id: deviceId,
        userId,
        deviceName: `Thiết bị ${deviceId}`,
        vehicleType: 'unknown',
        lat,
        lon,
        status: 'active',
        lastUpdate: new Date().toISOString()
      });
    }

    writeJson('devices.json', devices);

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

