import { Router, Request, Response } from 'express';
import { Server } from 'socket.io';
import { readJson, writeJson } from '../services/dataService';
import { SOS, Station } from '../types';

const router = Router();
const ASSIGNMENT_TIMEOUT_MS = 2 * 60 * 1000; // 2 phút

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

function getRequiredStationType(sosType: string): 'medical' | 'rescue' | null {
  if (sosType === 'medical' || sosType === 'accident') {
    return 'medical';
  }
  if (sosType === 'breakdown' || sosType === 'other') {
    return 'rescue';
  }
  return null;
}

// Tìm trạm từ danh sách readyStationIds, sắp xếp theo khoảng cách (gần đến xa)
export function findNearestFromReadyStations(
  sosLat: number,
  sosLon: number,
  sosType: string,
  readyStationIds: string[],
  excludeStationIds: string[] = []
): Station | null {
  try {
    const stations = readJson<Station>('stations.json');
    const requiredType = getRequiredStationType(sosType);
    
    // Lọc các trạm trong readyStationIds và loại trừ các trạm đã từ chối
    const readyStations = stations.filter(s => 
      readyStationIds.includes(s.id) && 
      !excludeStationIds.includes(s.id) &&
      s.lat && s.lon && !isNaN(s.lat) && !isNaN(s.lon) &&
      (!requiredType || s.type === requiredType)
    );
    
    if (readyStations.length === 0) {
      return null;
    }
    
    // Tính khoảng cách và sắp xếp theo khoảng cách (gần đến xa)
    const stationsWithDistance = readyStations.map(station => ({
      station,
      distance: calculateDistance(sosLat, sosLon, station.lat, station.lon)
    }));
    
    stationsWithDistance.sort((a, b) => a.distance - b.distance);
    
    const nearest = stationsWithDistance[0];
    console.log(`Found nearest ready station: ${nearest.station.stationName || 'Unknown'} (${nearest.station.id}), distance: ${nearest.distance.toFixed(2)} km`);
    
    return nearest.station;
  } catch (error: any) {
    console.error('Error finding nearest from ready stations:', error.message);
    return null;
  }
}

function findNearestStation(sosLat: number, sosLon: number, sosType: string, excludeStationIds: string[] = []): Station | null {
  try {
    const stations = readJson<Station>('stations.json');
    const requiredType = getRequiredStationType(sosType);
    
    // Tối ưu: Lọc và tính khoảng cách trong một lần duyệt
    const excludeSet = new Set(excludeStationIds);
    let nearestStation: Station | null = null;
    let minDistance = Infinity;
    
    for (const station of stations) {
      // Kiểm tra loại trạm
      if (requiredType && station.type !== requiredType) {
        continue;
      }
      
      // Loại trừ các trạm đã từ chối
      if (excludeSet.has(station.id)) {
        continue;
      }
      
      // Kiểm tra tọa độ hợp lệ
      if (!station.lat || !station.lon || isNaN(station.lat) || isNaN(station.lon)) {
        continue;
      }
      
      // Tính khoảng cách và so sánh ngay
      const distance = calculateDistance(sosLat, sosLon, station.lat, station.lon);
      if (distance < minDistance) {
        minDistance = distance;
        nearestStation = station;
      }
    }
    
    if (nearestStation) {
      console.log(`Found nearest station: ${nearestStation.stationName || 'Unknown'} (${nearestStation.id}), distance: ${minDistance.toFixed(2)} km`);
    } else {
      console.log('No stations found for SOS type:', sosType, excludeStationIds.length > 0 ? `(excluded ${excludeStationIds.length} stations)` : '');
    }
    
    return nearestStation;
  } catch (error: any) {
    console.error('Error finding nearest station:', error.message);
    return null;
  }
}

// Get Socket.IO instance
function getIO(req: Request): Server {
  return req.app.get('io');
}

function setAssignmentDeadline(sosList: SOS[], index: number) {
  const now = Date.now();
  sosList[index].assignedAt = new Date(now).toISOString();
  sosList[index].assignmentExpiresAt = new Date(now + ASSIGNMENT_TIMEOUT_MS).toISOString();
}

function clearAssignmentDeadline(sosList: SOS[], index: number) {
  sosList[index].assignedAt = undefined;
  sosList[index].assignmentExpiresAt = undefined;
}

