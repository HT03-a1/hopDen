export interface User {
  id: string;
  name: string;
  email: string;
  password: string;
  phone: string;
  address: string;
  lat: number;
  lon: number;
  createdAt: string;
}

export interface Station {
  id: string;
  stationName: string;
  type: 'medical' | 'rescue' | 'repair';
  email: string;
  password: string;
  phone: string;
  address: string;
  lat: number;
  lon: number;
  openHours: string;
  description: string;
  ratingAvg: number;
  ratingCount: number;
  createdAt: string;
}

export interface Device {
  id: string;
  userId: string;
  deviceName: string;
  vehicleType: string;
  lat: number;
  lon: number;
  status: 'active' | 'inactive' | 'sos';
  lastUpdate: string;
}

export interface SOS {
  id: string;
  userId: string;
  deviceId?: string;
  type: 'accident' | 'breakdown' | 'medical' | 'other';
  severity: 'low' | 'medium' | 'high' | 'critical';
  location: {
    lat: number;
    lon: number;
  };
  status: 'pending' | 'accepted' | 'on_route' | 'done' | 'cancelled';
  assignedStationId?: string;
  note?: string;
  createdAt: string;
  updatedAt: string;
}

export interface Rating {
  id: string;
  stationId: string;
  userId: string;
  sosId: string;
  rating: number; // 1-5
  comment?: string;
  createdAt: string;
}

export interface Telemetry {
  id: string;
  deviceId: string;
  userId: string;
  lat: number;
  lon: number;
  speed?: number;
  timestamp: string;
}

export interface MapEntity {
  type: 'user' | 'medical_station' | 'rescue_station' | 'repair_station' | 'device' | 'sos';
  id: string;
  name: string;
  lat: number;
  lon: number;
  [key: string]: any;
}

