import { Router, Request, Response } from 'express';
import { readJson, writeJson } from '../services/dataService';
import { Rating, Station } from '../types';

const router = Router();

// Create rating
router.post('/', (req: Request, res: Response) => {
  const { stationId, userId, sosId, rating, comment } = req.body;

  if (!stationId || !userId || !sosId || !rating) {
    return res.status(400).json({ error: 'Thiếu thông tin bắt buộc' });
  }

  if (rating < 1 || rating > 5) {
    return res.status(400).json({ error: 'Rating phải từ 1 đến 5' });
  }

  try {
    const ratings = readJson<Rating>('ratings.json');
    
    // Check if user already rated this SOS
    const existingRating = ratings.find(
      r => r.userId === userId && r.sosId === sosId
    );

    if (existingRating) {
      return res.status(400).json({ error: 'Bạn đã đánh giá SOS này rồi' });
    }

    const newRating: Rating = {
      id: `R${Date.now()}`,
      stationId,
      userId,
      sosId,
      rating,
      comment,
      createdAt: new Date().toISOString()
    };

    ratings.push(newRating);
    writeJson('ratings.json', ratings);

    // Update station rating average
    const stations = readJson<Station>('stations.json');
    const stationIndex = stations.findIndex(s => s.id === stationId);
    
    if (stationIndex !== -1) {
      const stationRatings = ratings.filter(r => r.stationId === stationId);
      const totalRating = stationRatings.reduce((sum, r) => sum + r.rating, 0);
      stations[stationIndex].ratingAvg = totalRating / stationRatings.length;
      stations[stationIndex].ratingCount = stationRatings.length;
      writeJson('stations.json', stations);
    }

    res.status(201).json(newRating);
  } catch (error) {
    console.error('Create rating error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

// Get ratings by station
router.get('/', (req: Request, res: Response) => {
  const { stationId } = req.query;

  try {
    let ratings = readJson<Rating>('ratings.json');

    if (stationId) {
      ratings = ratings.filter(r => r.stationId === stationId);
    }

    res.json(ratings);
  } catch (error) {
    console.error('Get ratings error:', error);
    res.status(500).json({ error: 'Lỗi server' });
  }
});

export default router;

