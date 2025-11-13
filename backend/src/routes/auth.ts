import { Router, Request, Response } from 'express';
import { readJson, writeJson } from '../services/dataService';
import { User, Station } from '../types';

const router = Router();

// Login
router.post('/login', (req: Request, res: Response) => {
  const { email, password, role } = req.body;

  if (!email || !password || !role) {
    return res.status(400).json({ error: 'Email, password và role là bắt buộc' });
  }

  try {
    if (role === 'user') {
      const users = readJson<User>('users.json');
      const user = users.find(u => u.email === email && u.password === password);
      
      if (!user) {
        return res.status(401).json({ error: 'Email hoặc mật khẩu không đúng' });
      }

      // Mock token
      const token = `mock_token_${user.id}_${Date.now()}`;
      
      return res.json({
        token,
        profile: {
          id: user.id,
          name: user.name,
          email: user.email,
          phone: user.phone,
          address: user.address,
          lat: user.lat,
          lon: user.lon,
          type: 'user'
        }
      });
    } else if (role === 'medical_station' || role === 'rescue_station') {
      const stations = readJson<Station>('stations.json');
      const stationType = role === 'medical_station' ? 'medical' : 'rescue'; // Đã gộp repair vào rescue
      const station = stations.find(
        s => s.email === email && s.password === password && s.type === stationType
      );

      if (!station) {
        return res.status(401).json({ error: 'Email hoặc mật khẩu không đúng' });
      }

      const token = `mock_token_${station.id}_${Date.now()}`;

      return res.json({
        token,
        profile: {
          id: station.id,
          stationName: station.stationName,
          email: station.email,
          phone: station.phone,
          address: station.address,
          lat: station.lat,
          lon: station.lon,
          type: station.type,
          openHours: station.openHours,
          description: station.description,
          ratingAvg: station.ratingAvg,
          ratingCount: station.ratingCount
        }
      });
    } else {
      return res.status(400).json({ error: 'Role không hợp lệ' });
    }
  } catch (error) {
    console.error('Login error:', error);
    return res.status(500).json({ error: 'Lỗi server' });
  }
});

// Register User
router.post('/register-user', (req: Request, res: Response) => {
  const { name, email, password, phone, address, lat, lon } = req.body;

  if (!name || !email || !password || !phone) {
    return res.status(400).json({ error: 'Vui lòng điền đầy đủ thông tin' });
  }

  try {
    const users = readJson<User>('users.json');
    
    // Check if email already exists
    if (users.some(u => u.email === email)) {
      return res.status(400).json({ error: 'Email đã tồn tại' });
    }

    // Generate new user ID
    const maxId = users.reduce((max, user) => {
      const num = parseInt(user.id.replace('U', ''));
      return num > max ? num : max;
    }, 0);
    const newId = `U${String(maxId + 1).padStart(4, '0')}`;

    const newUser: User = {
      id: newId,
      name,
      email,
      password, // In production, should hash this
      phone,
      address: address || '', // Optional address
      lat: lat || 10.7769, // Default to Ho Chi Minh City if not provided
      lon: lon || 106.7009,
      createdAt: new Date().toISOString()
    };

    users.push(newUser);
    writeJson('users.json', users);

    const token = `mock_token_${newUser.id}_${Date.now()}`;

    return res.status(201).json({
      token,
      profile: {
        id: newUser.id,
        name: newUser.name,
        email: newUser.email,
        phone: newUser.phone,
        address: newUser.address,
        lat: newUser.lat,
        lon: newUser.lon,
        type: 'user'
      }
    });
  } catch (error) {
    console.error('Register user error:', error);
    return res.status(500).json({ error: 'Lỗi server' });
  }
});

// Register Station
router.post('/register-station', (req: Request, res: Response) => {
  const { stationName, email, password, phone, address, lat, lon, openHours, type, description } = req.body;

  if (!stationName || !email || !password || !phone || !address || lat === undefined || lon === undefined || !type) {
    return res.status(400).json({ error: 'Vui lòng điền đầy đủ thông tin' });
  }

  if (!['medical', 'rescue'].includes(type)) {
    return res.status(400).json({ error: 'Loại trạm không hợp lệ' });
  }

  try {
    const stations = readJson<Station>('stations.json');
    
    if (stations.some(s => s.email === email)) {
      return res.status(400).json({ error: 'Email đã tồn tại' });
    }

    const maxId = stations.reduce((max, station) => {
      const num = parseInt(station.id.replace('S', ''));
      return num > max ? num : max;
    }, 0);
    const newId = `S${String(maxId + 1).padStart(4, '0')}`;

    const newStation: Station = {
      id: newId,
      stationName,
      type: type as 'medical' | 'rescue', // Đã gộp repair vào rescue
      email,
      password,
      phone,
      address,
      lat,
      lon,
      openHours: openHours || '24/7',
      description: description || '',
      ratingAvg: 0,
      ratingCount: 0,
      createdAt: new Date().toISOString()
    };

    stations.push(newStation);
    writeJson('stations.json', stations);

    const token = `mock_token_${newStation.id}_${Date.now()}`;

    return res.status(201).json({
      token,
      profile: {
        id: newStation.id,
        stationName: newStation.stationName,
        email: newStation.email,
        phone: newStation.phone,
        address: newStation.address,
        lat: newStation.lat,
        lon: newStation.lon,
        type: newStation.type,
        openHours: newStation.openHours,
        description: newStation.description,
        ratingAvg: newStation.ratingAvg,
        ratingCount: newStation.ratingCount
      }
    });
  } catch (error) {
    console.error('Register station error:', error);
    return res.status(500).json({ error: 'Lỗi server' });
  }
});

// Update user location
router.patch('/users/:id/location', (req: Request, res: Response) => {
  const { id } = req.params;
  const { lat, lon } = req.body;

  if (lat === undefined || lon === undefined) {
    return res.status(400).json({ error: 'Thiếu thông tin vị trí' });
  }

  const latNum = parseFloat(lat);
  const lonNum = parseFloat(lon);

  if (isNaN(latNum) || isNaN(lonNum)) {
    return res.status(400).json({ error: 'Tọa độ không hợp lệ' });
  }

  if (latNum < -90 || latNum > 90 || lonNum < -180 || lonNum > 180) {
    return res.status(400).json({ error: 'Tọa độ nằm ngoài phạm vi hợp lệ' });
  }

  try {
    const users = readJson<User>('users.json');
    const userIndex = users.findIndex(u => u.id === id);

    if (userIndex === -1) {
      return res.status(404).json({ error: 'Không tìm thấy người dùng' });
    }

    users[userIndex].lat = latNum;
    users[userIndex].lon = lonNum;
    writeJson('users.json', users);

    return res.json({
      id: users[userIndex].id,
      name: users[userIndex].name,
      email: users[userIndex].email,
      phone: users[userIndex].phone,
      address: users[userIndex].address,
      lat: users[userIndex].lat,
      lon: users[userIndex].lon,
      type: 'user'
    });
  } catch (error) {
    console.error('Update user location error:', error);
    return res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

