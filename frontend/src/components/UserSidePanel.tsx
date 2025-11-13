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
  
  // Removed debug logging to reduce console noise

  // Load thông tin trạm cho các SOS có assignedStationId
  useEffect(() => {
    const loadStationInfo = async () => {
      const newStationInfo: { [sosId: string]: any } = {};
      
      for (const sos of sosList) {
        // Load thông tin trạm nếu có assignedStationId và SOS chưa done/cancelled
        if (sos.assignedStationId && sos.status !== 'done' && sos.status !== 'cancelled') {
          try {
            const response = await apiClient.get(`/stations/${sos.assignedStationId}`);
            newStationInfo[sos.id] = response.data;
            
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
    <div className="h-full flex flex-col bg-gray-50">
      {/* Enhanced Header */}
      <div className="bg-gradient-to-r from-slate-700 via-slate-800 to-slate-900 text-white px-6 py-4 shadow-lg">
        <div className="flex items-center space-x-3">
          <div className="bg-white/20 backdrop-blur-sm p-2 rounded-lg">
            <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
            </svg>
          </div>
          <h2 className="text-xl font-bold tracking-tight">Thông tin người dùng</h2>
        </div>
      </div>

      <div className="flex-1 overflow-y-auto p-4 space-y-4">
        {/* User Info Card - Enhanced */}
        <div className="bg-white rounded-xl shadow-md border border-gray-100 overflow-hidden transition-all hover:shadow-lg">
          <button
            onClick={() => setExpandedSection(expandedSection === 'info' ? null : 'info')}
            className="w-full flex justify-between items-center font-semibold text-lg p-4 hover:bg-gray-50 transition-colors"
          >
            <div className="flex items-center space-x-2">
              <svg className="w-5 h-5 text-blue-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
              </svg>
              <span className="text-gray-800">Thông tin cá nhân</span>
            </div>
            <svg className={`w-5 h-5 text-gray-400 transition-transform ${expandedSection === 'info' ? 'rotate-180' : ''}`} fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
            </svg>
          </button>
          {expandedSection === 'info' && (
            <div className="px-4 pb-4 space-y-3 text-sm border-t border-gray-100">
              <div className="flex items-center space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
                </svg>
                <span className="font-semibold text-gray-700">Tên:</span>
                <span className="text-gray-900">{profile.name}</span>
              </div>
              <div className="flex items-center space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M7 20l4-16m2 16l4-16M6 9h14M4 15h14" />
                </svg>
                <span className="font-semibold text-gray-700">ID người dùng:</span>
                <span className="bg-gradient-to-r from-yellow-100 to-yellow-50 border border-yellow-200 px-3 py-1 rounded-lg font-mono text-xs font-bold text-yellow-800">
                  {profile.id}
                </span>
              </div>
              <div className="bg-blue-50 border-l-4 border-blue-400 p-3 rounded-r text-xs text-blue-800">
                <div className="flex items-start space-x-2">
                  <svg className="w-4 h-4 mt-0.5 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 16h-1v-4h-1m1-4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                  </svg>
                  <span>Lưu ý: ID này sẽ được dùng để cặp với thiết bị hộp đen phần cứng</span>
                </div>
              </div>
              <div className="flex items-center space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 8l7.89 5.26a2 2 0 002.22 0L21 8M5 19h14a2 2 0 002-2V7a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2z" />
                </svg>
                <span className="font-semibold text-gray-700">Email:</span>
                <span className="text-gray-900">{profile.email}</span>
              </div>
              <div className="flex items-center space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M3 5a2 2 0 012-2h3.28a1 1 0 01.948.684l1.498 4.493a1 1 0 01-.502 1.21l-2.257 1.13a11.042 11.042 0 005.516 5.516l1.13-2.257a1 1 0 011.21-.502l4.493 1.498a1 1 0 01.684.949V19a2 2 0 01-2 2h-1C9.716 21 3 14.284 3 6V5z" />
                </svg>
                <span className="font-semibold text-gray-700">SĐT:</span>
                <span className="text-gray-900">{profile.phone}</span>
              </div>
              <div className="flex items-start space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400 mt-0.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z" />
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 11a3 3 0 11-6 0 3 3 0 016 0z" />
                </svg>
                <div className="flex-1">
                  <span className="font-semibold text-gray-700">Địa chỉ:</span>
                  <span className="text-gray-900 ml-2">{profile.address}</span>
                </div>
              </div>
              <div className="flex items-center space-x-2 py-2">
                <svg className="w-4 h-4 text-gray-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z" />
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 11a3 3 0 11-6 0 3 3 0 016 0z" />
                </svg>
                <span className="font-semibold text-gray-700">Vị trí:</span>
                <span className="text-gray-900 font-mono text-xs">{profile?.lat?.toFixed(4) || 'N/A'}, {profile?.lon?.toFixed(4) || 'N/A'}</span>
              </div>
              <div className="mt-4 pt-3 border-t border-gray-200 space-y-3">
                <div className="bg-indigo-50 border-l-4 border-indigo-400 p-3 rounded-r text-xs text-indigo-800">
                  <div className="flex items-start space-x-2">
                    <svg className="w-4 h-4 mt-0.5 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M13 10V3L4 14h7v7l9-11h-7z" />
                    </svg>
                    <span>Vị trí được cập nhật tự động từ ESP32. Nếu không có dữ liệu từ ESP32, bạn có thể nhập thủ công.</span>
                  </div>
                </div>
                <button
                  onClick={() => setShowLocationForm(!showLocationForm)}
                  className="w-full flex items-center justify-center space-x-2 text-sm bg-gradient-to-r from-gray-600 to-gray-700 text-white px-4 py-2.5 rounded-lg hover:from-gray-700 hover:to-gray-800 transition-all shadow-md hover:shadow-lg transform hover:scale-[1.02] active:scale-[0.98]"
                >
                  {showLocationForm ? (
                    <>
                      <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                      </svg>
                      <span>Đóng</span>
                    </>
                  ) : (
                    <>
                      <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M11 5H6a2 2 0 00-2 2v11a2 2 0 002 2h11a2 2 0 002-2v-5m-1.414-9.414a2 2 0 112.828 2.828L11.828 15H9v-2.828l8.586-8.586z" />
                      </svg>
                      <span>Nhập thủ công</span>
                    </>
                  )}
                </button>
                {showLocationForm && (
                  <div className="mt-3 space-y-3 p-4 bg-gradient-to-br from-gray-50 to-gray-100 rounded-xl border border-gray-200">
                    <div>
                      <label className="text-xs font-semibold text-gray-700 block mb-2 flex items-center space-x-1">
                        <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 20l-5.447-2.724A1 1 0 013 16.382V5.618a1 1 0 011.447-.894L9 7m0 13l6-3m-6 3V7m6 10l4.553 2.276A1 1 0 0021 18.382V7.618a1 1 0 00-.553-.894L15 4m0 13V4m0 0L9 7" />
                        </svg>
                        <span>Vĩ độ (Latitude):</span>
                      </label>
                      <input
                        type="number"
                        step="any"
                        value={manualLat}
                        onChange={(e) => setManualLat(e.target.value)}
                        placeholder={profile.lat?.toString() || '21.0014'}
                        className="w-full text-sm px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent transition"
                      />
                    </div>
                    <div>
                      <label className="text-xs font-semibold text-gray-700 block mb-2 flex items-center space-x-1">
                        <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 20l-5.447-2.724A1 1 0 013 16.382V5.618a1 1 0 011.447-.894L9 7m0 13l6-3m-6 3V7m6 10l4.553 2.276A1 1 0 0021 18.382V7.618a1 1 0 00-.553-.894L15 4m0 13V4m0 0L9 7" />
                        </svg>
                        <span>Kinh độ (Longitude):</span>
                      </label>
                      <input
                        type="number"
                        step="any"
                        value={manualLon}
                        onChange={(e) => setManualLon(e.target.value)}
                        placeholder={profile.lon?.toString() || '105.8425'}
                        className="w-full text-sm px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500 focus:border-transparent transition"
                      />
                    </div>
                    {locationError && (
                      <div className="bg-red-50 border-l-4 border-red-400 p-3 rounded-r text-xs text-red-700 flex items-start space-x-2">
                        <svg className="w-4 h-4 mt-0.5 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                          <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4m0 4h.01M21 12a9 9 0 11-18 0 9 9 0 0118 0z" />
                        </svg>
                        <span>{locationError}</span>
                      </div>
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
                          const response = await apiClient.patch(`/auth/users/${profile.id}/location`, { lat, lon });
                          
                          // Cập nhật profile trong store với dữ liệu từ response
                          const updatedLat = response.data?.lat || lat;
                          const updatedLon = response.data?.lon || lon;
                          
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
                      className="w-full flex items-center justify-center space-x-2 text-sm bg-gradient-to-r from-green-500 to-green-600 text-white px-4 py-2.5 rounded-lg hover:from-green-600 hover:to-green-700 transition-all shadow-md hover:shadow-lg transform hover:scale-[1.02] active:scale-[0.98] disabled:opacity-50 disabled:cursor-not-allowed disabled:transform-none"
                    >
                      {updatingLocation ? (
                        <>
                          <svg className="animate-spin h-4 w-4" fill="none" viewBox="0 0 24 24">
                            <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                            <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
                          </svg>
                          <span>Đang cập nhật...</span>
                        </>
                      ) : (
                        <>
                          <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 13l4 4L19 7" />
                          </svg>
                          <span>Lưu vị trí</span>
                        </>
                      )}
                    </button>
                  </div>
                )}
              </div>
            </div>
          )}
        </div>

        {/* SOS Button - Enhanced */}
        <div className="bg-white rounded-xl shadow-md border border-gray-100 overflow-hidden transition-all hover:shadow-lg">
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
                    className="w-full py-3 px-4 rounded-lg font-semibold text-sm bg-gray-400 text-white hover:bg-gray-500 transition-all disabled:opacity-50 disabled:cursor-not-allowed shadow-md"
                    disabled={true}
                    title="Đã có SOS đang active, vui lòng hủy SOS hiện tại trước"
                  >
                    <div className="flex items-center justify-center space-x-2">
                      <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                      </svg>
                      <span>Gửi SOS (Đã có SOS active)</span>
                    </div>
                  </button>
                  <button
                    onClick={() => {
                      if (window.confirm('Bạn có chắc chắn muốn hủy SOS này?')) {
                        onCancelSOS(activeSOS.id);
                      }
                    }}
                    className="w-full flex items-center justify-center space-x-2 py-3 px-4 rounded-lg font-semibold text-lg bg-gradient-to-r from-red-600 to-red-700 text-white hover:from-red-700 hover:to-red-800 transition-all shadow-lg hover:shadow-xl transform hover:scale-[1.02] active:scale-[0.98]"
                  >
                    <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                    </svg>
                    <span>Hủy SOS</span>
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
                  className={`w-full flex items-center justify-center space-x-2 py-4 px-4 rounded-lg font-bold text-lg transition-all shadow-lg hover:shadow-xl transform hover:scale-[1.02] active:scale-[0.98] ${
                    isBlinking 
                      ? 'bg-gradient-to-r from-yellow-400 to-yellow-500 text-black animate-pulse' 
                      : 'bg-gradient-to-r from-red-600 to-red-700 text-white hover:from-red-700 hover:to-red-800'
                  }`}
                >
                  <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
                  </svg>
                  <span>Gửi SOS</span>
                </button>
              );
            }
          })()}
        </div>

        {/* Find Nearest Stations - Enhanced */}
        <div className="bg-white rounded-xl shadow-md border border-gray-100 overflow-hidden transition-all hover:shadow-lg">
          <div className="p-4 border-b border-gray-100">
            <h3 className="font-semibold text-gray-800 flex items-center space-x-2">
              <svg className="w-5 h-5 text-blue-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M17.657 16.657L13.414 20.9a1.998 1.998 0 01-2.827 0l-4.244-4.243a8 8 0 1111.314 0z" />
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 11a3 3 0 11-6 0 3 3 0 016 0z" />
              </svg>
              <span>Tìm trạm gần nhất</span>
            </h3>
          </div>
          <div className="p-4 space-y-3">
            <button
              onClick={() => onFindNearestStation('medical')}
              className="w-full flex items-center justify-center space-x-2 bg-gradient-to-r from-red-500 to-red-600 text-white py-3 px-4 rounded-lg hover:from-red-600 hover:to-red-700 transition-all shadow-md hover:shadow-lg transform hover:scale-[1.02] active:scale-[0.98]"
            >
              <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 21V5a2 2 0 00-2-2H7a2 2 0 00-2 2v16m14 0h2m-2 0h-5m-9 0H3m2 0h5M9 7h1m-1 4h1m4-4h1m-1 4h1m-5 10v-5a1 1 0 011-1h2a1 1 0 011 1v5m-4 0h4" />
              </svg>
              <span className="font-semibold">Tìm trạm y tế gần nhất</span>
            </button>
            <button
              onClick={() => onFindNearestStation('rescue')}
              className="w-full flex items-center justify-center space-x-2 bg-gradient-to-r from-orange-500 to-orange-600 text-white py-3 px-4 rounded-lg hover:from-orange-600 hover:to-orange-700 transition-all shadow-md hover:shadow-lg transform hover:scale-[1.02] active:scale-[0.98]"
            >
              <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z" />
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 12a3 3 0 11-6 0 3 3 0 016 0z" />
              </svg>
              <span className="font-semibold">Tìm trạm cứu hộ gần nhất</span>
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

        {/* SOS History - Enhanced */}
        <div className="bg-white rounded-xl shadow-md border border-gray-100 overflow-hidden transition-all hover:shadow-lg">
          <button
            onClick={() => setExpandedSection(expandedSection === 'sos' ? null : 'sos')}
            className="w-full flex justify-between items-center font-semibold p-4 hover:bg-gray-50 transition-colors"
          >
            <div className="flex items-center space-x-2">
              <svg className="w-5 h-5 text-red-600" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 8v4l3 3m6-3a9 9 0 11-18 0 9 9 0 0118 0z" />
              </svg>
              <span className="text-gray-800">Lịch sử SOS</span>
              <span className="bg-red-100 text-red-700 px-2 py-0.5 rounded-full text-xs font-bold">{sosList.length}</span>
            </div>
            <svg className={`w-5 h-5 text-gray-400 transition-transform ${expandedSection === 'sos' ? 'rotate-180' : ''}`} fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
            </svg>
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

