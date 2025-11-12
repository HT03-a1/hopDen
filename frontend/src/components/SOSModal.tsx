import { useState, useEffect } from 'react';
import apiClient from '../api/client';

interface SOSModalProps {
  userId: string;
  userLocation?: { lat: number; lon: number };
  onClose: () => void;
  onSuccess: () => void;
}

export default function SOSModal({ userId, userLocation, onClose, onSuccess }: SOSModalProps) {
  const [type, setType] = useState<'accident' | 'breakdown' | 'medical' | 'other'>('accident');
  const [severity, setSeverity] = useState<'low' | 'medium' | 'high' | 'critical'>('medium');
  const [note, setNote] = useState('');
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState('');
  const [showFields, setShowFields] = useState(false);
  const [hasSelectedType, setHasSelectedType] = useState(false);
  const [hasSelectedSeverity, setHasSelectedSeverity] = useState(false);
  const [currentLocation, setCurrentLocation] = useState<{ lat: number; lon: number } | null>(null);
  const [locationError, setLocationError] = useState('');
  const [gettingLocation, setGettingLocation] = useState(false);

  // Lấy vị trí hiện tại từ thiết bị
  useEffect(() => {
    if (!navigator.geolocation) {
      setLocationError('Trình duyệt không hỗ trợ Geolocation');
      // Fallback to default location
      setCurrentLocation({ lat: 10.7769, lon: 106.7009 });
      return;
    }

    setGettingLocation(true);
    navigator.geolocation.getCurrentPosition(
      (position) => {
        setCurrentLocation({
          lat: position.coords.latitude,
          lon: position.coords.longitude,
        });
        setLocationError('');
        setGettingLocation(false);
      },
      (error) => {
        console.error('Geolocation error:', error);
        setLocationError('Không thể lấy vị trí. Sử dụng vị trí mặc định.');
        // Fallback to default location or userLocation if available
        setCurrentLocation(userLocation || { lat: 10.7769, lon: 106.7009 });
        setGettingLocation(false);
      },
      {
        enableHighAccuracy: true,
        timeout: 10000,
        maximumAge: 0,
      }
    );
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Hiển thị các trường chọn sau 0.3s
  useEffect(() => {
    const timer = setTimeout(() => {
      setShowFields(true);
    }, 300);
    return () => clearTimeout(timer);
  }, []);

  // Tự động gửi sau khi đã chọn cả loại và mức độ
  useEffect(() => {
    if (hasSelectedType && hasSelectedSeverity && !loading) {
      const timer = setTimeout(() => {
        handleAutoSubmit();
      }, 500);
      return () => clearTimeout(timer);
    }
  }, [hasSelectedType, hasSelectedSeverity]);

  const handleAutoSubmit = async () => {
    if (loading) return;
    
    // Wait for location if still getting it
    if (gettingLocation) {
      setTimeout(() => handleAutoSubmit(), 500);
      return;
    }

    const location = currentLocation || userLocation || { lat: 10.7769, lon: 106.7009 };
    
    setError('');
    setLoading(true);

    try {
      await apiClient.post('/sos', {
        userId,
        type,
        severity,
        location: location,
        note,
      });
      onSuccess();
      onClose();
    } catch (err: any) {
      setError(err.response?.data?.error || 'Gửi SOS thất bại');
      setLoading(false);
      setHasSelectedType(false);
      setHasSelectedSeverity(false);
    }
  };

  const handleTypeChange = (newType: 'accident' | 'breakdown' | 'medical' | 'other') => {
    setType(newType);
    setHasSelectedType(true);
  };

  const handleSeverityChange = (newSeverity: 'low' | 'medium' | 'high' | 'critical') => {
    setSeverity(newSeverity);
    setHasSelectedSeverity(true);
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setLoading(true);

    // Wait for location if still getting it
    if (gettingLocation) {
      setTimeout(() => handleSubmit(e), 500);
      return;
    }

    const location = currentLocation || userLocation || { lat: 10.7769, lon: 106.7009 };

    try {
      await apiClient.post('/sos', {
        userId,
        type,
        severity,
        location: location,
        note,
      });
      onSuccess();
      onClose();
    } catch (err: any) {
      setError(err.response?.data?.error || 'Gửi SOS thất bại');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="pointer-events-auto">
      <div className="bg-white rounded-lg p-6 w-full max-w-md shadow-2xl">
        <h2 className="text-2xl font-bold mb-4">Gửi SOS</h2>

        {error && (
          <div className="bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded mb-4">
            {error}
          </div>
        )}

        {gettingLocation && (
          <div className="bg-yellow-100 border border-yellow-400 text-yellow-700 px-4 py-3 rounded mb-4">
            📍 Đang lấy vị trí hiện tại từ thiết bị...
          </div>
        )}
        {locationError && (
          <div className="bg-orange-100 border border-orange-400 text-orange-700 px-4 py-3 rounded mb-4 text-sm">
            ⚠️ {locationError}
          </div>
        )}
        {currentLocation && !gettingLocation && (
          <div className="bg-green-100 border border-green-400 text-green-700 px-4 py-3 rounded mb-4 text-sm">
            ✅ Đã lấy vị trí: {currentLocation.lat.toFixed(6)}, {currentLocation.lon.toFixed(6)}
          </div>
        )}
        {loading && (
          <div className="bg-blue-100 border border-blue-400 text-blue-700 px-4 py-3 rounded mb-4">
            Đang gửi SOS...
          </div>
        )}

        <form onSubmit={handleSubmit} className="space-y-4">
          <div className={`transition-all duration-300 ${showFields ? 'opacity-100 translate-y-0' : 'opacity-0 -translate-y-4'}`}>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Loại sự cố
            </label>
            <select
              value={type}
              onChange={(e) => handleTypeChange(e.target.value as any)}
              disabled={loading}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 disabled:opacity-50"
            >
              <option value="accident">🚗 Tai nạn</option>
              <option value="breakdown">🔧 Hỏng xe</option>
              <option value="medical">🏥 Y tế</option>
              <option value="other">⚠️ Khác</option>
            </select>
          </div>

          <div className={`transition-all duration-300 delay-100 ${showFields ? 'opacity-100 translate-y-0' : 'opacity-0 -translate-y-4'}`}>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Mức độ nghiêm trọng
            </label>
            <select
              value={severity}
              onChange={(e) => handleSeverityChange(e.target.value as any)}
              disabled={loading}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500 disabled:opacity-50"
            >
              <option value="low">Thấp</option>
              <option value="medium">Trung bình</option>
              <option value="high">Cao</option>
              <option value="critical">Khẩn cấp</option>
            </select>
          </div>

          <div>
            <label className="block text-sm font-medium text-gray-700 mb-1">
              Ghi chú (tùy chọn)
            </label>
            <textarea
              value={note}
              onChange={(e) => setNote(e.target.value)}
              className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
              rows={3}
              placeholder="Mô tả tình huống..."
            />
          </div>

          {!hasSelectedType || !hasSelectedSeverity ? (
            <div className="flex space-x-3">
              <button
                type="button"
                onClick={onClose}
                disabled={loading}
                className="flex-1 bg-gray-300 text-gray-700 py-2 px-4 rounded-md hover:bg-gray-400 disabled:opacity-50"
              >
                Hủy
              </button>
              <button
                type="submit"
                disabled={loading}
                className="flex-1 bg-red-600 text-white py-2 px-4 rounded-md hover:bg-red-700 disabled:opacity-50"
              >
                {loading ? 'Đang gửi...' : 'Gửi SOS'}
              </button>
            </div>
          ) : (
            <div className="text-center text-sm text-gray-600">
              Đang tự động gửi SOS...
            </div>
          )}
        </form>
      </div>
    </div>
  );
}

