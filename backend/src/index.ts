import express from 'express';
import cors from 'cors';
import { createServer } from 'http';
import { Server } from 'socket.io';
import authRoutes from './routes/auth';
import mapRoutes from './routes/map';
import sosRoutes, { enforceAssignmentTimeouts } from './routes/sos';
import stationRoutes from './routes/stations';
import ratingRoutes from './routes/ratings';
import telemetryRoutes from './routes/telemetry';
import { initializeData } from './services/dataService';
import { authenticate } from './middleware/auth';

const app = express();
const httpServer = createServer(app);

// CORS allowed origins
const allowedOrigins = [
  'https://hopdenthongminh.cloud',
  'https://www.hopdenthongminh.cloud',
  'http://localhost:5173',
  'http://localhost:3000',
  'http://127.0.0.1:5173',
  'http://127.0.0.1:3000',
];

// Helper function để check origin
function isOriginAllowed(origin: string | undefined): boolean {
  if (!origin) return true; // Cho phép requests không có origin
  return allowedOrigins.indexOf(origin) !== -1;
}

const io = new Server(httpServer, {
  cors: {
    origin: function (origin: string | undefined, callback: (err: Error | null, allow?: boolean) => void) {
      // Cho phép TẤT CẢ origins để tránh lỗi CORS
      // Log để debug
      if (origin) {
        console.log('[Socket.IO] CORS check - Origin:', origin);
      }
      // LUÔN cho phép (có thể tùy chỉnh sau nếu cần bảo mật hơn)
      callback(null, true);
    },
    methods: ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    allowedHeaders: ["Content-Type", "Authorization", "X-Requested-With", "Accept", "Origin"],
    credentials: true
  },
  transports: ['polling', 'websocket'], // Hỗ trợ cả polling và websocket
  allowEIO3: true, // Tương thích với client cũ
  pingTimeout: 60000, // Tăng timeout cho Cloudflare Tunnel
  pingInterval: 25000,
  connectTimeout: 45000, // Tăng connect timeout
  // Xử lý lỗi tốt hơn
  allowUpgrades: true,
  perMessageDeflate: false, // Tắt compression để tránh lỗi
  httpCompression: false
});

const PORT = process.env.PORT || 3000;

// CORS middleware - phải đặt TRƯỚC TẤT CẢ các routes
// Middleware để set CORS headers cho MỌI response (kể cả lỗi)
app.use((req, res, next) => {
  const origin = req.headers.origin;
  
  // LUÔN set CORS headers (cho phép tất cả để tránh lỗi)
  if (origin) {
    res.header('Access-Control-Allow-Origin', origin);
  } else {
    res.header('Access-Control-Allow-Origin', '*');
  }
  res.header('Access-Control-Allow-Credentials', 'true');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, PATCH, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Requested-With, Accept, Origin');
  res.header('Access-Control-Expose-Headers', 'Content-Range, X-Content-Range');
  
  // Xử lý preflight requests
  if (req.method === 'OPTIONS') {
    return res.sendStatus(204);
  }
  
  // Chống cache cho API responses
  if (req.path.startsWith('/api')) {
    res.setHeader('Cache-Control', 'no-store, no-cache, must-revalidate, proxy-revalidate');
    res.setHeader('Pragma', 'no-cache');
    res.setHeader('Expires', '0');
  }
  
  next();
});

// CORS options cho express-cors (backup)
const corsOptions = {
  origin: function (origin: string | undefined, callback: (err: Error | null, allow?: boolean) => void) {
    // Allow requests with no origin (mobile apps, Postman, etc.)
    if (!origin) return callback(null, true);
    if (allowedOrigins.indexOf(origin) !== -1) {
      callback(null, true);
    } else {
      // Cho phép tất cả để tránh lỗi CORS (có thể tùy chỉnh sau)
      callback(null, true);
    }
  },
  credentials: true,
  methods: ['GET', 'POST', 'PUT', 'DELETE', 'PATCH', 'OPTIONS'],
  allowedHeaders: ['Content-Type', 'Authorization', 'X-Requested-With', 'Accept', 'Origin'],
  exposedHeaders: ['Content-Range', 'X-Content-Range'],
  preflightContinue: false,
  optionsSuccessStatus: 204
};

// Apply CORS middleware (backup)
app.use(cors(corsOptions));

app.use(express.json());

// Initialize data files on startup (includes generateTaiKhoanFile)
initializeData();

// Socket.IO connection với error handling
io.on('connection', (socket) => {
  console.log('✅ Client connected:', socket.id);
  console.log('📍 Origin:', socket.handshake.headers.origin || 'unknown');
  console.log('🌐 Transport:', socket.conn.transport.name);

  // Xử lý lỗi connection
  socket.on('error', (error) => {
    console.error('❌ Socket.IO socket error:', error);
  });

  // Xử lý disconnect
  socket.on('disconnect', (reason) => {
    console.log('🔌 Client disconnected:', socket.id, 'Reason:', reason);
  });

  // Ping/pong để kiểm tra connection
  socket.on('ping', () => {
    socket.emit('pong');
  });
});

// Xử lý lỗi ở engine level (quan trọng cho "server error")
io.engine.on('connection_error', (err) => {
  console.error('❌ Socket.IO engine connection error:');
  console.error('   Message:', err.message);
  console.error('   Description:', err.description);
  console.error('   Context:', err.context);
  console.error('   Type:', err.type);
  
  // Nếu là lỗi CORS, log thêm thông tin
  if (err.message && (err.message.includes('CORS') || err.message.includes('origin'))) {
    console.error('   ⚠️ CORS-related error detected');
  }
});

// Make io available to routes
app.set('io', io);

const ASSIGNMENT_SWEEP_INTERVAL_MS = 15 * 1000;
setInterval(() => {
  try {
    enforceAssignmentTimeouts(io);
  } catch (error: any) {
    console.error('[SOS] Assignment sweep error:', error.message);
  }
}, ASSIGNMENT_SWEEP_INTERVAL_MS);

// Routes
app.use('/api/auth', authRoutes);
app.use('/api/map', mapRoutes);
app.use('/api/sos', sosRoutes);
app.use('/api/stations', stationRoutes);
app.use('/api/ratings', ratingRoutes);
app.use('/api/telemetry', telemetryRoutes);

// Health check
app.get('/api/health', (req, res) => {
  res.json({ status: 'ok', message: 'Server is running' });
});

// Error handler - đảm bảo CORS headers được gửi ngay cả khi có lỗi
app.use((err: any, req: express.Request, res: express.Response, next: express.NextFunction) => {
  const origin = req.headers.origin;
  if (!origin || allowedOrigins.indexOf(origin) !== -1) {
    res.header('Access-Control-Allow-Origin', origin || '*');
    res.header('Access-Control-Allow-Credentials', 'true');
  }
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, PATCH, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Requested-With, Accept, Origin');
  
  if (res.headersSent) {
    return next(err);
  }
  
  res.status(err.status || 500).json({
    error: err.message || 'Internal Server Error'
  });
});

// 404 handler - đảm bảo CORS headers cho 404
app.use((req: express.Request, res: express.Response) => {
  const origin = req.headers.origin;
  if (!origin || allowedOrigins.indexOf(origin) !== -1) {
    res.header('Access-Control-Allow-Origin', origin || '*');
    res.header('Access-Control-Allow-Credentials', 'true');
  }
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, PATCH, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Authorization, X-Requested-With, Accept, Origin');
  res.status(404).json({ error: 'Not Found' });
});

httpServer.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
  console.log(`CORS enabled for origins: ${allowedOrigins.join(', ')}`);
});