export function findNextStationForSOS(sos: SOS): Station | null {
  if (!sos.location) {
    return null;
  }
  const rejectedStations = sos.rejectedStationIds || [];
  const readyStations = sos.readyStationIds || [];

  console.log(`\n[findNextStationForSOS] ========================================`);
  console.log(`[findNextStationForSOS] Tìm trạm tiếp theo cho SOS ${sos.id}:`);
  console.log(`[findNextStationForSOS]   - Ready stations (${readyStations.length}):`, readyStations);
  console.log(`[findNextStationForSOS]   - Rejected stations (${rejectedStations.length}):`, rejectedStations);
  console.log(`[findNextStationForSOS]   - SOS location:`, sos.location);
  console.log(`[findNextStationForSOS]   - SOS type:`, sos.type);

  let nextStation: Station | null = null;
  
  // ƯU TIÊN 1: Tìm trong các trạm đã bấm "Sẵn sàng hỗ trợ" (sắp xếp theo khoảng cách)
  if (readyStations.length > 0) {
    console.log(`[findNextStationForSOS] 🔍 Ưu tiên tìm trong ${readyStations.length} trạm sẵn sàng...`);
    nextStation = findNearestFromReadyStations(
      sos.location.lat,
      sos.location.lon,
      sos.type,
      readyStations,
      rejectedStations
    );
    if (nextStation) {
      console.log(`[findNextStationForSOS] ✅ Tìm thấy trạm sẵn sàng gần nhất: ${nextStation.id} (${nextStation.stationName || 'Unknown'})`);
    } else {
      console.log(`[findNextStationForSOS] ⚠️ Không tìm thấy trạm sẵn sàng phù hợp`);
    }
  } else {
    console.log(`[findNextStationForSOS] ℹ️ Không có trạm sẵn sàng, tìm trạm gần nhất...`);
  }

  // ƯU TIÊN 2: Nếu không có trạm sẵn sàng, tìm trạm gần nhất
  if (!nextStation) {
    console.log(`[findNextStationForSOS] 🔍 Tìm trạm gần nhất (không có trạm sẵn sàng)...`);
    nextStation = findNearestStation(
      sos.location.lat,
      sos.location.lon,
      sos.type,
      rejectedStations
    );
    if (nextStation) {
      console.log(`[findNextStationForSOS] ✅ Tìm thấy trạm gần nhất: ${nextStation.id} (${nextStation.stationName || 'Unknown'})`);
    } else {
      console.log(`[findNextStationForSOS] ❌ Không tìm thấy trạm nào`);
    }
  }

  return nextStation;
}

// Hàm kiểm tra và reassign nếu có trạm sẵn sàng tốt hơn (chỉ khi SOS đang pending và chưa accept)
function checkAndReassignToReadyStation(sos: SOS, sosList: SOS[], index: number, io?: Server): boolean {
  // Chỉ reassign nếu:
  // 1. SOS đang pending (chưa accept)
  // 2. Có assignedStationId (đã được gán)
  // 3. Có readyStationIds (có trạm sẵn sàng)
  // 4. Trạm hiện tại KHÔNG trong readyStationIds (trạm hiện tại chưa bấm sẵn sàng)
  if (!sos.location || 
      sos.status !== 'pending' || 
      !sos.assignedStationId || 
      !sos.readyStationIds || 
      sos.readyStationIds.length === 0) {
    return false;
  }

  // Nếu trạm hiện tại đã bấm sẵn sàng, không cần reassign
  if (sos.readyStationIds.includes(sos.assignedStationId)) {
    return false;
  }

  // Tìm trạm sẵn sàng gần nhất (loại trừ trạm hiện tại)
  const readyStation = findNearestFromReadyStations(
    sos.location.lat,
    sos.location.lon,
    sos.type,
    sos.readyStationIds,
    [sos.assignedStationId] // Loại trừ trạm hiện tại
  );

  if (!readyStation) {
    return false;
  }

  // Tính khoảng cách để so sánh
  const stations = readJson<Station>('stations.json');
  const currentStation = stations.find(s => s.id === sos.assignedStationId);
  
  if (currentStation && currentStation.lat && currentStation.lon) {
    const currentDistance = calculateDistance(
      sos.location.lat,
      sos.location.lon,
      currentStation.lat,
      currentStation.lon
    );
    const readyDistance = calculateDistance(
      sos.location.lat,
      sos.location.lon,
      readyStation.lat,
      readyStation.lon
    );

    // Reassign nếu trạm sẵn sàng gần hơn HOẶC nếu gần tương đương (ưu tiên trạm sẵn sàng)
    // Cho phép reassign nếu trạm sẵn sàng gần hơn hoặc chênh lệch < 2km (ưu tiên trạm chủ động)
    const shouldReassign = readyDistance < currentDistance || (currentDistance - readyDistance) < 2;
    
    if (shouldReassign) {
      console.log(`[REASSIGN] 🔄 Reassigning SOS ${sos.id} from station ${sos.assignedStationId} (${currentDistance.toFixed(2)} km) to ready station ${readyStation.id} (${readyDistance.toFixed(2)} km)`);
      
      // Thêm trạm cũ vào rejected
      const rejectedStations = new Set(sos.rejectedStationIds || []);
      rejectedStations.add(sos.assignedStationId);
      sosList[index].rejectedStationIds = Array.from(rejectedStations);
      
      // Reassign
      sosList[index].assignedStationId = readyStation.id;
      setAssignmentDeadline(sosList, index);
      sosList[index].updatedAt = new Date().toISOString();
      
      // Xóa trạm mới khỏi readyStationIds
      const filteredReadyStations = sos.readyStationIds.filter(id => id !== readyStation.id);
      sosList[index].readyStationIds = filteredReadyStations.length > 0 ? filteredReadyStations : undefined;
      
      return true;
    }
  }

  return false;
}

