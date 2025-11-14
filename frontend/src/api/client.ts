import axios from 'axios';

const API_BASE_URL = import.meta.env.VITE_API_URL || 'http://localhost:3000/api';

export const apiClient = axios.create({
  baseURL: API_BASE_URL,
  headers: {
    'Content-Type': 'application/json',
  },
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
});

export default apiClient;

