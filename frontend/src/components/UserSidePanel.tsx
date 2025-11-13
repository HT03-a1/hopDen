import { useState, useEffect } from 'react';
import apiClient from '../api/client';
import { useAuthStore } from '../store/authStore';

interface UserSidePanelProps {
  profile: any;
  sosList: any[];
  onSendSOS: () => void;
  onFindNearestStation: (type: 'medical' | 'rescue') => void;
  onShowRoute: (fromLat: number, fromLon: number, toLat: number, toLon: number) => void;
  onRateStation: (station: any) => void;
  showSOSModal: boolean;
  onCancelSOS?: (sosId: string) => void;
  onUpdateLocation?: (lat: number, lon: number) => void;
}

// Tính khoảng cách giữa hai điểm (Haversine formula)
function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
  const R = 6371; // Bán kính Trái Đất (km)
  const dLat = (lat2 - lat1) * Math.PI / 180;
  const dLon = (lon2 - lon1) * Math.PI / 180;
  const a = 
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
    Math.sin(dLon / 2) * Math.sin(dLon / 2);
  const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
  return R * c;
}

export default function UserSidePanel({
  profile: profileProp,
  sosList,
  onSendSOS,
  onFindNearestStation,
  onShowRoute,
  onRateStation,
  showSOSModal,
  onCancelSOS,
  onUpdateLocation,
}: UserSidePanelProps) {
  const [expandedSection, setExpandedSection] = useState<string | null>('info');
  const [isBlinking, setIsBlinking] = useState(false);
  const [showLocationForm, setShowLocationForm] = useState(false);
  const [manualLat, setManualLat] = useState('');
  const [manualLon, setManualLon] = useState('');
  const [updatingLocation, setUpdatingLocation] = useState(false);
  const [locationError, setLocationError] = useState('');
  const [stationInfo, setStationInfo] = useState<{ [sosId: string]: any }>({});
  const { profile: profileFromStore, updateProfile } = useAuthStore();
  
  // Luôn ưu tiên dùng profile từ store (đã được cập nhật) thay vì từ props
  // Store sẽ được cập nhật khi location thay đổi, nên dùng store để đảm bảo hiển thị đúng
  const profile = profileFromStore || profileProp;
  
  // Log để debug
  useEffect(() => {
    if (profileFromStore) {
      console.log('UserSidePanel - Profile from store:', {
        lat: profileFromStore.lat,
        lon: profileFromStore.lon,
        id: profileFromStore.id
      });
    }
    if (profileProp) {
      console.log('UserSidePanel - Profile from props:', {
        lat: profileProp.lat,
        lon: profileProp.lon,
        id: profileProp.id
      });
    }
  }, [profileFromStore, profileProp]);

  // Load thông tin trạm cho các SOS có assignedStationId
  useEffect(() => {
    const loadStationInfo = async () => {
      const newStationInfo: { [sosId: string]: any } = {};
      
      for (const sos of sosList) {
        // Load thông tin trạm nếu có assignedStationId và SOS chưa done/cancelled
        if (sos.assignedStationId && sos.status !== 'done' && sos.status !== 'cancelled') {
          try {
            console.log(`Loading station info for SOS ${sos.id}, stationId: ${sos.assignedStationId}`);
            const response = await apiClient.get(`/stations/${sos.assignedStationId}`);
            newStationInfo[sos.id] = response.data;
            console.log(`Station info loaded for SOS ${sos.id}:`, response.data);
            
            // Tính khoảng cách nếu có vị trí
            if (profile?.lat && profile?.lon && response.data.lat && response.data.lon && sos.location) {
              const distance = calculateDistance(
                sos.location.lat,
                sos.location.lon,
                response.data.lat,
                response.data.lon
              );
              newStationInfo[sos.id].distance = distance;
            }
          } catch (error: any) {
            console.error(`Error loading station info for SOS ${sos.id}:`, error);
            console.error(`Error details:`, error.response?.data || error.message);
          }
        }
      }
      
      console.log('Final stationInfo:', newStationInfo);
      setStationInfo(newStationInfo);
    };

    if (sosList.length > 0) {
      loadStationInfo();
    } else {
      setStationInfo({});
    }
  }, [sosList, profile]);

  // Blinking effect for SOS button when modal is open
  useEffect(() => {
    if (showSOSModal) {
      const interval = setInterval(() => {
        setIsBlinking(prev => !prev);
      }, 500);
      return () => clearInterval(interval);
    } else {
      setIsBlinking(false);
    }
  }, [showSOSModal]);

  return (
    <div className="h-full flex flex-col">
      <div className="p-4 bg-blue-600 text-white">
        <h2 className="text-xl font-bold">Thông tin người dùng</h2>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* User Info */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'info' ? null : 'info')}
            className="w-full flex justify-between items-center font-semibold text-lg mb-2"
          >
            <span>Thông tin cá nhân</span>
            <span>{expandedSection === 'info' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'info' && (
            <div className="space-y-2 text-sm">
              <div>
                <span className="font-semibold">Tên:</span> {profile.name}
              </div>
              <div>
                <span className="font-semibold">ID người dùng:</span>{' '}
                <span className="bg-yellow-100 px-2 py-1 rounded font-mono text-xs">
                  {profile.id}
                </span>
              </div>
              <div className="text-xs text-gray-500 mt-2">
                Lưu ý: ID này sẽ được dùng để cặp với thiết bị hộp đen phần cứng
              </div>
              <div>
                <span className="font-semibold">Email:</span> {profile.email}
              </div>
              <div>
                <span className="font-semibold">SĐT:</span> {profile.phone}
              </div>
              <div>
                <span className="font-semibold">Địa chỉ:</span> {profile.address}
              </div>
              <div>
                <span className="font-semibold">Vị trí:</span> {profile?.lat?.toFixed(4) || 'N/A'}, {profile?.lon?.toFixed(4) || 'N/A'}
              </div>
              <div className="mt-3 space-y-2">
                <button
                  onClick={async () => {
                    if (!navigator.geolocation) {
                      setLocationError('Trình duyệt không hỗ trợ Geolocation');
                      setShowLocationForm(true);
                      return;
                    }
                    
                    setUpdatingLocation(true);
                    setLocationError('');
                    
                    navigator.geolocation.getCurrentPosition(
                      async (position) => {
                        const lat = position.coords.latitude;
                        const lon = position.coords.longitude;
                        
                        try {
                          const response = await apiClient.patch(`/auth/users/${profile.id}/location`, { lat, lon });
                          // Cập nhật profile trong store
                          updateProfile({ lat, lon });
                          if (onUpdateLocation) {
                            onUpdateLocation(lat, lon);
                          }
                          alert('Cập nhật vị trí thành công!');
                        } catch (error: any) {
                          setLocationError(error.response?.data?.error || 'Không thể cập nhật vị trí');
                        } finally {
                          setUpdatingLocation(false);
                        }
                      },
                      (error) => {
                        console.error('Geolocation error:', error);
                        setLocationError('Không thể lấy vị trí GPS. Vui lòng nhập thủ công.');
                        setShowLocationForm(true);
                        setUpdatingLocation(false);
                      },
                      {
                        enableHighAccuracy: true,
                        timeout: 10000,
                        maximumAge: 0,
                      }
                    );
                  }}
                  disabled={updatingLocation}
                  className="w-full text-xs bg-blue-500 text-white px-3 py-2 rounded hover:bg-blue-600 transition disabled:opacity-50"
                >
                  {updatingLocation ? 'Đang cập nhật...' : '📍 Cập nhật vị trí (GPS)'}
                </button>
                <button
                  onClick={() => setShowLocationForm(!showLocationForm)}
                  className="w-full text-xs bg-gray-500 text-white px-3 py-2 rounded hover:bg-gray-600 transition"
                >
                  {showLocationForm ? '✖️ Đóng' : '✏️ Nhập thủ công'}
                </button>
                {showLocationForm && (
                  <div className="mt-2 space-y-2 p-3 bg-gray-50 rounded">
                    <div>
                      <label className="text-xs font-semibold block mb-1">Vĩ độ (Latitude):</label>
                      <input
                        type="number"
                        step="any"
                        value={manualLat}
                        onChange={(e) => setManualLat(e.target.value)}
                        placeholder={profile.lat?.toString() || '21.0014'}
                        className="w-full text-xs px-2 py-1 border rounded"
                      />
                    </div>
                    <div>
                      <label className="text-xs font-semibold block mb-1">Kinh độ (Longitude):</label>
                      <input
                        type="number"
                        step="any"
                        value={manualLon}
                        onChange={(e) => setManualLon(e.target.value)}
                        placeholder={profile.lon?.toString() || '105.8425'}
                        className="w-full text-xs px-2 py-1 border rounded"
                      />
                    </div>
                    {locationError && (
                      <div className="text-xs text-red-600">{locationError}</div>
                    )}
                    <button
                      onClick={async () => {
                        // Bắt buộc phải nhập cả lat và lon, không dùng giá trị mặc định
                        if (!manualLat || !manualLon) {
                          setLocationError('Vui lòng nhập đầy đủ cả vĩ độ và kinh độ');
                          return;
                        }
                        
                        const lat = parseFloat(manualLat);
                        const lon = parseFloat(manualLon);
                        
                        if (isNaN(lat) || isNaN(lon)) {
                          setLocationError('Tọa độ phải là số hợp lệ');
                          return;
                        }
                        
                        if (lat < -90 || lat > 90 || lon < -180 || lon > 180) {
                          setLocationError('Tọa độ nằm ngoài phạm vi hợp lệ (Lat: -90 đến 90, Lon: -180 đến 180)');
                          return;
                        }
                        
                        setUpdatingLocation(true);
                        setLocationError('');
                        
                        try {
                          console.log('Updating location manually:', { lat, lon, userId: profile.id });
                          const response = await apiClient.patch(`/auth/users/${profile.id}/location`, { lat, lon });
                          console.log('Location update response:', response.data);
                          
                          // Cập nhật profile trong store với dữ liệu từ response
                          const updatedLat = response.data?.lat || lat;
                          const updatedLon = response.data?.lon || lon;
                          
                          console.log('Updating profile in store with:', { lat: updatedLat, lon: updatedLon });
                          
                          // Cập nhật store ngay lập tức
                          updateProfile({ lat: updatedLat, lon: updatedLon });
                          
                          // Gọi callback để cập nhật MapPage
                          if (onUpdateLocation) {
                            onUpdateLocation(updatedLat, updatedLon);
                          }
                          
                          setManualLat('');
                          setManualLon('');
                          setShowLocationForm(false);
                          
                          // Hiển thị thông báo thành công với thông tin chi tiết
                          alert(`✅ Cập nhật vị trí thành công!\n\nVĩ độ: ${updatedLat.toFixed(6)}\nKinh độ: ${updatedLon.toFixed(6)}\n\nVị trí đã được cập nhật trong thông tin cá nhân.`);
                          
                          // Không reload page, để user thấy thay đổi ngay
                          // Component sẽ tự động re-render khi profile trong store thay đổi
                        } catch (error: any) {
                          console.error('Error updating location:', error);
                          setLocationError(error.response?.data?.error || 'Không thể cập nhật vị trí');
                        } finally {
                          setUpdatingLocation(false);
                        }
                      }}
                      disabled={updatingLocation || !manualLat || !manualLon}
                      className="w-full text-xs bg-green-500 text-white px-3 py-2 rounded hover:bg-green-600 transition disabled:opacity-50 disabled:cursor-not-allowed"
                    >
                      {updatingLocation ? 'Đang cập nhật...' : '✓ Lưu vị trí'}
                    </button>
                  </div>
                )}
              </div>
            </div>
          )}
        </div>

        {/* SOS Button */}
        <div className="bg-white rounded-lg shadow p-4 relative">
          {/* Kiểm tra xem có SOS đang active không */}
          {(() => {
            const activeSOS = sosList.find(sos => 
              sos.status === 'pending' || sos.status === 'accepted' || sos.status === 'on_route'
            );
            
            if (activeSOS && onCancelSOS) {
              // Nếu có SOS active, hiển thị cả nút Gửi SOS và Hủy SOS
              return (
                <div className="space-y-2">
                  <button
                    onClick={onSendSOS}
                    className="w-full py-2 px-4 rounded-md font-semibold text-sm bg-gray-400 text-white hover:bg-gray-500 transition disabled:opacity-50"
                    disabled={true}
                    title="Đã có SOS đang active, vui lòng hủy SOS hiện tại trước"
                  >
                    🚨 Gửi SOS (Đã có SOS active)
                  </button>
                  <button
                    onClick={() => {
                      if (window.confirm('Bạn có chắc chắn muốn hủy SOS này?')) {
                        onCancelSOS(activeSOS.id);
                      }
                    }}
                    className="w-full py-3 px-4 rounded-md font-semibold text-lg bg-red-600 text-white hover:bg-red-700 transition"
                  >
                    ❌ Hủy SOS
                  </button>
                  <div className="text-xs text-center text-gray-600 mt-1">
                    SOS đang active: {activeSOS.type === 'accident' && '🚗 Tai nạn'}
                    {activeSOS.type === 'breakdown' && '🔧 Hỏng xe'}
                    {activeSOS.type === 'medical' && '🏥 Y tế'}
                    {activeSOS.type === 'other' && '⚠️ Khác'} - {activeSOS.status === 'pending' && 'Chờ xử lý'}
                    {activeSOS.status === 'accepted' && 'Đã nhận'}
                    {activeSOS.status === 'on_route' && 'Đang đi'}
                  </div>
                </div>
              );
            } else {
              // Nếu không có SOS active, hiển thị nút Gửi SOS bình thường
              return (
                <button
                  onClick={onSendSOS}
                  className={`w-full py-3 px-4 rounded-md font-semibold text-lg transition-all ${
                    isBlinking 
                      ? 'bg-yellow-500 text-black animate-pulse' 
                      : 'bg-red-600 text-white hover:bg-red-700'
                  }`}
                >
                  🚨 Gửi SOS
                </button>
              );
            }
          })()}
        </div>

        {/* Find Nearest Stations */}
        <div className="bg-white rounded-lg shadow p-4">
          <h3 className="font-semibold mb-3">Tìm trạm gần nhất</h3>
          <div className="space-y-2">
            <button
              onClick={() => onFindNearestStation('medical')}
              className="w-full bg-red-500 text-white py-2 px-4 rounded-md hover:bg-red-600"
            >
              🏥 Tìm trạm y tế gần nhất
            </button>
            <button
              onClick={() => onFindNearestStation('rescue')}
              className="w-full bg-orange-500 text-white py-2 px-4 rounded-md hover:bg-orange-600"
            >
              🔧 Tìm trạm cứu hộ gần nhất
            </button>
          </div>
        </div>

        {/* Thông tin trạm đang kết nối */}
        {(() => {
          // Tìm SOS đang active (pending, accepted, on_route)
          const activeSOS = sosList.find(sos => 
            sos.status === 'pending' || sos.status === 'accepted' || sos.status === 'on_route'
          );
          
          // Hiển thị nếu có SOS active và có assignedStationId (đã được gán cho trạm)
          if (activeSOS && activeSOS.assignedStationId) {
            const station = stationInfo[activeSOS.id];
            
            // Xác định trạng thái hiển thị
            const statusText = activeSOS.status === 'pending' ? '🔄 Đang kết nối' :
                              activeSOS.status === 'accepted' ? '✅ Đã nhận' :
                              activeSOS.status === 'on_route' ? '🚗 Đang đi' : 'Chờ xử lý';
            
            const statusColor = activeSOS.status === 'pending' ? 'bg-yellow-100 text-yellow-800' :
                               activeSOS.status === 'accepted' ? 'bg-green-100 text-green-800' :
                               activeSOS.status === 'on_route' ? 'bg-purple-100 text-purple-800' :
                               'bg-gray-100 text-gray-800';
            
            return (
              <div className="bg-white rounded-lg shadow p-4">
                <div className="flex items-center justify-between mb-3">
                  <h3 className="font-semibold text-lg">🏥 Trạm đang kết nối</h3>
                  <span className={`text-xs px-2 py-1 rounded font-semibold ${statusColor}`}>
                    {statusText}
                  </span>
                </div>
                
                {station ? (
                  <div className="space-y-3">
                    <div className={`p-4 rounded-lg border-2 ${
                      activeSOS.status === 'pending' ? 'bg-yellow-50 border-yellow-200' :
                      activeSOS.status === 'accepted' ? 'bg-green-50 border-green-200' :
                      activeSOS.status === 'on_route' ? 'bg-purple-50 border-purple-200' :
                      'bg-gray-50 border-gray-200'
                    }`}>
                      <div className={`text-lg font-bold mb-2 ${
                        activeSOS.status === 'pending' ? 'text-yellow-900' :
                        activeSOS.status === 'accepted' ? 'text-green-900' :
                        activeSOS.status === 'on_route' ? 'text-purple-900' :
                        'text-gray-900'
                      }`}>
                        {station.stationName || 'Đang tải...'}
                      </div>
                      
                      <div className="space-y-2 text-sm">
                        <div className="flex items-center text-gray-700">
                          <span className="font-semibold w-20">📞 SĐT:</span>
                          <span>{station.phone || 'N/A'}</span>
                        </div>
                        
                        <div className="flex items-start text-gray-700">
                          <span className="font-semibold w-20">📍 Địa chỉ:</span>
                          <span className="flex-1">{station.address || 'N/A'}</span>
                        </div>
                        
                        {station.distance !== undefined && (
                          <div className="flex items-center text-blue-700 font-semibold">
                            <span className="font-semibold w-20">📏 Khoảng cách:</span>
                            <span className="text-lg">{station.distance.toFixed(2)} km</span>
                          </div>
                        )}
                        
                        {station.ratingAvg !== undefined && (
                          <div className="flex items-center text-yellow-700">
                            <span className="font-semibold w-20">⭐ Đánh giá:</span>
                            <span className="flex items-center gap-1">
                              <span className="text-lg font-bold">{station.ratingAvg.toFixed(1)}</span>
                              <span className="text-xs text-gray-600">({station.ratingCount || 0} lượt)</span>
                            </span>
                          </div>
                        )}
                        
                        {station.openHours && (
                          <div className="flex items-center text-gray-700">
                            <span className="font-semibold w-20">🕐 Giờ làm việc:</span>
                            <span>{station.openHours}</span>
                          </div>
                        )}
                      </div>
                    </div>
                    
                    {/* Thông tin SOS */}
                    <div className="bg-gray-50 p-3 rounded border border-gray-200">
                      <div className="text-xs text-gray-600 mb-1">
                        <span className="font-semibold">Loại SOS:</span> {
                          activeSOS.type === 'accident' && '🚗 Tai nạn'
                        }
                        {activeSOS.type === 'breakdown' && '🔧 Hỏng xe'}
                        {activeSOS.type === 'medical' && '🏥 Y tế'}
                        {activeSOS.type === 'other' && '⚠️ Khác'}
                      </div>
                      <div className="text-xs text-gray-600">
                        <span className="font-semibold">Mức độ:</span> {activeSOS.severity}
                      </div>
                      {activeSOS.note && (
                        <div className="text-xs text-gray-700 mt-2 italic">
                          💬 {activeSOS.note}
                        </div>
                      )}
                    </div>
                  </div>
                ) : (
                  <div className="bg-yellow-50 border border-yellow-200 rounded-lg p-4 text-center">
                    <div className="text-yellow-700 text-sm">
                      🔄 Đang tải thông tin trạm...
                    </div>
                  </div>
                )}
              </div>
            );
          }
          // Nếu có SOS pending nhưng chưa có assignedStationId, hiển thị đang tìm trạm
          else if (activeSOS && activeSOS.status === 'pending' && !activeSOS.assignedStationId) {
            return (
              <div className="bg-white rounded-lg shadow p-4">
                <div className="flex items-center justify-between mb-3">
                  <h3 className="font-semibold text-lg">🏥 Trạm đang kết nối</h3>
                  <span className="text-xs px-2 py-1 rounded font-semibold bg-yellow-100 text-yellow-800">
                    🔄 Đang tìm trạm
                  </span>
                </div>
                <div className="bg-yellow-50 border border-yellow-200 rounded-lg p-4 text-center">
                  <div className="text-yellow-700 text-sm">
                    🔍 Đang tìm trạm phù hợp gần nhất...
                  </div>
                </div>
              </div>
            );
          }
          return null;
        })()}

        {/* SOS History */}
        <div className="bg-white rounded-lg shadow p-4">
          <button
            onClick={() => setExpandedSection(expandedSection === 'sos' ? null : 'sos')}
            className="w-full flex justify-between items-center font-semibold mb-2"
          >
            <span>Lịch sử SOS ({sosList.length})</span>
            <span>{expandedSection === 'sos' ? '▼' : '▶'}</span>
          </button>
          {expandedSection === 'sos' && (
            <div className="space-y-2 max-h-64 overflow-y-auto">
              {sosList.length === 0 ? (
                <p className="text-sm text-gray-500">Chưa có lịch sử SOS</p>
              ) : (
                sosList.map((sos) => {
                  const station = stationInfo[sos.id];
                  return (
                    <div key={sos.id} className="border-l-4 border-red-500 pl-3 py-2 text-sm">
                      <div className="font-semibold">
                        {sos.type === 'accident' && '🚗 Tai nạn'}
                        {sos.type === 'breakdown' && '🔧 Hỏng xe'}
                        {sos.type === 'medical' && '🏥 Y tế'}
                        {sos.type === 'other' && '⚠️ Khác'}
                      </div>
                      <div className="text-xs text-gray-600">
                        Mức độ: {sos.severity} | Trạng thái: {sos.status === 'pending' && 'Chờ xử lý'}
                        {sos.status === 'accepted' && 'Đã nhận'}
                        {sos.status === 'on_route' && 'Đang đi'}
                        {sos.status === 'done' && 'Hoàn thành'}
                        {sos.status === 'cancelled' && 'Đã hủy'}
                      </div>
                      <div className="text-xs text-gray-500">
                        {new Date(sos.createdAt).toLocaleString('vi-VN')}
                      </div>
                      
                      {/* Hiển thị thông tin trạm đang kết nối */}
                      {station && (sos.status === 'accepted' || sos.status === 'on_route') && (
                        <div className="mt-2 p-2 bg-blue-50 border border-blue-200 rounded">
                          <div className="text-xs font-semibold text-blue-700 mb-1">
                            🏥 Trạm đang kết nối:
                          </div>
                          <div className="text-xs text-gray-700">
                            <div className="font-semibold">{station.stationName || 'N/A'}</div>
                            <div>📞 {station.phone || 'N/A'}</div>
                            <div>📍 {station.address || 'N/A'}</div>
                            {station.distance !== undefined && (
                              <div className="text-blue-600 font-semibold mt-1">
                                📏 Khoảng cách: {station.distance.toFixed(2)} km
                              </div>
                            )}
                            {station.ratingAvg !== undefined && (
                              <div className="text-yellow-600 mt-1">
                                ⭐ Đánh giá: {station.ratingAvg.toFixed(1)} ({station.ratingCount || 0} lượt)
                              </div>
                            )}
                          </div>
                        </div>
                      )}
                      
                      {/* Hiển thị thông báo đang tìm trạm */}
                      {sos.status === 'pending' && sos.assignedStationId && (
                        <div className="mt-2 p-2 bg-yellow-50 border border-yellow-200 rounded text-xs text-yellow-700">
                          🔄 Đang tìm trạm phù hợp...
                        </div>
                      )}
                      
                      <div className="mt-2 flex gap-2 flex-wrap">
                        {(sos.status === 'pending' || sos.status === 'accepted' || sos.status === 'on_route') && onCancelSOS && (
                          <button
                            onClick={() => {
                              if (window.confirm('Bạn có chắc chắn muốn hủy SOS này?')) {
                                onCancelSOS(sos.id);
                              }
                            }}
                            className="text-xs bg-red-500 text-white px-2 py-1 rounded hover:bg-red-600"
                          >
                            ❌ Hủy SOS
                          </button>
                        )}
                        {sos.status === 'done' && sos.assignedStationId && (
                          <button
                            onClick={() => onRateStation({ id: sos.assignedStationId })}
                            className="text-xs bg-blue-500 text-white px-2 py-1 rounded hover:bg-blue-600"
                          >
                            Đánh giá trạm
                          </button>
                        )}
                      </div>
                    </div>
                  );
                })
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

