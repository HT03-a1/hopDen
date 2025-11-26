import axios from 'axios';

// Tự động detect API URL dựa trên environment
function getApiBaseUrl(): string {
  // Nếu có VITE_API_URL từ env, dùng nó
  if (import.meta.env.VITE_API_URL) {
    return import.meta.env.VITE_API_URL;
  }
  
  // Nếu đang chạy trên production domain
  if (typeof window !== 'undefined') {
    const hostname = window.location.hostname;
    
    // Production domain
    if (hostname === 'hopdenthongminh.cloud' || hostname === 'www.hopdenthongminh.cloud') {
      return 'https://api.hopdenthongminh.cloud/api';
    }
    
    // Local development
    if (hostname === 'localhost' || hostname === '127.0.0.1') {
      return 'http://localhost:3000/api';
    }
  }
  
  // Default fallback
  return 'http://localhost:3000/api';
}

const API_BASE_URL = getApiBaseUrl();

if (import.meta.env.DEV) {
  console.log('[apiClient] 🔍 API Base URL:', API_BASE_URL);
  console.log('[apiClient] 🔍 Current hostname:', typeof window !== 'undefined' ? window.location.hostname : 'N/A');
}

export const apiClient = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json',
  },
  withCredentials: true, // Gửi cookies nếu cần (cho CORS credentials)
  timeout: 30000, // 30 seconds timeout
});

// Add token to requests
apiClient.interceptors.request.use((config) => {
  const tokenData = localStorage.getItem('auth-storage');
  if (tokenData) {
    try {
      const parsed = JSON.parse(tokenData);
      // authStore lưu token trực tiếp: { token: "...", profile: {...} }
      // Hoặc có thể là Zustand format: { state: { token: "...", profile: {...} } }
      let token: string | null = null;
      
      if (parsed.token) {
        // Format trực tiếp từ authStore
        token = parsed.token;
      } else if (parsed.state?.token) {
        // Format Zustand
        token = parsed.state.token;
      }
      
      if (token) {
        config.headers.Authorization = `Bearer ${token}`;
        // Log để debug
        if (import.meta.env.DEV) {
          console.log('[apiClient] ✅ Adding token to request:', config.url, 'Token:', token.substring(0, 30) + '...');
        }
      } else {
        if (import.meta.env.DEV) {
          console.warn('[apiClient] ⚠️ No token found in auth-storage:', parsed);
        }
      }
    } catch (e) {
      if (import.meta.env.DEV) {
        console.error('[apiClient] ❌ Error parsing auth-storage:', e);
      }
    }
  } else {
    if (import.meta.env.DEV) {
      console.warn('[apiClient] ⚠️ No auth-storage in localStorage');
    }
  }
  return config;
}, (error) => {
  return Promise.reject(error);
});

// Error interceptor để xử lý CORS và network errors
apiClient.interceptors.response.use(
  (response) => response,
  (error) => {
    if (error.code === 'ERR_NETWORK' || error.message === 'Network Error') {
      console.error('[apiClient] ❌ Network Error - Có thể do CORS hoặc server không chạy');
      console.error('[apiClient] 🔍 API URL:', API_BASE_URL);
      console.error('[apiClient] 🔍 Error details:', error);
      
      // Nếu là CORS error, thử fallback
      if (typeof window !== 'undefined' && window.location.hostname.includes('hopdenthongminh.cloud')) {
        console.warn('[apiClient] ⚠️ CORS error detected. Đảm bảo backend đã cấu hình CORS đúng.');
      }
    } else if (error.response) {
      // Server responded with error status
      if (error.response.status === 502) {
        console.error('[apiClient] ❌ 502 Bad Gateway - Backend server có thể không chạy hoặc Cloudflare Tunnel có vấn đề');
      } else if (error.response.status === 403) {
        console.error('[apiClient] ❌ 403 Forbidden - Có thể do CORS policy');
      }
    }
    
    return Promise.reject(error);
  }
);

export default apiClient;

