import express from 'express';
import cors from 'cors';
import { createServer } from 'http';
import { Server } from 'socket.io';
import authRoutes from './routes/auth';
import mapRoutes from './routes/map';
import sosRoutes from './routes/sos';
import stationRoutes from './routes/stations';
import ratingRoutes from './routes/ratings';
import telemetryRoutes from './routes/telemetry';
import { initializeData } from './services/dataService';

const app = express();
const httpServer = createServer(app);

// CORS allowed origins
const allowedOrigins = [
  'https://hopdenthongminh.cloud',
  'https://www.hopdenthongminh.cloud',
  'http://localhost:5173',
  'http://localhost:3000',
];

const io = new Server(httpServer, {
  cors: {
    origin: function (origin, callback) {
      // Allow requests with no origin (mobile apps, Postman, etc.)
      if (!origin) return callback(null, true);
      if (allowedOrigins.indexOf(origin) !== -1) {
        callback(null, true);
      } else {
        callback(new Error('Not allowed by CORS'));
      }
    },
    methods: ["GET", "POST"],
    credentials: true
  },
  transports: ['polling', 'websocket'], // Hỗ trợ cả polling và websocket
  allowEIO3: true, // Tương thích với client cũ
  pingTimeout: 60000, // Tăng timeout cho Cloudflare Tunnel
  pingInterval: 25000
});

const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors({
  origin: function (origin, callback) {
    // Allow requests with no origin (mobile apps, Postman, etc.)
    if (!origin) return callback(null, true);
    if (allowedOrigins.indexOf(origin) !== -1) {
      callback(null, true);
    } else {
      callback(new Error('Not allowed by CORS'));
    }
  },
  credentials: true
}));
app.use(express.json());

// Initialize data files on startup (includes generateTaiKhoanFile)
initializeData();

// Socket.IO connection (reduced logging for production)
io.on('connection', (socket) => {
  // Only log in development
  if (process.env.NODE_ENV !== 'production') {
    console.log('Client connected:', socket.id);
  }

  socket.on('disconnect', () => {
    // Only log in development
    if (process.env.NODE_ENV !== 'production') {
      console.log('Client disconnected:', socket.id);
    }
  });
});

// Make io available to routes
app.set('io', io);

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

httpServer.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
});

