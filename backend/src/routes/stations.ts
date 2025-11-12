import { Router, Request, Response } from 'express';
import { readJson } from '../services/dataService';
import { Station } from '../types';

const router = Router();

// Calculate distance between two coordinates (Haversine formula)
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

// Get nearest station
router.get('/nearest', (req: Request, res: Response) => {
  const { lat, lon, type } = req.query;

  if (!lat || !lon || !type) {
    return res.status(400).json({ error: 'Thiếu thông tin tọa độ hoặc loại trạm' });
  }

  const userLat = parseFloat(lat as string);
  const userLon = parseFloat(lon as string);
  const stationType = type as string;

  // Validate coordinates
  if (isNaN(userLat) || isNaN(userLon) || userLat < -90 || userLat > 90 || userLon < -180 || userLon > 180) {
    return res.status(400).json({ error: 'Tọa độ không hợp lệ' });
  }

  try {
    const stations = readJson<Station>('stations.json');
    
    console.log(`Finding nearest ${stationType} station for user at (${userLat}, ${userLon})`);
    console.log(`Total stations available: ${stations.length}`);
    
    // Filter by type
    let filteredStations = stations;
    if (stationType === 'medical') {
      filteredStations = stations.filter(s => s.type === 'medical');
    } else if (stationType === 'rescue') {
      filteredStations = stations.filter(s => s.type === 'rescue' || s.type === 'repair');
    }

    console.log(`Filtered stations by type ${stationType}: ${filteredStations.length}`);

    if (filteredStations.length === 0) {
      return res.status(404).json({ error: 'Không tìm thấy trạm phù hợp' });
    }

    // Calculate distances and find nearest
    let nearestStation = filteredStations[0];
    let minDistance = calculateDistance(userLat, userLon, nearestStation.lat, nearestStation.lon);
    
    console.log(`Initial nearest: ${nearestStation.stationName} at (${nearestStation.lat}, ${nearestStation.lon}), distance: ${minDistance.toFixed(2)} km`);

    filteredStations.forEach(station => {
      // Skip stations with invalid coordinates
      if (!station.lat || !station.lon || isNaN(station.lat) || isNaN(station.lon)) {
        console.warn(`Station ${station.id} (${station.stationName}) has invalid coordinates`);
        return;
      }
      
      const distance = calculateDistance(userLat, userLon, station.lat, station.lon);
      if (distance < minDistance) {
        minDistance = distance;
        nearestStation = station;
        console.log(`New nearest: ${station.stationName} at (${station.lat}, ${station.lon}), distance: ${distance.toFixed(2)} km`);
      }
    });

    console.log(`Final nearest station: ${nearestStation.stationName}, distance: ${minDistance.toFixed(2)} km`);

    res.json({
      ...nearestStation,
      distance: minDistance
    });
  } catch (error) {
    console.error('Get nearest station error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get all stations by type
router.get('/', (req: Request, res: Response) => {
  const { type } = req.query;

  try {
    let stations = readJson<Station>('stations.json');
    
    if (type) {
      stations = stations.filter(s => s.type === type);
    }

    res.json(stations);
  } catch (error) {
    console.error('Get stations error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