export function enforceAssignmentTimeouts(io?: Server): SOS[] {
  // QUAN TRỌNG: Đọc lại từ file để có readyStationIds mới nhất (có thể đã được cập nhật bởi trạm bấm "Sẵn sàng")
  let sosList = readJson<SOS>('sos.json');
  if (!Array.isArray(sosList)) {
    sosList = [];
  }

  const now = Date.now();
  let changed = false;
  const updatedItems: SOS[] = [];

  // Log để debug readyStationIds - ĐỌC TỪ FILE
  console.log(`\n[enforceAssignmentTimeouts] ========================================`);
  console.log(`[enforceAssignmentTimeouts] Checking ${sosList.length} SOS for expired assignments and ready stations...`);
  sosList.forEach((sos) => {
    if (sos && sos.readyStationIds && sos.readyStationIds.length > 0) {
      console.log(`[enforceAssignmentTimeouts] ✅ SOS ${sos.id} has ${sos.readyStationIds.length} ready stations:`, sos.readyStationIds);
    }
  });

  // BƯỚC 1: Kiểm tra và reassign sang trạm sẵn sàng (nếu có trạm sẵn sàng tốt hơn)
  sosList.forEach((sos, index) => {
    if (checkAndReassignToReadyStation(sos, sosList, index, io)) {
      changed = true;
      updatedItems.push(sosList[index]);
    }
  });

  // BƯỚC 2: Kiểm tra timeout (hết thời gian)
  sosList.forEach((sos, index) => {
    if (
      sos &&
      sos.status === 'pending' &&
      sos.assignedStationId &&
      sos.assignmentExpiresAt
    ) {
      const expiresAt = Date.parse(sos.assignmentExpiresAt);
      if (!Number.isNaN(expiresAt) && expiresAt <= now) {
        console.log(`[SOS] ⏳ Assignment for ${sos.id} expired for station ${sos.assignedStationId}`);
        console.log(`[SOS] 📋 Ready stations before reassign:`, sos.readyStationIds);
        console.log(`[SOS] 📋 Rejected stations before reassign:`, sos.rejectedStationIds);

        // QUAN TRỌNG: Đọc lại từ file để có readyStationIds mới nhất (tránh cache)
        // Có thể trạm đã bấm "Sẵn sàng" sau khi SOS được tạo
        const freshSosList = readJson<SOS>('sos.json');
        const freshSos = freshSosList.find(s => s.id === sos.id);
        if (freshSos) {
          // Cập nhật readyStationIds từ file mới nhất
          sos.readyStationIds = freshSos.readyStationIds || [];
          sos.rejectedStationIds = freshSos.rejectedStationIds || [];
          console.log(`[TIMEOUT] 📥 Đọc lại từ file - Ready stations:`, sos.readyStationIds);
          console.log(`[TIMEOUT] 📥 Đọc lại từ file - Rejected stations:`, sos.rejectedStationIds);
        }

        const rejectedStations = new Set(sos.rejectedStationIds || []);
        rejectedStations.add(sos.assignedStationId);
        sos.rejectedStationIds = Array.from(rejectedStations);

        // Tìm trạm tiếp theo - ƯU TIÊN các trạm đã bấm "Sẵn sàng hỗ trợ"
        const nextStation = findNextStationForSOS(sos);
        if (nextStation) {
          console.log(`[SOS] 🔁 Reassigning ${sos.id} to station ${nextStation.id}`);
          console.log(`[SOS] 📋 Ready stations before reassign:`, sos.readyStationIds);
          console.log(`[SOS] ✅ Next station ${nextStation.id} ${sos.readyStationIds?.includes(nextStation.id) ? 'WAS in ready list (priority)' : 'was NOT in ready list'}`);
          
          sos.assignedStationId = nextStation.id;
          setAssignmentDeadline(sosList, index);
          sos.updatedAt = new Date().toISOString();

          // Xóa trạm mới được gán khỏi readyStationIds (nếu có)
          if (sos.readyStationIds && sos.readyStationIds.length > 0) {
            const filteredReadyStations = sos.readyStationIds.filter(id => id !== nextStation.id);
            sos.readyStationIds = filteredReadyStations.length > 0 ? filteredReadyStations : undefined;
            console.log(`[SOS] 📋 Ready stations after reassign:`, sos.readyStationIds);
          }
        } else {
          console.log(`[SOS] ⚠️ No available station to reassign ${sos.id}`);
          sos.assignedStationId = undefined;
          clearAssignmentDeadline(sosList, index);
          sos.updatedAt = new Date().toISOString();
        }

        changed = true;
        updatedItems.push(sosList[index]);
      }
    }
  });

  if (changed) {
    writeJson('sos.json', sosList);
    if (io) {
      updatedItems.forEach(updatedSOS => {
        io.emit('sos:update', updatedSOS);
      });
    }
  }

  return sosList;
}

