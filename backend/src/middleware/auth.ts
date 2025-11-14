import { Request, Response, NextFunction } from 'express';
import { readJson } from '../services/dataService';
import { User, Station } from '../types';

// Extend Request interface to include user info
declare global {
  namespace Express {
    interface Request {
      userId?: string;
      userType?: 'user' | 'medical' | 'rescue';
    }
  }
}

// Middleware to authenticate and extract user info from token
export const authenticate = (req: Request, res: Response, next: NextFunction) => {
  try {
    const authHeader = req.headers.authorization;
    
    // Log để debug
    if (process.env.NODE_ENV !== 'production') {
      console.log(`[AUTH] Request to ${req.path}:`, {
        hasAuthHeader: !!authHeader,
        authHeader: authHeader ? authHeader.substring(0, 20) + '...' : 'none'
      });
    }
    
    if (!authHeader || !authHeader.startsWith('Bearer ')) {
      // Không có token - cho phép tiếp tục nhưng không có user info
      if (process.env.NODE_ENV !== 'production') {
        console.log(`[AUTH] No authorization header or not Bearer token`);
      }
      return next();
    }

    const token = authHeader.substring(7); // Remove 'Bearer ' prefix
    
    // Log token để debug
    if (process.env.NODE_ENV !== 'production') {
      console.log(`[AUTH] Token received: ${token.substring(0, 30)}...`);
    }
    
    // Parse token format: mock_token_${id}_${timestamp}
    const tokenParts = token.split('_');
    if (tokenParts.length < 3 || tokenParts[0] !== 'mock' || tokenParts[1] !== 'token') {
      // Token không đúng format - cho phép tiếp tục nhưng không có user info
      if (process.env.NODE_ENV !== 'production') {
        console.log(`[AUTH] Token format invalid:`, tokenParts);
      }
      return next();
    }

    const userId = tokenParts[2]; // User ID hoặc Station ID
    
    // Log userId để debug
    if (process.env.NODE_ENV !== 'production') {
      console.log(`[AUTH] Extracted userId: ${userId}`);
    }
    
    // Kiểm tra xem là user hay station
    const users = readJson<User>('users.json');
    const stations = readJson<Station>('stations.json');
    
    const user = users.find(u => u.id === userId);
    if (user) {
      req.userId = userId;
      req.userType = 'user';
      if (process.env.NODE_ENV !== 'production') {
        console.log(`[AUTH] ✅ Authenticated as user: ${userId}`);
      }
      return next();
    }
    
    const station = stations.find(s => s.id === userId);
    if (station) {
      req.userId = userId;
      req.userType = station.type; // 'medical' hoặc 'rescue'
      if (process.env.NODE_ENV !== 'production') {
        console.log(`[AUTH] ✅ Authenticated as station: ${userId} (${station.type})`);
      }
      return next();
    }
    
    // Không tìm thấy user/station - cho phép tiếp tục nhưng không có user info
    if (process.env.NODE_ENV !== 'production') {
      console.log(`[AUTH] ⚠️ User/Station not found: ${userId}`);
    }
    next();
  } catch (error) {
    // Lỗi khi parse token - cho phép tiếp tục nhưng không có user info
    if (process.env.NODE_ENV !== 'production') {
      console.error(`[AUTH] Error in authenticate middleware:`, error);
    }
    next();
  }
};

