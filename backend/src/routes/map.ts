import { Router, Request, Response } from 'express';
import { readJson } from '../services/dataService';
import { User, Station, Device, SOS, MapEntity } from '../types';

const router = Router();

// Get all map entities
router.get('/entities', (req: Request, res: Response) => {
  try {
    const entities: MapEntity[] = [];

    // Get users
    try {
      const users = readJson<User>('users.json');
      if (Array.isArray(users)) {
        users.forEach(user => {
          if (user && user.id && user.lat && user.lon) {
            entities.push({
              type: 'user',
              id: user.id,
              name: user.name || '',
              lat: user.lat,
              lon: user.lon,
              email: user.email,
              phone: user.phone,
              address: user.address
            });
          }
        });
      }
    } catch (error: any) {
      console.error('Error loading users:', error.message);
    }

    // Get stations
    try {
      const stations = readJson<Station>('stations.json');
      if (Array.isArray(stations)) {
        stations.forEach(station => {
          if (!station || !station.id || !station.stationName) {
            return;
          }
          if (!station.lat || !station.lon) {
            console.warn(`Station ${station.id} (${station.stationName}) missing coordinates`);
            return;
          }
          entities.push({
            type: station.type === 'medical' ? 'medical_station' : 'rescue_station', // Đã gộp repair vào rescue
            id: station.id,
            name: station.stationName,
            lat: station.lat,
            lon: station.lon,
            email: station.email,
            phone: station.phone,
            address: station.address,
            openHours: station.openHours,
            description: station.description,
            ratingAvg: station.ratingAvg,
            ratingCount: station.ratingCount,
            stationType: station.type
          });
        });
      }
    } catch (error: any) {
      console.error('Error loading stations:', error.message);
    }

    // Get active SOS
    try {
      const sosList = readJson<SOS>('sos.json');
      const users = readJson<User>('users.json');
      
      // Tạo map user để tra cứu nhanh
      const userMap = new Map();
      if (Array.isArray(users)) {
        users.forEach(user => {
          if (user && user.id) {
            userMap.set(user.id, user);
          }
        });
      }
      
      if (Array.isArray(sosList)) {
        sosList
          .filter(sos => sos && sos.status && sos.status !== 'done' && sos.status !== 'cancelled' && sos.location)
          .forEach(sos => {
            if (sos.location && sos.location.lat && sos.location.lon) {
              // Lấy thông tin user từ userMap
              const user = userMap.get(sos.userId);
              
              entities.push({
                type: 'sos',
                id: sos.id,
                name: `SOS - ${sos.type}`,
                lat: sos.location.lat,
                lon: sos.location.lon,
                sosType: sos.type,
                severity: sos.severity,
                status: sos.status,
                userId: sos.userId,
                userName: user?.name || 'Không xác định',
                userPhone: user?.phone || 'N/A',
                userEmail: user?.email || 'N/A',
                createdAt: sos.createdAt,
                note: sos.note || ''
              });
            }
          });
      }
    } catch (error: any) {
      console.error('Error loading SOS:', error.message);
    }

    // Get devices (optional)
    try {
      const devices = readJson<Device>('devices.json');
      if (Array.isArray(devices)) {
        devices.forEach(device => {
          if (device && device.id && device.lat && device.lon) {
            entities.push({
              type: 'device',
              id: device.id,
              name: device.deviceName || '',
              lat: device.lat,
              lon: device.lon,
              userId: device.userId,
              status: device.status
            });
          }
        });
      }
    } catch (error: any) {
      console.error('Error loading devices:', error.message);
    }

    res.json(entities);
  } catch (error: any) {
    console.error('Get map entities error:', error);
    console.error('Error stack:', error.stack);
    res.status(500).json({ error: 'Lỗi server: ' + (error.message || 'Unknown error') });
  }
});

// Get routes using OSRM (Open Source Routing Machine) - route thực tế theo đường bộ
router.get('/routes', async (req: Request, res: Response) => {
  try {
    const { fromLat, fromLon, toLat, toLon } = req.query;

    if (!fromLat || !fromLon || !toLat || !toLon) {
      return res.status(400).json({ error: 'Thiếu thông tin tọa độ' });
    }

    const fromLatNum = parseFloat(fromLat as string);
    const fromLonNum = parseFloat(fromLon as string);
    const toLatNum = parseFloat(toLat as string);
    const toLonNum = parseFloat(toLon as string);

    // Validate coordinates
    if (isNaN(fromLatNum) || isNaN(fromLonNum) || isNaN(toLatNum) || isNaN(toLonNum)) {
      return res.status(400).json({ error: 'Tọa độ không hợp lệ' });
    }

    // Sử dụng OSRM public server để lấy route thực tế theo đường bộ
    // Format: lon,lat (OSRM dùng lon trước, lat sau)
    const osrmUrl = `https://router.project-osrm.org/route/v1/driving/${fromLonNum},${fromLatNum};${toLonNum},${toLatNum}?overview=full&geometries=geojson`;
    
    try {
      // Fetch với timeout 10 giây
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);
      
      const response = await fetch(osrmUrl, {
        signal: controller.signal,
        headers: {
          'Accept': 'application/json'
        }
      });
      
      clearTimeout(timeoutId);

      if (!response.ok) {
        throw new Error(`OSRM API error: ${response.status}`);
      }

      const data = await response.json() as {
        code?: string;
        routes?: Array<{
          geometry?: {
            coordinates?: [number, number][];
          };
        }>;
      };

      if (data.code !== 'Ok' || !data.routes || data.routes.length === 0) {
        // Fallback: tạo route đơn giản nếu OSRM không có route
        console.warn('OSRM không tìm thấy route, sử dụng route đơn giản');
        return createSimpleRoute(fromLatNum, fromLonNum, toLatNum, toLonNum, res);
      }

      // Lấy geometry từ OSRM response
      const geometry = data.routes[0]?.geometry?.coordinates;
      if (!geometry || geometry.length === 0) {
        throw new Error('OSRM route không có geometry');
      }

      // Convert từ [lon, lat] sang {lat, lon}
      const points = geometry.map((coord: [number, number]) => ({
        lat: coord[1], // OSRM trả về [lon, lat]
        lon: coord[0]
      }));

      res.json({ polyline: points });
    } catch (error: any) {
      console.error('Error fetching route from OSRM:', error.message || error);
      // Fallback: tạo route đơn giản
      return createSimpleRoute(fromLatNum, fromLonNum, toLatNum, toLonNum, res);
    }
  } catch (error) {
    console.error('Error in route endpoint:', error);
    res.status(500).json({ error: 'Lỗi khi tính toán route' });
  }
});

// Hàm tạo route đơn giản khi không có OSRM (fallback)
function createSimpleRoute(
  fromLat: number,
  fromLon: number,
  toLat: number,
  toLon: number,
  res: Response
) {
  const points: { lat: number; lon: number }[] = [];
  
  // Tạo route đơn giản với đường thẳng có nhiều điểm để mượt
  const numPoints = 50;
  const latDiff = toLat - fromLat;
  const lonDiff = toLon - fromLon;
  
  for (let i = 0; i <= numPoints; i++) {
    const ratio = i / numPoints;
    points.push({
      lat: fromLat + latDiff * ratio,
      lon: fromLon + lonDiff * ratio
    });
  }
  
  res.json({ polyline: points });
}

export default router;

