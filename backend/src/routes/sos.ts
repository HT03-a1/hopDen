import { Router, Request, Response } from 'express';
import { Server } from 'socket.io';
import { readJson, writeJson } from '../services/dataService';
import { SOS, Station } from '../types';

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
function findNearestFromReadyStations(
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
    let sosList = readJson<SOS>('sos.json');
    
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

    // Tự động tìm và gán trạm gần nhất
    try {
      const nearestStation = findNearestStation(lat, lon, newSOS.type);
      
      if (nearestStation) {
        // Tự động gán SOS cho trạm gần nhất - đặt status = 'pending' (đang kết nối)
        // Trạm sẽ phải claim (nhận) thì mới chuyển sang 'accepted'
        const sosIndex = sosList.findIndex(s => s.id === newSOS.id);
        if (sosIndex !== -1) {
          sosList[sosIndex].assignedStationId = nearestStation.id;
          sosList[sosIndex].status = 'pending'; // Đặt pending để trạm có thể nhận
          sosList[sosIndex].updatedAt = new Date().toISOString();
          writeJson('sos.json', sosList);
          
          console.log(`SOS ${newSOS.id} automatically assigned to station ${nearestStation.id} (${nearestStation.stationName}), status: pending (waiting for station to accept)`);
          
          // Cập nhật newSOS để trả về
          newSOS.assignedStationId = nearestStation.id;
          newSOS.status = 'pending';
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
        
        // TỰ ĐỘNG TÍNH TOÁN VÀ TÌM TRẠM TIẾP THEO NGAY LẬP TỨC
        // Ưu tiên: 1) Các trạm đã ấn "Sẵn sàng" (sắp xếp theo khoảng cách), 2) Trạm gần nhất như bình thường
        const readyStations = sos.readyStationIds || [];
        console.log(`Đang tìm trạm tiếp theo (loại trừ ${rejectedStations.length} trạm đã từ chối, có ${readyStations.length} trạm sẵn sàng)...`);
        
        let nextStation: Station | null = null;
        
        // Ưu tiên tìm trong các trạm đã ấn "Sẵn sàng"
        if (readyStations.length > 0) {
          nextStation = findNearestFromReadyStations(
            sos.location.lat,
            sos.location.lon,
            sos.type,
            readyStations,
            rejectedStations // Loại trừ các trạm đã từ chối
          );
          if (nextStation) {
            console.log(`✅ Tìm thấy trạm sẵn sàng gần nhất: ${nextStation.id} (${nextStation.stationName || 'Unknown'})`);
          }
        }
        
        // Nếu không có trạm sẵn sàng, tìm trạm gần nhất như bình thường
        if (!nextStation) {
          console.log(`Không có trạm sẵn sàng, tìm trạm gần nhất như bình thường...`);
          nextStation = findNearestStation(
            sos.location.lat, 
            sos.location.lon, 
            sos.type,
            rejectedStations // Loại trừ tất cả các trạm đã từ chối
          );
        }
        
        if (nextStation) {
          // TỰ ĐỘNG CẬP NHẬT TRẠM MỚI NGAY LẬP TỨC
          sosList[sosIndex].assignedStationId = nextStation.id;
          sosList[sosIndex].status = 'pending'; // Đặt pending để trạm mới có thể nhận
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
      }
    }
    // Các trường hợp khác (update status bình thường)
    else {
      sosList[sosIndex].status = status as 'accepted' | 'on_route' | 'done' | 'cancelled';
      sosList[sosIndex].updatedAt = new Date().toISOString();
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
      console.log(`Station ${stationId} marked as ready for SOS ${id}`);
    } else if (ready === false && isReady) {
      // Xóa trạm khỏi danh sách sẵn sàng
      const index = readyStations.indexOf(stationId);
      if (index > -1) {
        readyStations.splice(index, 1);
      }
      sosList[sosIndex].readyStationIds = readyStations.length > 0 ? readyStations : undefined;
      console.log(`Station ${stationId} unmarked as ready for SOS ${id}`);
    }

    sosList[sosIndex].updatedAt = new Date().toISOString();
    writeJson('sos.json', sosList);

    // Broadcast update
    const io = getIO(req);
    io.emit('sos:update', sosList[sosIndex]);

    res.json(sosList[sosIndex]);
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

    // Tìm trạm tiếp theo (loại trừ trạm đã từ chối)
    const nextStation = findNearestStation(
      sos.location.lat,
      sos.location.lon,
      sos.type,
      [stationId] // Loại trừ trạm đã từ chối
    );

    if (nextStation) {
      // Gán cho trạm tiếp theo
      sosList[sosIndex].assignedStationId = nextStation.id;
      sosList[sosIndex].status = 'accepted';
      sosList[sosIndex].updatedAt = new Date().toISOString();
      console.log(`SOS ${id} reassigned to next nearest station: ${nextStation.id} (${nextStation.stationName})`);
    } else {
      // Không tìm thấy trạm tiếp theo, đặt về pending
      sosList[sosIndex].assignedStationId = undefined;
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