// Create SOS
router.post('/', (req: Request, res: Response) => {
  const { userId, deviceId, type, severity, location, note } = req.body;

  console.log('Create SOS request:', { userId, type, severity, location, note });

  if (!userId || !type || !severity || !location || !location.lat || !location.lon) {
    console.error('Missing required fields:', { userId, type, severity, location });
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc' });
  }

  // Validate và convert location coordinates
  const lat = parseFloat(location.lat);
  const lon = parseFloat(location.lon);
  
  if (isNaN(lat) || isNaN(lon)) {
    console.error('Invalid coordinates:', { lat: location.lat, lon: location.lon });
    return res.status(400).json({ error: 'Tọa độ không hợp lệ' });
  }
  
  // Validate coordinate ranges (lat: -90 to 90, lon: -180 to 180)
  if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
    console.error('Coordinates out of range:', { lat, lon });
    return res.status(400).json({ error: 'Tọa độ nằm ngoài phạm vi hợp lệ' });
  }

  console.log(`SOS location validated: lat=${lat}, lon=${lon}`);

  try {
    const io = getIO(req);
    let sosList = enforceAssignmentTimeouts(io);
    
    // Đảm bảo sosList là array
    if (!Array.isArray(sosList)) {
      console.warn('SOS list is not an array, initializing new array');
      sosList = [];
    }
    
    console.log(`Current SOS list length: ${sosList.length}`);
    
    const newSOS: SOS = {
      id: `SOS${Date.now()}`,
      userId,
      deviceId,
      type: type as 'accident' | 'breakdown' | 'medical' | 'other',
      severity: severity as 'low' | 'medium' | 'high' | 'critical',
      location: {
        lat: lat,
        lon: lon
      },
      status: 'pending',
      note: note || '',
      rejectedStationIds: [], // Khởi tạo danh sách trạm đã từ chối
      readyStationIds: [], // Khởi tạo danh sách trạm sẵn sàng nhận nhiệm vụ
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString()
    };

    sosList.push(newSOS);
    writeJson('sos.json', sosList);
    console.log(`SOS created successfully: ${newSOS.id}`);

    // Tự động tìm và gán trạm - ƯU TIÊN các trạm đã bấm "Sẵn sàng hỗ trợ" (nếu có)
    try {
      // Dùng findNextStationForSOS để đảm bảo ưu tiên ready stations (nếu có)
      // Khi SOS mới tạo, readyStationIds thường rỗng, nhưng vẫn dùng hàm này để nhất quán
      const nearestStation = findNextStationForSOS(newSOS);
      
      if (nearestStation) {
        // Tự động gán SOS cho trạm - đặt status = 'pending' (đang kết nối)
        // Trạm sẽ phải claim (nhận) thì mới chuyển sang 'accepted'
        const sosIndex = sosList.findIndex(s => s.id === newSOS.id);
        if (sosIndex !== -1) {
          sosList[sosIndex].assignedStationId = nearestStation.id;
          sosList[sosIndex].status = 'pending'; // Đặt pending để trạm có thể nhận
          setAssignmentDeadline(sosList, sosIndex);
          sosList[sosIndex].updatedAt = new Date().toISOString();
          writeJson('sos.json', sosList);
          
          const isReadyStation = newSOS.readyStationIds?.includes(nearestStation.id);
          console.log(`SOS ${newSOS.id} automatically assigned to station ${nearestStation.id} (${nearestStation.stationName}), status: pending (waiting for station to accept)`);
          console.log(`  ${isReadyStation ? '✅ Trạm này đã bấm "Sẵn sàng" - được ưu tiên!' : 'ℹ️ Trạm này chưa bấm "Sẵn sàng"'}`);
          
          // Cập nhật newSOS để trả về
          newSOS.assignedStationId = nearestStation.id;
          newSOS.status = 'pending';
          newSOS.assignedAt = sosList[sosIndex].assignedAt;
          newSOS.assignmentExpiresAt = sosList[sosIndex].assignmentExpiresAt;
          newSOS.updatedAt = sosList[sosIndex].updatedAt;
        }
      } else {
        console.log(`No station found for SOS ${newSOS.id}, keeping status as pending`);
      }
    } catch (autoAssignError: any) {
      console.error('Error auto-assigning station:', autoAssignError.message);
      // Tiếp tục dù có lỗi auto-assign
    }

    // Broadcast new SOS
    try {
      const io = getIO(req);
      io.emit('sos:new', newSOS);
      io.emit('sos:update', newSOS);
    } catch (socketError: any) {
      console.error('Error broadcasting SOS:', socketError.message);
      // Tiếp tục dù có lỗi socket
    }

    res.status(201).json(newSOS);
  } catch (error: any) {
    console.error('Create SOS error:', error);
    console.error('Error stack:', error.stack);
    res.status(500).json({ error: 'Lỗi server: ' + (error.message || 'Unknown error') });
  }
});

