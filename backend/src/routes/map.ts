import { Router, Request, Response } from 'express';
import { readJson } from '../services/dataService';
import { User, Station, Device, SOS, MapEntity } from '../types';
import { authenticate } from '../middleware/auth';

const router = Router();

// Get all map entities
router.get('/entities', authenticate, (req: Request, res: Response) => {
  try {
    // Log để debug authentication
    if (process.env.NODE_ENV !== 'production') {
      console.log(`[MAP] Request from: userId=${req.userId}, userType=${req.userType}`);
    }
    
    const entities: MapEntity[] = [];

    // Load SOS list trước để kiểm tra users có SOS active (cho station type)
    let activeSOSUserIds = new Set<string>();
    if (req.userType === 'medical' || req.userType === 'rescue') {
      try {
        const sosList = readJson<SOS>('sos.json');
        if (Array.isArray(sosList)) {
          // Lọc SOS active (pending, accepted, on_route) và phù hợp với loại trạm
          const allowedTypes = req.userType === 'medical' 
            ? ['medical', 'accident'] 
            : ['breakdown', 'other'];
          
          sosList
            .filter(sos => 
              sos && 
              sos.status && 
              sos.status !== 'done' && 
              sos.status !== 'cancelled' &&
              allowedTypes.includes(sos.type)
            )
            .forEach(sos => {
              if (sos.userId) {
                activeSOSUserIds.add(sos.userId);
              }
            });
          
          // Log để debug
          if (process.env.NODE_ENV !== 'production') {
            console.log(`[MAP] Station ${req.userId} (${req.userType}): Active SOS user IDs:`, Array.from(activeSOSUserIds));
          }
        }
      } catch (error: any) {
        console.error('Error loading SOS for privacy check:', error.message);
      }
    }

    // Get users - CHỈ hiển thị user hiện tại nếu là user type
    // CHỈ hiển thị users có SOS active nếu là station type
    try {
      const users = readJson<User>('users.json');
      if (Array.isArray(users)) {
        users.forEach(user => {
          if (user && user.id && user.lat && user.lon) {
            // Nếu là user type và có userId trong request, chỉ hiển thị user của mình
            if (req.userType === 'user' && req.userId) {
              if (user.id !== req.userId) {
                return; // Bỏ qua user khác
              }
            }
            
            // Nếu là station type, CHỈ hiển thị users có SOS active (bảo vệ quyền riêng tư)
            if (req.userType === 'medical' || req.userType === 'rescue') {
              // BẮT BUỘC phải có userId và user phải có SOS active
              if (!req.userId) {
                // Không có userId, không hiển thị user
                return;
              }
              if (!activeSOSUserIds.has(user.id)) {
                // Log để debug
                if (process.env.NODE_ENV !== 'production') {
                  console.log(`[MAP] Station ${req.userId}: Hiding user ${user.id} (no active SOS)`);
                }
                return; // Bỏ qua users không có SOS active
              }
              // Log để debug
              if (process.env.NODE_ENV !== 'production') {
                console.log(`[MAP] Station ${req.userId}: Showing user ${user.id} (has active SOS)`);
              }
            }
            
            // Nếu không có authentication HOẶC không phải user/station type, không hiển thị user (bảo vệ quyền riêng tư)
            if (!req.userType) {
              // Không có authentication, không hiển thị users (chỉ hiển thị stations, SOS, devices)
              return;
            }
            
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
    // CHỈ hiển thị SOS của chính user nếu là user type
    // Hiển thị tất cả SOS active nếu là station type (để trạm có thể xử lý)
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
        // Log tổng số SOS để debug
        if (process.env.NODE_ENV !== 'production') {
          console.log(`[MAP] Total SOS in system: ${sosList.length}`);
          const activeSOS = sosList.filter(sos => sos && sos.status && sos.status !== 'done' && sos.status !== 'cancelled' && sos.location);
          console.log(`[MAP] Active SOS (not done/cancelled, has location): ${activeSOS.length}`);
        }
        
        sosList
          .filter(sos => {
            // Filter SOS active (pending, accepted, on_route) và có location
            const isActive = sos && sos.status && sos.status !== 'done' && sos.status !== 'cancelled';
            const hasLocation = sos.location && sos.location.lat && sos.location.lon;
            
            // Log để debug filter
            if (process.env.NODE_ENV !== 'production' && sos) {
              if (!isActive) {
                console.log(`[MAP] Filtering out SOS ${sos.id}: status=${sos.status} (not active)`);
              } else if (!hasLocation) {
                console.log(`[MAP] Filtering out SOS ${sos.id}: no location`);
              }
            }
            
            return isActive && hasLocation;
          })
          .forEach(sos => {
            if (sos.location && sos.location.lat && sos.location.lon) {
              // Log để debug
              if (process.env.NODE_ENV !== 'production') {
                console.log(`[MAP] Processing SOS ${sos.id}: userId=${sos.userId}, type=${sos.type}, status=${sos.status}, req.userId=${req.userId}, req.userType=${req.userType}`);
              }
              
              // Nếu là user type, CHỈ hiển thị SOS của chính user đó
              if (req.userType === 'user' && req.userId) {
                if (sos.userId !== req.userId) {
                  // Bỏ qua SOS của user khác
                  if (process.env.NODE_ENV !== 'production') {
                    console.log(`[MAP] User ${req.userId}: Hiding SOS ${sos.id} (belongs to user ${sos.userId})`);
                  }
                  return; // Bỏ qua SOS của user khác
                }
                // Log khi hiển thị SOS của chính user - QUAN TRỌNG: Phải hiển thị SOS của chính mình
                if (process.env.NODE_ENV !== 'production') {
                  console.log(`[MAP] ✅ User ${req.userId}: Showing SOS ${sos.id} (type ${sos.type}, status ${sos.status}, location=(${sos.location.lat}, ${sos.location.lon}))`);
                }
                // Tiếp tục để thêm SOS vào entities (KHÔNG return ở đây)
              }
              
              // Nếu là station type, chỉ hiển thị SOS phù hợp với loại trạm
              if (req.userType === 'medical' || req.userType === 'rescue') {
                const allowedTypes = req.userType === 'medical' 
                  ? ['medical', 'accident'] 
                  : ['breakdown', 'other'];
                
                if (!allowedTypes.includes(sos.type)) {
                  // Bỏ qua SOS không phù hợp với loại trạm
                  if (process.env.NODE_ENV !== 'production') {
                    console.log(`[MAP] Station ${req.userId} (${req.userType}): Hiding SOS ${sos.id} (type ${sos.type} not allowed)`);
                  }
                  return;
                }
                // Log khi hiển thị SOS cho trạm
                if (process.env.NODE_ENV !== 'production') {
                  console.log(`[MAP] Station ${req.userId} (${req.userType}): Showing SOS ${sos.id} (type ${sos.type}, status ${sos.status})`);
                }
              }
              
              // Nếu không có authentication, không hiển thị SOS (bảo vệ quyền riêng tư)
              // QUAN TRỌNG: Chỉ check này nếu KHÔNG phải user type và KHÔNG phải station type
              // Vì nếu đã vào block user type hoặc station type ở trên thì đã có authentication
              // Nếu req.userType là undefined/null, nghĩa là không có authentication
              if (!req.userType) {
                if (process.env.NODE_ENV !== 'production') {
                  console.log(`[MAP] No authentication: Hiding SOS ${sos.id}`);
                }
                return; // Không có authentication, không hiển thị SOS
              }
              
              // Lấy thông tin user từ userMap
              const user = userMap.get(sos.userId);
              
              // Log chi tiết khi thêm SOS vào entities
              if (process.env.NODE_ENV !== 'production') {
                console.log(`[MAP] ✅ Adding SOS entity: id=${sos.id}, userId=${sos.userId}, type=${sos.type}, status=${sos.status}, location=(${sos.location.lat}, ${sos.location.lon})`);
              }
              
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

    // Get devices (optional) - CHỈ hiển thị devices KHÔNG có user tương ứng
    // Nếu device có userId, LUÔN bỏ qua device marker (chỉ hiển thị user marker)
    try {
      const devices = readJson<Device>('devices.json');
      
      if (Array.isArray(devices)) {
        devices.forEach(device => {
          if (device && device.id && device.lat && device.lon) {
            // Nếu device có userId, LUÔN bỏ qua device marker (user sẽ có marker riêng)
            if (device.userId) {
              // Device thuộc về user, không hiển thị device marker
              if (process.env.NODE_ENV !== 'production') {
                console.log(`[MAP] Skipping device ${device.id} - belongs to user ${device.userId}, user marker will be shown instead`);
              }
              return;
            }
            
            // Chỉ hiển thị device nếu KHÔNG có userId (device độc lập, không thuộc về user nào)
            
            // Áp dụng filter tương tự như users (cho user type và station type)
            // Nhưng vì device không có userId, nên không cần filter theo user
            
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

    // Log tổng số entities trước khi trả về
    if (process.env.NODE_ENV !== 'production') {
      const sosEntities = entities.filter(e => e.type === 'sos');
      const userEntities = entities.filter(e => e.type === 'user');
      const deviceEntities = entities.filter(e => e.type === 'device');
      console.log(`[MAP] Returning entities: total=${entities.length}, sos=${sosEntities.length}, user=${userEntities.length}, device=${deviceEntities.length}`);
      if (sosEntities.length > 0) {
        console.log(`[MAP] SOS entities being returned:`, sosEntities.map(e => ({ id: e.id, userId: e.userId, type: e.sosType, lat: e.lat, lon: e.lon })));
      }
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

