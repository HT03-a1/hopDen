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
  type: 'user' | 'medical' | 'rescue'; // Đã gộp repair vào rescue
  openHours?: string;
  description?: string;
  ratingAvg?: number;
  ratingCount?: number;
  lastLocationSource?: 'hardware' | 'mobile';
  lastLocationUpdatedAt?: string;
  lastHardwareLocationAt?: string;
  isOnline?: boolean;
}

interface AuthState {
  token: string | null;
  profile: UserProfile | null;
  setAuth: (token: string, profile: UserProfile) => void;
  updateProfile: (updates: Partial<UserProfile>) => void;
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

export const useAuthStore = create<AuthState>((set, get) => ({
  token: initialState.token,
  profile: initialState.profile,
  setAuth: (token, profile) => {
    localStorage.setItem('auth-storage', JSON.stringify({ token, profile }));
    set({ token, profile });
  },
  updateProfile: (updates) => {
    const currentProfile = get().profile;
    if (currentProfile) {
      const updatedProfile = { ...currentProfile, ...updates };
      const token = get().token;
      localStorage.setItem('auth-storage', JSON.stringify({ token, profile: updatedProfile }));
      set({ profile: updatedProfile });
    }
  },
  logout: () => {
    localStorage.removeItem('auth-storage');
    set({ token: null, profile: null });
  },
}));