// Get all SOS
router.get('/', (req: Request, res: Response) => {
  const { status, userId, stationId } = req.query;

  try {
    let sosList = readJson<SOS>('sos.json');
    
    // Đảm bảo sosList là array
    if (!Array.isArray(sosList)) {
      console.warn('SOS list is not an array, returning empty array');
      return res.json([]);
    }

    if (status) {
      sosList = sosList.filter(sos => sos && sos.status === status);
    }

    if (userId) {
      sosList = sosList.filter(sos => sos && sos.userId === userId);
    }

    if (stationId) {
      sosList = sosList.filter(sos => sos && sos.assignedStationId === stationId);
    }

    res.json(sosList);
  } catch (error) {
    console.error('Get SOS error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Check SOS status for hardware (để phần cứng kiểm tra SOS có bị hủy không)
router.get('/check/:userId', (req: Request, res: Response) => {
  const { userId } = req.params;

  try {
    const sosList = readJson<SOS>('sos.json');
    
    // Tìm SOS active (pending, accepted, on_route) của user
    const activeSOS = sosList.find(sos => 
      sos && 
      sos.userId === userId &&
      sos.status !== 'done' &&
      sos.status !== 'cancelled' &&
      sos.type === 'accident' // Chỉ kiểm tra SOS tai nạn
    );
    
    if (activeSOS) {
      // Có SOS active
      return res.json({
        hasActiveSOS: true,
        sosId: activeSOS.id,
        status: activeSOS.status,
        severity: activeSOS.severity,
        createdAt: activeSOS.createdAt
      });
    } else {
      // Không có SOS active
      return res.json({
        hasActiveSOS: false,
        message: 'No active SOS found'
      });
    }
  } catch (error) {
    console.error('Check SOS status error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get SOS by ID
router.get('/:id', (req: Request, res: Response) => {
  const { id } = req.params;

  try {
    const sosList = readJson<SOS>('sos.json');
    const sos = sosList.find(s => s.id === id);

    if (!sos) {
      return res.status(404).json({ error: 'Không tìm thấy SOS' });
    }

    res.json(sos);
  } catch (error) {
    console.error('Get SOS by ID error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Claim SOS (station accepts task)
router.patch('/:id/claim', (req: Request, res: Response) => {
  const { id } = req.params;
  const { stationId } = req.body;

  console.log('Claim SOS request:', { id, stationId, body: req.body });

  if (!stationId) {
    console.error('Missing stationId in claim request');
    return res.status(400).json({ error: 'Thiếu stationId' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    console.log('Total SOS in file:', sosList.length);
    console.log('Looking for SOS ID:', id);
    console.log('Available SOS IDs:', sosList.map(s => ({ id: s.id, status: s.status })));
    
    // Try to find SOS - check both exact match and string comparison
    let sosIndex = sosList.findIndex(s => s.id === id);
    
    // If not found, try string comparison (in case of type mismatch)
    if (sosIndex === -1) {
      sosIndex = sosList.findIndex(s => String(s.id) === String(id));
    }

    if (sosIndex === -1) {
      console.error('SOS not found:', id);
      console.log('Available SOS IDs:', sosList.map(s => s.id));
      return res.status(404).json({ error: `Không tìm thấy SOS với ID: ${id}. Tổng số SOS: ${sosList.length}` });
    }

    const sos = sosList[sosIndex];
    console.log('Found SOS:', sos);

    const requiredType = getRequiredStationType(sos.type);
    if (requiredType) {
      const stations = readJson<Station>('stations.json');
      const station = stations.find(s => s.id === stationId);
      if (!station) {
        return res.status(404).json({ error: 'Không tìm thấy thông tin trạm' });
      }
      if (station.type !== requiredType) {
        return res.status(400).json({ error: 'Trạm không phù hợp với loại SOS này' });
      }
    }

    if (sos.status !== 'pending') {
      console.error('SOS already claimed or processed:', sos.status);
      return res.status(400).json({ error: `SOS đã được nhận hoặc đã xử lý. Trạng thái hiện tại: ${sos.status}` });
    }

    sosList[sosIndex].assignedStationId = stationId;
    const currentReadyStations = sosList[sosIndex].readyStationIds;
    if (currentReadyStations && currentReadyStations.length > 0) {
      const filteredReadyStations = currentReadyStations.filter(id => id !== stationId);
      if (filteredReadyStations.length > 0) {
        sosList[sosIndex].readyStationIds = filteredReadyStations;
      } else {
        delete sosList[sosIndex].readyStationIds;
      }
    }
    sosList[sosIndex].status = 'accepted';
    clearAssignmentDeadline(sosList, sosIndex);
    sosList[sosIndex].updatedAt = new Date().toISOString();

    writeJson('sos.json', sosList);
    console.log('SOS claimed successfully:', sosList[sosIndex]);

    // Broadcast update - đảm bảo tất cả clients nhận được cập nhật
    const io = getIO(req);
    const updatedSOS = sosList[sosIndex];
    io.emit('sos:update', updatedSOS);
    // Broadcast riêng cho user để đảm bảo họ nhận được thông tin trạm ngay
    io.to(`user:${updatedSOS.userId}`).emit('sos:update', updatedSOS);
    // Broadcast cho tất cả trạm
    io.emit('sos:claimed', updatedSOS);

    res.json(updatedSOS);
  } catch (error: any) {
    console.error('Claim SOS error:', error);
    console.error('Error stack:', error.stack);
    res.status(500).json({ error: `Lỗi server: ${error.message || 'Unknown error'}` });
  }
});

// Update SOS status
router.patch('/:id/status', (req: Request, res: Response) => {
  const { id } = req.params;
  const { status, stationId } = req.body; // Thêm stationId để phân biệt user hay station đang update

  if (!status || !['accepted', 'on_route', 'done', 'cancelled'].includes(status)) {
    return res.status(400).json({ error: 'Status không hợp lệ' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    const sosIndex = sosList.findIndex(s => s.id === id);

    if (sosIndex === -1) {
      return res.status(404).json({ error: 'Không tìm thấy SOS' });
    }

    const sos = sosList[sosIndex];
    const oldStatus = sos.status;
    const oldStationId = sos.assignedStationId;

    console.log(`Update SOS status request:`, { 
      id, 
      status, 
      stationId, 
      oldStatus, 
      oldStationId,
      hasLocation: !!sos.location 
    });

    // Xử lý trường hợp cancelled
    if (status === 'cancelled') {
      // Nếu là trạm từ chối/hủy nhiệm vụ (có stationId và đúng là trạm được gán), tự động tìm trạm tiếp theo
      // Xử lý cả pending, accepted và on_route
      if (stationId && oldStationId === stationId && sos.location && 
          (oldStatus === 'pending' || oldStatus === 'accepted' || oldStatus === 'on_route')) {
        console.log(`\n=== TRẠM TỪ CHỐI - TỰ ĐỘNG TÍNH TOÁN VÀ CẬP NHẬT TRẠM MỚI ===`);
        console.log(`Station ${stationId} cancelling/rejecting SOS ${id} (oldStatus: ${oldStatus})`);
        console.log(`SOS Location: ${sos.location.lat}, ${sos.location.lon}`);
        console.log(`SOS Type: ${sos.type}`);
        
        // Lấy danh sách các trạm đã từ chối (nếu có)
        const rejectedStations = sos.rejectedStationIds || [];
        if (!rejectedStations.includes(stationId)) {
          rejectedStations.push(stationId); // Thêm trạm hiện tại vào danh sách từ chối
        }
        console.log(`Danh sách trạm đã từ chối:`, rejectedStations);
        
        // QUAN TRỌNG: Đọc lại từ file để có readyStationIds mới nhất (tránh cache)
        // Có thể trạm đã bấm "Sẵn sàng" sau khi SOS được tạo
        const freshSosList = readJson<SOS>('sos.json');
        const freshSos = freshSosList.find(s => s.id === id);
        if (freshSos) {
          // Cập nhật readyStationIds và rejectedStationIds từ file mới nhất
          sos.readyStationIds = freshSos.readyStationIds || [];
          sos.rejectedStationIds = [...(freshSos.rejectedStationIds || []), ...rejectedStations.filter(r => !freshSos.rejectedStationIds?.includes(r))];
          console.log(`[STATUS] 📥 Đọc lại từ file - Ready stations:`, sos.readyStationIds);
          console.log(`[STATUS] 📥 Đọc lại từ file - Rejected stations:`, sos.rejectedStationIds);
        }
        
        // TỰ ĐỘNG TÍNH TOÁN VÀ TÌM TRẠM TIẾP THEO NGAY LẬP TỨC
        // Ưu tiên: 1) Các trạm đã ấn "Sẵn sàng" (sắp xếp theo khoảng cách), 2) Trạm gần nhất như bình thường
        // Dùng findNextStationForSOS để đảm bảo logic ưu tiên nhất quán
        const readyStations = sos.readyStationIds || [];
        console.log(`Đang tìm trạm tiếp theo (loại trừ ${sos.rejectedStationIds?.length || 0} trạm đã từ chối, có ${readyStations.length} trạm sẵn sàng)...`);
        
        // Cập nhật rejectedStationIds trước khi tìm
        sos.rejectedStationIds = rejectedStations;
        
        // Dùng findNextStationForSOS để đảm bảo ưu tiên ready stations
        const nextStation = findNextStationForSOS(sos);
        
        if (nextStation) {
          // TỰ ĐỘNG CẬP NHẬT TRẠM MỚI NGAY LẬP TỨC
          sosList[sosIndex].assignedStationId = nextStation.id;
          sosList[sosIndex].status = 'pending'; // Đặt pending để trạm mới có thể nhận
          setAssignmentDeadline(sosList, sosIndex);
          sosList[sosIndex].rejectedStationIds = rejectedStations; // Lưu danh sách trạm đã từ chối
          const currentReadyStations = sosList[sosIndex].readyStationIds;
          if (currentReadyStations && currentReadyStations.length > 0) {
            const filteredReadyStations = currentReadyStations.filter(id => id !== nextStation.id);
            if (filteredReadyStations.length > 0) {
              sosList[sosIndex].readyStationIds = filteredReadyStations;
            } else {
              delete sosList[sosIndex].readyStationIds;
            }
          }
          sosList[sosIndex].updatedAt = new Date().toISOString();
          
          // Ghi file ngay lập tức để đảm bảo dữ liệu được lưu
          writeJson('sos.json', sosList);
          
          console.log(`✅ ĐÃ TỰ ĐỘNG CẬP NHẬT: SOS ${id} được gán cho trạm mới ${nextStation.id} (${nextStation.stationName || 'Unknown'})`);
          console.log(`   Status: pending (chờ trạm mới nhận)`);
          console.log(`   Thời gian cập nhật: ${sosList[sosIndex].updatedAt}`);
          console.log(`=== HOÀN TẤT TỰ ĐỘNG CẬP NHẬT ===\n`);
        } else {
          // Không tìm thấy trạm tiếp theo, đặt về pending (không có trạm nào)
          sosList[sosIndex].assignedStationId = undefined;
          clearAssignmentDeadline(sosList, sosIndex);
          sosList[sosIndex].status = 'pending';
          sosList[sosIndex].rejectedStationIds = rejectedStations; // Vẫn lưu danh sách từ chối
          sosList[sosIndex].updatedAt = new Date().toISOString();
          
          // Ghi file ngay lập tức
          writeJson('sos.json', sosList);
          
          console.log(`⚠️ Không tìm thấy trạm tiếp theo cho SOS ${id}`);
          console.log(`   Tất cả trạm phù hợp đã từ chối hoặc không có trạm nào khả dụng`);
          console.log(`   Status: pending (chờ trạm mới)`);
          console.log(`=== HOÀN TẤT ===\n`);
        }
      } 
      // Nếu là user hủy SOS (không có stationId hoặc không phải trạm được gán), hủy trực tiếp
      else {
        console.log(`User cancelling SOS ${id}, setting status to cancelled (stationId: ${stationId}, oldStationId: ${oldStationId})`);
        sosList[sosIndex].status = 'cancelled';
        sosList[sosIndex].updatedAt = new Date().toISOString();
        // Xóa assignedStationId khi user hủy
        sosList[sosIndex].assignedStationId = undefined;
        clearAssignmentDeadline(sosList, sosIndex);
      }
    }
    // Các trường hợp khác (update status bình thường)
    else {
      sosList[sosIndex].status = status as 'accepted' | 'on_route' | 'done' | 'cancelled';
      sosList[sosIndex].updatedAt = new Date().toISOString();
      if (status !== 'pending') {
        clearAssignmentDeadline(sosList, sosIndex);
      }
    }

    // Chỉ ghi file nếu chưa ghi trong phần cancelled (để tránh ghi 2 lần)
    if (!(status === 'cancelled' && stationId && oldStationId === stationId && 
          (oldStatus === 'pending' || oldStatus === 'accepted' || oldStatus === 'on_route'))) {
      writeJson('sos.json', sosList);
    }

    // Broadcast update - đảm bảo broadcast cả khi reassign
    const io = getIO(req);
    const updatedSOS = sosList[sosIndex];
    console.log('📡 Broadcasting SOS update:', {
      id: updatedSOS.id,
      status: updatedSOS.status,
      assignedStationId: updatedSOS.assignedStationId,
      rejectedStationIds: updatedSOS.rejectedStationIds
    });
    io.emit('sos:update', updatedSOS);
    
    // Nếu có reassign, broadcast thêm một lần để đảm bảo client nhận được ngay lập tức
    if (status === 'cancelled' && stationId && 
        (oldStatus === 'pending' || oldStatus === 'accepted' || oldStatus === 'on_route') &&
        updatedSOS.status === 'pending' && updatedSOS.assignedStationId) {
      console.log('📡 Broadcasting reassigned SOS (pending with new station) - đảm bảo client nhận được:', {
        id: updatedSOS.id,
        newStationId: updatedSOS.assignedStationId,
        rejectedStations: updatedSOS.rejectedStationIds
      });
      // Broadcast ngay lập tức và thêm một lần nữa sau 200ms để đảm bảo
      setTimeout(() => {
        io.emit('sos:update', updatedSOS);
        io.emit('sos:reassigned', updatedSOS); // Thêm event riêng cho reassign
      }, 200);
    }

    res.json(updatedSOS);
  } catch (error) {
    console.error('Update SOS status error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Endpoint để trạm báo "Sẵn sàng nhận nhiệm vụ" (toggle ready status)
router.patch('/:id/ready', (req: Request, res: Response) => {
  const { id } = req.params;
  const { stationId, ready } = req.body; // ready: true = sẵn sàng, false = hủy sẵn sàng

  if (!stationId) {
    return res.status(400).json({ error: 'Thiếu stationId' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    const sosIndex = sosList.findIndex(s => s.id === id);

    if (sosIndex === -1) {
      return res.status(404).json({ error: 'Không tìm thấy SOS' });
    }

    const sos = sosList[sosIndex];
    
    // Chỉ cho phép báo sẵn sàng nếu SOS đang pending và chưa được gán cho trạm này
    if (sos.status !== 'pending') {
      return res.status(400).json({ error: 'Chỉ có thể báo sẵn sàng cho SOS đang pending' });
    }

    // Nếu SOS đã được gán cho trạm này, không cần báo sẵn sàng
    if (sos.assignedStationId === stationId) {
      return res.status(400).json({ error: 'SOS đã được gán cho trạm này, không cần báo sẵn sàng' });
    }

    const readyStations = sos.readyStationIds || [];
    const requiredType = getRequiredStationType(sos.type);

    const stations = readJson<Station>('stations.json');
    const station = stations.find(s => s.id === stationId);
    if (!station) {
      return res.status(404).json({ error: 'Không tìm thấy thông tin trạm' });
    }

    if (requiredType && station.type !== requiredType) {
      return res.status(400).json({ error: 'Trạm không phù hợp với loại SOS này' });
    }
    const isReady = readyStations.includes(stationId);

    if (ready === true && !isReady) {
      // Thêm trạm vào danh sách sẵn sàng
      readyStations.push(stationId);
      sosList[sosIndex].readyStationIds = readyStations;
      console.log(`[READY] ✅ Station ${stationId} marked as ready for SOS ${id}`);
      console.log(`[READY] 📋 Ready stations list:`, readyStations);
    } else if (ready === false && isReady) {
      // Xóa trạm khỏi danh sách sẵn sàng
      const index = readyStations.indexOf(stationId);
      if (index > -1) {
        readyStations.splice(index, 1);
      }
      sosList[sosIndex].readyStationIds = readyStations.length > 0 ? readyStations : undefined;
      console.log(`[READY] ❌ Station ${stationId} unmarked as ready for SOS ${id}`);
      console.log(`[READY] 📋 Ready stations list after removal:`, sosList[sosIndex].readyStationIds);
    } else {
      console.log(`[READY] ℹ️ No change needed - station ${stationId} ready status: ${isReady}, requested: ${ready}`);
    }

    sosList[sosIndex].updatedAt = new Date().toISOString();
    writeJson('sos.json', sosList);
    console.log(`[READY] 💾 Saved SOS ${id} with readyStationIds:`, sosList[sosIndex].readyStationIds);

    // Broadcast update - QUAN TRỌNG để tất cả trạm thấy readyStationIds được cập nhật
    const io = getIO(req);
    const updatedSOS = sosList[sosIndex];
    console.log(`[SOS] 📡 Broadcasting ready status update for SOS ${id}:`, {
      stationId,
      ready,
      readyStationIds: updatedSOS.readyStationIds,
      assignedStationId: updatedSOS.assignedStationId
    });
    io.emit('sos:update', updatedSOS);

    res.json(updatedSOS);
  } catch (error: any) {
    console.error('Toggle ready status error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Endpoint để trạm từ chối nhiệm vụ (reject)
router.patch('/:id/reject', (req: Request, res: Response) => {
  const { id } = req.params;
  const { stationId } = req.body;

  if (!stationId) {
    return res.status(400).json({ error: 'Thiếu stationId' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    const sosIndex = sosList.findIndex(s => s.id === id);

    if (sosIndex === -1) {
      return res.status(404).json({ error: 'Không tìm thấy SOS' });
    }

    const sos = sosList[sosIndex];

    // Chỉ cho phép từ chối nếu SOS đang được gán cho trạm này
    if (sos.assignedStationId !== stationId) {
      return res.status(403).json({ error: 'SOS không được gán cho trạm này' });
    }

    if (sos.status !== 'accepted') {
      return res.status(400).json({ error: 'Chỉ có thể từ chối SOS ở trạng thái accepted' });
    }

    console.log(`Station ${stationId} rejecting SOS ${id}, finding next nearest station...`);
    
    // QUAN TRỌNG: Đọc lại từ file để có readyStationIds mới nhất (tránh cache)
    const freshSosList = readJson<SOS>('sos.json');
    const freshSos = freshSosList.find(s => s.id === id);
    if (freshSos) {
      // Cập nhật readyStationIds từ file mới nhất
      sos.readyStationIds = freshSos.readyStationIds || [];
      sos.rejectedStationIds = freshSos.rejectedStationIds || [];
      console.log(`[REJECT] 📥 Đọc lại từ file - Ready stations:`, sos.readyStationIds);
      console.log(`[REJECT] 📥 Đọc lại từ file - Rejected stations:`, sos.rejectedStationIds);
    } else {
      console.log(`[REJECT] Ready stations (from memory):`, sos.readyStationIds);
      console.log(`[REJECT] Rejected stations (from memory):`, sos.rejectedStationIds);
    }

    // Tìm trạm tiếp theo - ƯU TIÊN các trạm đã bấm "Sẵn sàng hỗ trợ"
    const rejectedStations = [...(sos.rejectedStationIds || []), stationId];
    const nextStation = findNextStationForSOS({
      ...sos,
      rejectedStationIds: rejectedStations
    });

    if (nextStation) {
      // Gán cho trạm tiếp theo
      sosList[sosIndex].assignedStationId = nextStation.id;
      sosList[sosIndex].status = 'pending';
      setAssignmentDeadline(sosList, sosIndex);
      sosList[sosIndex].updatedAt = new Date().toISOString();
      console.log(`SOS ${id} reassigned to next nearest station: ${nextStation.id} (${nextStation.stationName})`);
    } else {
      // Không tìm thấy trạm tiếp theo, đặt về pending
      sosList[sosIndex].assignedStationId = undefined;
      clearAssignmentDeadline(sosList, sosIndex);
      sosList[sosIndex].status = 'pending';
      console.log(`No next station found for SOS ${id}, status set to pending`);
    }

    writeJson('sos.json', sosList);

    // Broadcast update
    const io = getIO(req);
    io.emit('sos:update', sosList[sosIndex]);

    res.json(sosList[sosIndex]);
  } catch (error: any) {
    console.error('Reject SOS error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

