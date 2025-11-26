import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    host: '0.0.0.0', // Cho phép truy cập từ bên ngoài
    allowedHosts: [
      'hopdenthongminh.cloud',
      'www.hopdenthongminh.cloud',
      'localhost',
      '127.0.0.1'
    ],
    // Cấu hình HMR (Hot Module Replacement) - chỉ trong development
    // Trong production, HMR sẽ tự động tắt
    hmr: {
      clientPort: 5173,
      protocol: 'ws',
      // Chỉ kết nối HMR trong development (localhost)
      // Trong production build, HMR sẽ không hoạt động
    },
    // Tự động mở browser
    open: false,
    // Watch files để tự động reload
    watch: {
      usePolling: false, // Dùng native file watching (nhanh hơn)
      interval: 100, // Nếu dùng polling, check mỗi 100ms
    },
    proxy: {
      '/api': {
        target: 'http://localhost:3000',
        changeOrigin: true,
        ws: true, // Hỗ trợ WebSocket cho proxy
      }
    }
  },
  // Cấu hình build
  build: {
    // Tăng chunk size warning limit
    chunkSizeWarningLimit: 1000,
    // Source maps cho development
    sourcemap: true,
  },
  // Optimize dependencies
  optimizeDeps: {
    include: ['react', 'react-dom', 'react-router-dom', 'axios', 'socket.io-client'],
  },
})

