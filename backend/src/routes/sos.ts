import { Router, Request, Response } from 'express';
import { Server } from 'socket.io';
import { readJson, writeJson } from '../services/dataService';
import { SOS } from '../types';

const router = Router();

// Get Socket.IO instance
function getIO(req: Request): Server {
  return req.app.get('io');
}

// Create SOS
router.post('/', (req: Request, res: Response) => {
  const { userId, deviceId, type, severity, location, note } = req.body;

  if (!userId || !type || !severity || !location || !location.lat || !location.lon) {
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    
    const newSOS: SOS = {
      id: `SOS${Date.now()}`,
      userId,
      deviceId,
      type: type as 'accident' | 'breakdown' | 'medical' | 'other',
      severity: severity as 'low' | 'medium' | 'high' | 'critical',
      location: {
        lat: location.lat,
        lon: location.lon
      },
      status: 'pending',
      note,
      createdAt: new Date().toISOString(),
      updatedAt: new Date().toISOString()
    };

    sosList.push(newSOS);
    writeJson('sos.json', sosList);

    // Broadcast new SOS
    const io = getIO(req);
    io.emit('sos:new', newSOS);

    res.status(201).json(newSOS);
  } catch (error) {
    console.error('Create SOS error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get all SOS
router.get('/', (req: Request, res: Response) => {
  const { status, userId, stationId } = req.query;

  try {
    let sosList = readJson<SOS>('sos.json');

    if (status) {
      sosList = sosList.filter(sos => sos.status === status);
    }

    if (userId) {
      sosList = sosList.filter(sos => sos.userId === userId);
    }

    if (stationId) {
      sosList = sosList.filter(sos => sos.assignedStationId === stationId);
    }

    res.json(sosList);
  } catch (error) {
    console.error('Get SOS error:', error);
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

    if (sos.status !== 'pending') {
      console.error('SOS already claimed or processed:', sos.status);
      return res.status(400).json({ error: `SOS đã được nhận hoặc đã xử lý. Trạng thái hiện tại: ${sos.status}` });
    }

    sosList[sosIndex].assignedStationId = stationId;
    sosList[sosIndex].status = 'accepted';
    sosList[sosIndex].updatedAt = new Date().toISOString();

    writeJson('sos.json', sosList);
    console.log('SOS claimed successfully:', sosList[sosIndex]);

    // Broadcast update
    const io = getIO(req);
    io.emit('sos:update', sosList[sosIndex]);

    res.json(sosList[sosIndex]);
  } catch (error: any) {
    console.error('Claim SOS error:', error);
    console.error('Error stack:', error.stack);
    res.status(500).json({ error: `Lỗi server: ${error.message || 'Unknown error'}` });
  }
});

// Update SOS status
router.patch('/:id/status', (req: Request, res: Response) => {
  const { id } = req.params;
  const { status } = req.body;

  if (!status || !['accepted', 'on_route', 'done', 'cancelled'].includes(status)) {
    return res.status(400).json({ error: 'Status không hợp lệ' });
  }

  try {
    const sosList = readJson<SOS>('sos.json');
    const sosIndex = sosList.findIndex(s => s.id === id);

    if (sosIndex === -1) {
      return res.status(404).json({ error: 'Không tìm thấy SOS' });
    }

    sosList[sosIndex].status = status as 'accepted' | 'on_route' | 'done' | 'cancelled';
    sosList[sosIndex].updatedAt = new Date().toISOString();

    writeJson('sos.json', sosList);

    // Broadcast update
    const io = getIO(req);
    io.emit('sos:update', sosList[sosIndex]);

    res.json(sosList[sosIndex]);
  } catch (error) {
    console.error('Update SOS status error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

