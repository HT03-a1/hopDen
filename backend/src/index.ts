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
import { initializeData, generateTaiKhoanFile } from './services/dataService';

const app = express();
const httpServer = createServer(app);
const io = new Server(httpServer, {
  cors: {
    origin: "http://localhost:5173",
    methods: ["GET", "POST"],
    credentials: true
  }
});

const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors({
  origin: "http://localhost:5173",
  credentials: true
}));
app.use(express.json());

// Initialize data files on startup
initializeData();

// Generate taikhoan.md on startup (will update if data exists)
setTimeout(() => {
  generateTaiKhoanFile();
}, 1000);

// Socket.IO connection
io.on('connection', (socket) => {
  console.log('Client connected:', socket.id);

  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
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

