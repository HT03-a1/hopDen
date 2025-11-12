import { create } from 'zustand';

interface UserProfile {
  id: string;
  name?: string;
  stationName?: string;
  email: string;
  phone?: string;
  address?: string;
  lat?: number;
  lon?: number;
  type: 'user' | 'medical' | 'rescue' | 'repair';
  openHours?: string;
  description?: string;
  ratingAvg?: number;
  ratingCount?: number;
}

interface AuthState {
  token: string | null;
  profile: UserProfile | null;
  setAuth: (token: string, profile: UserProfile) => void;
  logout: () => void;
}

// Load from localStorage on init
const loadAuth = () => {
  try {
    const stored = localStorage.getItem('auth-storage');
    if (stored) {
      const parsed = JSON.parse(stored);
      return { token: parsed.token || null, profile: parsed.profile || null };
    }
  } catch (e) {
    // Ignore
  }
  return { token: null, profile: null };
};

const initialState = loadAuth();

export const useAuthStore = create<AuthState>((set) => ({
  token: initialState.token,
  profile: initialState.profile,
  setAuth: (token, profile) => {
    localStorage.setItem('auth-storage', JSON.stringify({ token, profile }));
    set({ token, profile });
  },
  logout: () => {
    localStorage.removeItem('auth-storage');
    set({ token: null, profile: null });
  },
}));

