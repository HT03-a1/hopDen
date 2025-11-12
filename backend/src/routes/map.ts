import { Router, Request, Response } from 'express';
import { readJson } from '../services/dataService';
import { User, Station, Device, SOS, MapEntity } from '../types';

const router = Router();

// Get all map entities
router.get('/entities', (req: Request, res: Response) => {
  try {
    const entities: MapEntity[] = [];

    // Get users
    const users = readJson<User>('users.json');
    users.forEach(user => {
      entities.push({
        type: 'user',
        id: user.id,
        name: user.name,
        lat: user.lat,
        lon: user.lon,
        email: user.email,
        phone: user.phone,
        address: user.address
      });
    });

    // Get stations
    const stations = readJson<Station>('stations.json');
    console.log(`Loading ${stations.length} stations for map`);
    stations.forEach(station => {
      if (!station.lat || !station.lon) {
        console.warn(`Station ${station.id} (${station.stationName}) missing coordinates`);
        return;
      }
      entities.push({
        type: station.type === 'medical' ? 'medical_station' : 
              station.type === 'rescue' ? 'rescue_station' : 'repair_station',
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
    console.log(`Added ${entities.filter(e => e.type.includes('station')).length} stations to map entities`);

    // Get active SOS
    const sosList = readJson<SOS>('sos.json');
    sosList
      .filter(sos => sos.status !== 'done' && sos.status !== 'cancelled')
      .forEach(sos => {
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
          createdAt: sos.createdAt
        });
      });

    // Get devices (optional)
    const devices = readJson<Device>('devices.json');
    devices.forEach(device => {
      entities.push({
        type: 'device',
        id: device.id,
        name: device.deviceName,
        lat: device.lat,
        lon: device.lon,
        userId: device.userId,
        status: device.status
      });
    });

    res.json(entities);
  } catch (error) {
    console.error('Get map entities error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get routes (mock polyline with street-like path)
router.get('/routes', (req: Request, res: Response) => {
  const { fromLat, fromLon, toLat, toLon } = req.query;

  if (!fromLat || !fromLon || !toLat || !toLon) {
    return res.status(400).json({ error: 'Thiếu thông tin tọa độ' });
  }

  const fromLatNum = parseFloat(fromLat as string);
  const fromLonNum = parseFloat(fromLon as string);
  const toLatNum = parseFloat(toLat as string);
  const toLonNum = parseFloat(toLon as string);

  // Calculate distance and direction
  const latDiff = toLatNum - fromLatNum;
  const lonDiff = toLonNum - fromLonNum;
  const absLatDiff = Math.abs(latDiff);
  const absLonDiff = Math.abs(lonDiff);
  const totalDistance = Math.sqrt(latDiff * latDiff + lonDiff * lonDiff);
  
  // Generate street-like route with multiple segments following street grid
  const points: { lat: number; lon: number }[] = [];
  
  // Start point
  points.push({ lat: fromLatNum, lon: fromLonNum });
  
  // Create route with 4-5 segments to simulate real street navigation
  // Each segment follows either horizontal (lat constant) or vertical (lon constant) direction
  
  if (absLonDiff > absLatDiff) {
    // Horizontal distance is larger - create route with horizontal segments
    // Pattern: H -> V -> H -> V -> H (Horizontal, Vertical, Horizontal, Vertical, Horizontal)
    
    const numSegments = 5;
    const segmentLength = 1 / numSegments;
    
    let currentLat = fromLatNum;
    let currentLon = fromLonNum;
    
    for (let seg = 0; seg < numSegments; seg++) {
      const segStart = seg * segmentLength;
      const segEnd = (seg + 1) * segmentLength;
      
      if (seg % 2 === 0) {
        // Horizontal segment (keep lat, change lon)
        const startLon = fromLonNum + lonDiff * segStart;
        const endLon = fromLonNum + lonDiff * segEnd;
        const steps = 30; // More points for smoother line
        
        for (let i = 1; i <= steps; i++) {
          const ratio = i / steps;
          const lon = startLon + (endLon - startLon) * ratio;
          // Keep lat constant (horizontal street)
          points.push({
            lat: currentLat,
            lon: lon
          });
        }
        currentLon = endLon;
      } else {
        // Vertical segment (change lat, keep lon)
        const startLat = fromLatNum + latDiff * segStart;
        const endLat = fromLatNum + latDiff * segEnd;
        const steps = 30;
        
        for (let i = 1; i <= steps; i++) {
          const ratio = i / steps;
          const lat = startLat + (endLat - startLat) * ratio;
          // Keep lon constant (vertical street)
          points.push({
            lat: lat,
            lon: currentLon
          });
        }
        currentLat = endLat;
      }
    }
  } else {
    // Vertical distance is larger - create route with vertical segments first
    // Pattern: V -> H -> V -> H -> V (Vertical, Horizontal, Vertical, Horizontal, Vertical)
    
    const numSegments = 5;
    const segmentLength = 1 / numSegments;
    
    let currentLat = fromLatNum;
    let currentLon = fromLonNum;
    
    for (let seg = 0; seg < numSegments; seg++) {
      const segStart = seg * segmentLength;
      const segEnd = (seg + 1) * segmentLength;
      
      if (seg % 2 === 0) {
        // Vertical segment (change lat, keep lon)
        const startLat = fromLatNum + latDiff * segStart;
        const endLat = fromLatNum + latDiff * segEnd;
        const steps = 30;
        
        for (let i = 1; i <= steps; i++) {
          const ratio = i / steps;
          const lat = startLat + (endLat - startLat) * ratio;
          // Keep lon constant (vertical street)
          points.push({
            lat: lat,
            lon: currentLon
          });
        }
        currentLat = endLat;
      } else {
        // Horizontal segment (keep lat, change lon)
        const startLon = fromLonNum + lonDiff * segStart;
        const endLon = fromLonNum + lonDiff * segEnd;
        const steps = 30;
        
        for (let i = 1; i <= steps; i++) {
          const ratio = i / steps;
          const lon = startLon + (endLon - startLon) * ratio;
          // Keep lat constant (horizontal street)
          points.push({
            lat: currentLat,
            lon: lon
          });
        }
        currentLon = endLon;
      }
    }
  }
  
  // Ensure end point is exactly at destination
  points.push({ lat: toLatNum, lon: toLonNum });

  res.json({ polyline: points });
});

export default router;

